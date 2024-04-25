#ifndef _g_shader_h_
#define _g_shader_h_

#include "include/GShader.h"
#include "include/GMatrix.h"
#include "clipping.h"

/**
 *  GShaders create colors to fill whatever geometry is being drawn to a GCanvas.
 */
class MyShader : public GShader {
  public:
    MyShader(const GBitmap& device, const GMatrix& matrix, const GTileMode tileMode) : fDevice(device), fMat(matrix), fTileMode(tileMode) {}

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() override { 
      return fDevice.isOpaque(); 
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    bool setContext(const GMatrix& ctm) override { 
      GMatrix mat = ctm * fMat;

      if (auto inv = mat.invert()) {
        fInv = *inv;
        return true;
      }
      
      return false;
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */

    // update to adjust for tile modes
    void shadeRow(int x, int y, int count, GPixel row[]) override { 

      // compute initial inv * (x, y)
      float xp = fInv[0] * (x + 0.5f) + fInv[2] * (y + 0.5f) + fInv[4];
      float yp = fInv[1] * (x + 0.5f) + fInv[3] * (y + 0.5f) + fInv[5];

      switch(fTileMode) {
        case GTileMode::kClamp:

          // y never changes
          if (fInv[1] == 0.0f) {
            int currY = clampY(fDevice, GFloorToInt(yp));
            
            for (int i = 0; i < count; i++) {
              int currX = GFloorToInt(xp);

              // check for coords outside bitmap and "clamp"
              currX = clampX(fDevice, currX);

              row[i] = *(fDevice.getAddr(currX, currY));

              xp += fInv[0];
            } 
          } else {
            for (int i = 0; i < count; i++) {
              int currX = GFloorToInt(xp);
              int currY = GFloorToInt(yp);

              // check for coords outside bitmap and "clamp"
              currX = clampX(fDevice, currX);
              currY = clampY(fDevice, currY);

              row[i] = *(fDevice.getAddr(currX, currY));

              xp += fInv[0];
              yp += fInv[1];
            } 
          }

          break;
        
        case GTileMode::kMirror:
          // y never changes
          if (fInv[1] == 0.0f) {
            int currY = mirrorY(fDevice, GFloorToInt(yp));
            
            for (int i = 0; i < count; i++) {
              int currX = GFloorToInt(xp);

              currX = mirrorX(fDevice, currX);

              row[i] = *(fDevice.getAddr(currX, currY));

              xp += fInv[0];
            } 
          } else {
            for (int i = 0; i < count; i++) {
              int currX = GFloorToInt(xp);
              int currY = GFloorToInt(yp);

              currX = mirrorX(fDevice, currX);
              currY = mirrorY(fDevice, currY);

              row[i] = *(fDevice.getAddr(currX, currY));

              xp += fInv[0];
              yp += fInv[1];
            } 
          }

          break;

        case GTileMode::kRepeat:
        // y never changes
          if (fInv[1] == 0.0f) {
            int currY = repeatY(fDevice, GFloorToInt(yp));
            
            for (int i = 0; i < count; i++) {
              int currX = GFloorToInt(xp);

              currX = repeatX(fDevice, currX);

              row[i] = *(fDevice.getAddr(currX, currY));

              xp += fInv[0];
            } 
          } else {
            for (int i = 0; i < count; i++) {
              int currX = GFloorToInt(xp);
              int currY = GFloorToInt(yp);

              currX = repeatX(fDevice, currX);
              currY = repeatY(fDevice, currY);

              row[i] = *(fDevice.getAddr(currX, currY));

              xp += fInv[0];
              yp += fInv[1];
            } 
          }

          break;

        default:
          break;

      }
    }

  private:
    const GBitmap fDevice;
    const GMatrix fMat;
    const GTileMode fTileMode;
    GMatrix fInv = GMatrix();
};


/**
 *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
 *  Returns null if the subclass can not be created.
 */
std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& device, const GMatrix& localMatrix, const GTileMode tileMode) {
  return std::unique_ptr<GShader>(new MyShader(device, localMatrix, tileMode));
}

GPixel ctp(const GColor& color) {
  int r = GRoundToInt(color.r * color.a * 255);
  int g = GRoundToInt(color.g * color.a * 255);
  int b = GRoundToInt(color.b * color.a * 255);
  int a = GRoundToInt(color.a * 255);

  return GPixel_PackARGB(a, r, g, b);
}

GColor clamp_color(const GColor color) {
  GColor col;
  col.a = color.a >= 1.0f ? 1.0f : (color.a <= 0.0f ? 0.0f : color.a);
  col.r = color.r >= 1.0f ? 1.0f : (color.r <= 0.0f ? 0.0f : color.r);
  col.g = color.g >= 1.0f ? 1.0f : (color.g <= 0.0f ? 0.0f : color.g);
  col.b = color.b >= 1.0f ? 1.0f : (color.b <= 0.0f ? 0.0f : color.b);

  return col;
}

class MyGradientShader : public GShader {
  public:
    MyGradientShader(const GPoint pt0, const GPoint pt1, const GColor colors[], int count, const GTileMode tileMode) :  fP0(pt0), fP1(pt1), fCount(count), fTileMode(tileMode) {

      for (int i = 0; i < count; i++) {
        fColors.push_back(clamp_color(colors[i]));

        if (i == count - 1) {
          fColorDiffs.push_back({ 0, 0, 0, 0 });
        } else {
          fColorDiffs.push_back(clamp_color(colors[i + 1]) - clamp_color(colors[i]));
        }
      }

      fColors.push_back({ 0, 0, 0, 0 });
    }

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() override { return false; }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    bool setContext(const GMatrix& ctm) override { 
      GMatrix transform = GMatrix(
        fP1.x - fP0.x,    -(fP1.y - fP0.y),   fP0.x,
        fP1.y - fP0.y,    fP1.x - fP0.x,      fP0.y
      );

      GMatrix m = ctm * transform;

      if (auto inv = m.invert()) {
        fInv = *inv;
        return true;
      }

      return false;
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) override { 
      if (fCount == 1) {
        GPixel pix = ctp(fColors[0]);
        for (int i = 0; i < count; i++) row[i] = pix;
        return;
      }

      float xpr = (fInv[0] * (x + 0.5f) + fInv[2] * (y + 0.5f) + fInv[4]) * (fCount - 1);
      float currX = xpr;
      if (currX < 0) currX = 0;
      if (currX > fCount - 1) currX = fCount - 1;

      int k = GFloorToInt(currX);
      float t = currX - (float) k;
      GColor mix = fColors[k] + (t) * fColorDiffs[k];

      switch (fTileMode) {
        case GTileMode::kClamp:

          for (int i = 0; i < count; i++) {
            currX = xpr;
            if (currX < 0) currX = 0;
            if (currX > fCount - 1) currX = fCount - 1;

            k = GFloorToInt(currX);
            t = currX - (float) k;

            assert(t >= 0.0f && t < 1.0f);

            mix = fColors[k] + (t) * fColorDiffs[k];

            assert(mix.a <= 1.0f && mix.r <= 1.0f && mix.g <= 1.0f && mix.b <= 1.0f);

            row[i] = ctp(mix);

            xpr += fInv[0] * (fCount - 1);
          }

          break;

        case GTileMode::kMirror:
          for (int i = 0; i < count; i++) {
            currX = xpr;

            currX = fmod(currX, 2 * (fCount - 1));

            if (currX > fCount - 1) {
              currX = 2 * (fCount - 1) - currX;
            }

            k = GFloorToInt(currX);
            t = currX - (float) k;

            assert(t >= 0.0f && t < 1.0f);

            mix = fColors[k] + (t) * fColorDiffs[k];

            assert(mix.a <= 1.0f && mix.r <= 1.0f && mix.g <= 1.0f && mix.b <= 1.0f);

            row[i] = ctp(mix);

            xpr += fInv[0] * (fCount - 1);
          }
          break;

        case GTileMode::kRepeat:
          for (int i = 0; i < count; i++) {
            currX = xpr;

            if (currX < 0) currX = 0;
            if (currX > fCount - 1) currX = fmod(currX, (float) fCount);

            k = GFloorToInt(currX);
            t = currX - (float) k;

            assert(t >= 0.0f && t < 1.0f);

            mix = fColors[k] + (t) * fColorDiffs[k];

            assert(mix.a <= 1.0f && mix.r <= 1.0f && mix.g <= 1.0f && mix.b <= 1.0f);

            row[i] = ctp(mix);

            xpr += fInv[0] * (fCount - 1);
          }
          break;

        default:
          break;
      }

    }

  private:
    const GPoint fP0;
    const GPoint fP1;
    int fCount;
    const GTileMode fTileMode;
    std::vector<GColor> fColors {};
    std::vector<GColor> fColorDiffs {};
    GMatrix fInv;

};

std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count, GTileMode tileMode) {
  return std::unique_ptr<GShader>(new MyGradientShader(p0, p1, colors, count, tileMode));
}

class TriangleGradientShader : public GShader {
  public:
    TriangleGradientShader(const GPoint pts[], const GColor colors[]) : fP0(pts[0]), fP1(pts[1]), fP2(pts[2]) {
      fCols[0] = colors[0];
      fCols[1] = colors[1];
      fCols[2] = colors[2];
    }

    bool isOpaque() override { return false; }

    bool setContext(const GMatrix& ctm) override {
      GMatrix transform = GMatrix(
        fP1.x - fP0.x,    fP2.x - fP0.x,    fP0.x,
        fP1.y - fP0.y,    fP2.y - fP0.y,    fP0.y
      );

      GMatrix mat = ctm * transform;

      if (auto inv = mat.invert()) {
        fInv = *inv;

        fDiffC = fInv[0] * fDiffC1 + fInv[3] * fDiffC2;
        return true;
      }
      
      return false;
    }

    void shadeRow(int x, int y, int count, GPixel row[]) override {
      
      GPoint p = {x + 0.5f, y + 0.5f};
      GPoint pp;

      GColor c;
      
      for (int i = 0; i < count; i++) {

        pp = fInv * p;

        pp.x = pp.x < 0.0f ? 0.0f : (pp.x > 1.0f ? 1.0f : pp.x);
        pp.y = pp.y < 0.0f ? 0.0f : (pp.y > 1.0f ? 1.0f : pp.y);

        c = pp.x * fCols[1] + pp.y * fCols[2] + (1 - pp.x - pp.y) * fCols[0];

        assert(c.a <= 1.0f);
        assert(c.r <= 1.0f);
        assert(c.g <= 1.0f);
        assert(c.b <= 1.0f);

        row[i] = ctp(c);
        p.x += 1.0f;
        // c += fDiffC;
      }
    }

  private:
    const GPoint fP0;
    const GPoint fP1;
    const GPoint fP2;

    GColor fCols[3] {};

    GColor fDiffC1;
    GColor fDiffC2;

    GColor fDiffC;

    GMatrix fInv;
};

GShader* GCreateTriangleGradient(const GPoint pts[], const GColor colors[]) {
  return new TriangleGradientShader(pts, colors);
}

class ProxyShader : public GShader {
  GShader* fRealShader;
  GMatrix  fExtraTransform;
public:
    ProxyShader(GShader* shader, const GMatrix& extraTransform)
        : fRealShader(shader), fExtraTransform(extraTransform) {}

    bool isOpaque() override { return fRealShader->isOpaque(); }

    bool setContext(const GMatrix& ctm) override {
        return fRealShader->setContext(ctm * fExtraTransform);
    }
    
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        fRealShader->shadeRow(x, y, count, row);
    }
};

std::unique_ptr<GShader> GCreateProxyShader(const GPoint pts[3], const GPoint texs[3], GShader* origShader) {
  GPaint paint;
  GMatrix p = compute_basis(pts[0], pts[1], pts[2]);
  GMatrix t = compute_basis(texs[0], texs[1], texs[2]);

  auto invT = t.invert();
  GMatrix mat = p * (*invT);

  return std::unique_ptr<GShader>(new ProxyShader(origShader, mat));
}

#endif  