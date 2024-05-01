#include <iostream>
#include "canvas.h"
#include "include/GMatrix.h"
#include "include/GPixel.h"
#include "include/GPath.h"

#include "blending.h"
#include "matrix.h"

// duplicate top of stack
void MyCanvas::save() {
  GMatrix dup = ctm[ctm.size() - 1];
  ctm.push_back(dup);
}

// pop top of stack
void MyCanvas::restore() {
  ctm.erase(ctm.end() - 1);
}

// top of stack * matrix
void MyCanvas::concat(const GMatrix& matrix) {
  GMatrix& top = ctm.back();
  GMatrix res = GMatrix::Concat(top, matrix);

  top = res;
}

void MyCanvas::clear(const GColor& color) {

  GPixel src = color_to_pixel(color);

  for (int y = 0; y < fDevice.height(); y++) {
    GPixel* pixel = fDevice.getAddr(0, y);
    for (int x = 0; x < fDevice.width(); x++) {
      pixel[x] = src;
    }
  }
}

void MyCanvas::drawRect(const GRect& rect, const GPaint& paint) {
  // first get the transformed points
  GPoint p1 = { rect.left, rect.top };
  GPoint p2 = { rect.right, rect.top };
  GPoint p3 = { rect.right, rect.bottom };
  GPoint p4 = { rect.left, rect.bottom };

  GPoint pts[4] = { p1, p2, p3, p4 };

  // GMatrix mat = ctm[ctm.size() - 1];

  drawConvexPolygon(pts, 4, paint);
}

void MyCanvas::drawConvexPolygon(const GPoint* pts, int count, const GPaint& paint) {
  // must have at least 3 points?
  if (count < 3) return;

  // retrieve top of stack
  GMatrix mat = ctm[ctm.size() - 1];
  GPoint dst[count];

  // map points using top of stack
  mat.mapPoints(dst, pts, count);
  
  std::vector<Segment> segments;

  pts_to_segments(fDevice, segments, dst, count);
  if (segments.size() < 2) return;
  
  std::sort(segments.begin(), segments.end());

  BlendProc blendMode = gProcs[(int) paint.getBlendMode()];

  if (paint.getShader()) {
    GShader* sh = paint.getShader();

    if (sh->setContext(mat)) {
      if (sh->isOpaque()) {
        if (blendMode == srcOverMode) blendMode = srcMode;
        if (blendMode == dstInMode) blendMode = dstMode;
        if (blendMode == srcATopMode) blendMode = srcInMode;
        if (blendMode == dstOutMode) blendMode = clearMode;
        if (blendMode == xorMode) blendMode = srcOutMode;
      }

      shade_fill_convex_polygon(fDevice, segments, sh, blendMode); 
    }

  } else {
    GPixel src = color_to_pixel(paint.getColor());

    blendMode = simplify_blend_mode(paint, blendMode);

    fill_convex_polygon(fDevice, segments, src, blendMode);
  }
}

void MyCanvas::drawPath(const GPath& path, const GPaint& paint) {
  GMatrix mat = ctm[ctm.size() - 1];
  GPath transform = path;
  transform.transform(mat);

  GPoint pts[4];
  GPath::Edger iter(transform);

  std::vector<Segment> segments;

  // Handle quadratic and cubic curves in the path when clipping
  // Draw curves with a tolerance of 1/4 pixel

  while (auto v = iter.next(pts)) {
    switch (v.value()) {
      case GPath::kLine: // pts[0..1]
        clip_segment(fDevice, segments, pts[0], pts[1]);
        break;

      case GPath::kQuad: // pts[0..2]
        clip_quad_curve(fDevice, segments, pts[0], pts[1], pts[2]);
        break;

      case GPath::kCubic: // pts[0..3]
        clip_cubic_curve(fDevice, segments, pts[0], pts[1], pts[2], pts[3]);
        break;

      default:
        break;
    }
  }

  if (segments.size() < 2) return;
  
  std::sort(segments.begin(), segments.end(), SegmentComparator());
    
  BlendProc blendMode = gProcs[(int) paint.getBlendMode()];

  if (paint.getShader()) {
    GShader* sh = paint.getShader();

    if (sh->setContext(ctm[ctm.size() - 1])) {
      if (sh->isOpaque()) {
        if (blendMode == srcOverMode) blendMode = srcMode;
        if (blendMode == dstInMode) blendMode = dstMode;
        if (blendMode == srcATopMode) blendMode = srcInMode;
        if (blendMode == dstOutMode) blendMode = clearMode;
        if (blendMode == xorMode) blendMode = srcOutMode;
      }

      shade_fill_path(fDevice, segments, sh, blendMode);
    }

  } else {  
    GPixel src = color_to_pixel(paint.getColor());

    blendMode = simplify_blend_mode(paint, blendMode);

    fill_path(fDevice, segments, src, blendMode);
  }
}

 /**
         *  Draw a mesh of triangles, with optional colors and/or texture-coordinates at each vertex.
         *
         *  If both colors and texs[] are specified, then at each pixel their values are multiplied
         *  together, component by component.
         */

void MyCanvas::drawMesh(const GPoint verts[], const GColor colors[], const GPoint texs[],
              int count, const int indices[], const GPaint& paint) {

  GMatrix mat = ctm[ctm.size() - 1];
  GMatrix m;
  GPaint pnt;

  GPoint p[3];
  GColor c[3];
  GPoint t[3];

  BlendProc blendMode = gProcs[(int) paint.getBlendMode()];

  int n = 0;
  std::vector<Segment> segments;

  for (int i = 0; i < count; i++) {
    p[0] = verts[indices[n]];      
    p[1] = verts[indices[n+1]];      
    p[2] = verts[indices[n+2]];

    m = mat * GMatrix(p[1].x - p[0].x, p[2].x - p[0].x, p[0].x, p[1].y - p[0].y, p[2].y - p[0].y, p[0].y);

    clip_segment(fDevice, segments, p[0], p[1]);
    clip_segment(fDevice, segments, p[1], p[2]);
    clip_segment(fDevice, segments, p[2], p[0]);

    if (colors) {        // triangle gradient
      c[0] = colors[indices[n]];     
      c[1] = colors[indices[n+1]];     
      c[2] = colors[indices[n+2]];

      GShader* sh = GCreateTriangleGradient(p, c);

      if (texs) {
        t[0] = texs[indices[n]];       
        t[1] = texs[indices[n+1]];       
        t[2] = texs[indices[n+2]];

        GShader* orig = paint.getShader();

        auto proxy = GCreateProxyShader(verts, t, orig);
        auto compose = GCreateComposeShader(sh, proxy.get());
        GPaint pnt = GPaint(compose.get());
        
        drawConvexPolygon(p, 3, pnt);
      } else {
        GPaint pnt = GPaint(sh);
        drawConvexPolygon(p, 3, pnt);
      }

    } else if (texs) {
      t[0] = texs[indices[n]];       
      t[1] = texs[indices[n+1]];
      t[2] = texs[indices[n+2]];
    
      GShader* origShader = paint.getShader();

      auto sh = GCreateProxyShader(p, t, origShader);

      if (sh) {
        pnt.setShader(sh.get());
        
        drawConvexPolygon(p, 3, pnt);
      }
    } else {
      drawConvexPolygon(p, 3, paint);
    }

    n += 3;
  }
}

/**
         *  Draw the quad, with optional color and/or texture coordinate at each corner. Tesselate
         *  the quad based on "level":
         *      level == 0 --> 1 quad  -->  2 triangles
         *      level == 1 --> 4 quads -->  8 triangles
         *      level == 2 --> 9 quads --> 18 triangles
         *      ...
         *  The 4 corners of the quad are specified in this order:
         *      top-left --> top-right --> bottom-right --> bottom-left
         *  Each quad is triangulated on the diagonal top-right --> bottom-left
         *      0---1
         *      |  /|
         *      | / |
         *      |/  |
         *      3---2
         *
         *  colors and/or texs can be null. The resulting triangles should be passed to drawMesh(...).
         */
void MyCanvas::drawQuad(const GPoint verts[4], const GColor colors[4], const GPoint texs[4],
              int level, const GPaint& paint) {

  if (level == 0) {
    int indices[6] = {0, 1, 3, 1, 2, 3};
    drawMesh(verts, colors, texs, 6, indices, paint);                                                                                                                                
    return;
  }

  assert(level >= 1);

  int quadCount = pow(level + 1, 2);

  float incr = 1.0f / (level + 1);
  
  GPoint pts[quadCount * 4];
  GColor cols[quadCount * 4];
  GPoint texts[quadCount * 4];

  int indices[quadCount * 6];

  int c = 0;
  int idx = 0;
  float v = 0.0f;

  for (int i = 0; i <= level; i++) {
    float u = 0.0f;

    for (int j = 0; j <= level; j++) {
      pts[c]   = pt_weighted_avg(u,      v,      verts[0], verts[1], verts[2], verts[3]);
      pts[c+1] = pt_weighted_avg(u+incr, v,      verts[0], verts[1], verts[2], verts[3]);
      pts[c+2] = pt_weighted_avg(u,      v+incr, verts[0], verts[1], verts[2], verts[3]);
      pts[c+3] = pt_weighted_avg(u+incr, v+incr, verts[0], verts[1], verts[2], verts[3]);

      if (colors) {
        cols[c]   = col_weighted_avg(u,      v,      colors[0], colors[1], colors[2], colors[3]);
        cols[c+1] = col_weighted_avg(u+incr, v,      colors[0], colors[1], colors[2], colors[3]);
        cols[c+2] = col_weighted_avg(u,      v+incr, colors[0], colors[1], colors[2], colors[3]);
        cols[c+3] = col_weighted_avg(u+incr, v+incr, colors[0], colors[1], colors[2], colors[3]);

        // cols[c] = GColor::RGBA(1, 0, 0, 1);
        // cols[c+1] = GColor::RGBA(1, 0, 0, 1);
        // cols[c+2] = GColor::RGBA(1, 0, 0, 1);
        // cols[c+3] = GColor::RGBA(1, 0, 0, 1);
      }

      if (texs) {
        texts[c]   = pt_weighted_avg(u,      v,      texs[0], texs[1], texs[2], texs[3]);
        texts[c+1] = pt_weighted_avg(u+incr, v,      texs[0], texs[1], texs[2], texs[3]);
        texts[c+2] = pt_weighted_avg(u,      v+incr, texs[0], texs[1], texs[2], texs[3]);
        texts[c+3] = pt_weighted_avg(u+incr, v+incr, texs[0], texs[1], texs[2], texs[3]);
      }

      indices[idx] = c;
      indices[idx+1] = c+1;
      indices[idx+2] = c+2;
      indices[idx+3] = c+1;
      indices[idx+4] = c+2;
      indices[idx+5] = c+3;

      u += incr;
      c += 4;
      idx += 6;
    }

    v += incr;
  }

  if (colors) {
    texs ? drawMesh(pts, cols, texts, quadCount * 2, indices, paint) : drawMesh(pts, cols, nullptr, quadCount * 2, indices, paint);
  } else if (texs) {
    drawMesh(pts, nullptr, texts, quadCount * 2, indices, paint);
  } else {
    drawMesh(pts, nullptr, nullptr, quadCount * 2, indices, paint);
  }
}

  // drawing quad with cubic sides
    // Coon's patch: 
      // compute midpoint for AD and AB
      // compute midpoint as if quad has straight edges
      // treat three centers as error vectors
        // p + (p_v - p) + (p_u - p)
        // = p_u + p_v - p

/**
         *  Draw the quad, with optional color and/or texture coordinate at each corner. Tesselate
         *  the quad based on "level":
         *      level == 0 --> 1 quad  -->  2 triangles
         *      level == 1 --> 4 quads -->  8 triangles
         *      level == 2 --> 9 quads --> 18 triangles
         *      ...
         *  The 4 corners of the quad are specified in this order:
         *      top-left --> top-right --> bottom-right --> bottom-left
         *  Each quad is triangulated on the diagonal top-right --> bottom-left
         *      0---1
         *      |  /|
         *      | / |
         *      |/  |
         *      3---2
         *
         *  colors and/or texs can be null. The resulting triangles should be passed to drawMesh(...).
         */
void MyCanvas::drawCubicQuad(const GPoint verts[12], const GColor colors[4], const GPoint texs[4],
              int level, const GPaint& paint) {

  if (level == 0) {
    int indices[6] = {0, 1, 3, 1, 2, 3};

    // drawMesh(verts, colors, texs, 6, indices, paint);                                                                                                                                
    return;
  }

  assert(level >= 1);

  int quadCount = pow(level + 1, 2);

  float incr = 1.0f / (level + 1);
  
  GPoint pts[quadCount * 4];
  GColor cols[quadCount * 4];
  GPoint texts[quadCount * 4];

  int indices[quadCount * 6];

  int c = 0;
  int idx = 0;
  float v = 0.0f;

  for (int i = 0; i <= level; i++) {
    float u = 0.0f;

    for (int j = 0; j <= level; j++) {
      pts[c]   = pt_coons_avg(u,      v,      verts);
      pts[c+1] = pt_coons_avg(u+incr, v,      verts);
      pts[c+2] = pt_coons_avg(u,      v+incr, verts);
      pts[c+3] = pt_coons_avg(u+incr, v+incr, verts);

      // pts[c]   = pt_weighted_avg(u,      v,      verts[0], verts[3], verts[6], verts[9]);
      // pts[c+1] = pt_weighted_avg(u+incr, v,      verts[0], verts[3], verts[6], verts[9]);
      // pts[c+2] = pt_weighted_avg(u,      v+incr, verts[0], verts[3], verts[6], verts[9]);
      // pts[c+3] = pt_weighted_avg(u+incr, v+incr, verts[0], verts[3], verts[6], verts[9]);

      if (colors) {
        cols[c]   = col_weighted_avg(u,      v,      colors[0], colors[1], colors[2], colors[3]);
        cols[c+1] = col_weighted_avg(u+incr, v,      colors[0], colors[1], colors[2], colors[3]);
        cols[c+2] = col_weighted_avg(u,      v+incr, colors[0], colors[1], colors[2], colors[3]);
        cols[c+3] = col_weighted_avg(u+incr, v+incr, colors[0], colors[1], colors[2], colors[3]);
      }

      if (texs) {
        texts[c]   = pt_weighted_avg(u,       v,      texs[0], texs[1], texs[2], texs[3]);
        texts[c+1] = pt_weighted_avg(u+incr,  v,      texs[0], texs[1], texs[2], texs[3]);
        texts[c+2] = pt_weighted_avg(u,      v+incr,  texs[0], texs[1], texs[2], texs[3]);
        texts[c+3] = pt_weighted_avg(u+incr, v+incr,  texs[0], texs[1], texs[2], texs[3]);
      }

      indices[idx] = c;
      indices[idx+1] = c+1;
      indices[idx+2] = c+2;
      indices[idx+3] = c+1;
      indices[idx+4] = c+2;
      indices[idx+5] = c+3;

      u += incr;
      c += 4;
      idx += 6;
    }

    v += incr;
  }

  if (colors) {
    texs ? drawMesh(pts, cols, texts, quadCount * 2, indices, paint) : drawMesh(pts, cols, nullptr, quadCount * 2, indices, paint);
  } else if (texs) {
    drawMesh(pts, nullptr, texts, quadCount * 2, indices, paint);
  } else {
    drawMesh(pts, nullptr, nullptr, quadCount * 2, indices, paint);
  }

}

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
  return std::unique_ptr<GCanvas>(new MyCanvas(device));
}

std::string GDrawSomething(GCanvas* canvas, GISize dim) {
  // GColor bg = GColor({ 1, 1, 1, 1 });
  // canvas->clear(bg);

  // create the bitmap
  GBitmap bm;
  GPoint src[12] = {
    {50, 50}, 
      {100, 70}, {150, 60}, 
    {200, 50}, 
      {190, 100}, {210, 150},
    {200, 200}, 
      {160, 210}, {100, 220},
    {50, 200},
      {40, 170}, {30, 110}};

  GColor colors[4] = { 
      GColor::RGBA(1, .2, 0, 1), 
      GColor::RGBA(0, .5, .9, 1), 
      GColor::RGBA(.5, 0, .7, 1), 
      GColor::RGBA(0, .7, .6, 1) 
    }; 

    GBitmap bitmap;
    bitmap.readFromFile("apps/spock.png");
    const float w = bitmap.width();
    const float h = bitmap.height();

    auto shader = GCreateBitmapShader(bitmap, GMatrix(), GTileMode::kMirror);

    const GPoint texs[4] = {
            {w, 0}, {2*w, 0}, {2*w, h}, {w, h},
    };

  // for (int i = 0; i < 4; i++) {
  //   GPoint pts[4] = {{src[0].x, src[0].y}, {src[2].x, src[2].y}, {src[3].x, src[3].y}, {src[1].x, src[1].y}};
  //   

  //   for (int j = 0; j < 4; j++) {
  //     pts[j].x += 50;       pts[j].y += 50;
  //   }

    // create the shader, takes a bitmap and a local matrix (identity?)
    
  // }

  // GPoint quadPts[4] = { src[0], src[3], src[6], src[9] };

  // canvas->drawQuad(quadPts, colors, nullptr, 12, GPaint());
  canvas->drawCubicQuad(src, colors, nullptr, 12, GPaint());

  // auto shader = GCreateBitmapShader(bm, GMatrix());
  // GPaint paint(shader.get());

  // canvas->save();
  // canvas->scale(0.03f, 0.03f);

  // canvas->translate(cx * 2, cy * 2);

  // canvas->restore();

  return "gradient";
}