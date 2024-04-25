#include "include/GColor.h"
#include "include/GRect.h"
#include "blendModes.h"
#include "shader.h"
#include <iostream>

typedef GPixel(*BlendProc) (GPixel, GPixel);

const BlendProc gProcs[] = {
  clearMode, srcMode, dstMode, srcOverMode, dstOverMode, srcInMode, dstInMode, 
  srcOutMode, dstOutMode, srcATopMode, dstATopMode, xorMode
};

BlendProc simplify_blend_mode(const GPaint& paint, BlendProc blendMode) {
  if (paint.getAlpha() == 1.0f) {
      if (blendMode == srcOverMode) return srcMode;
      if (blendMode == dstInMode) return dstMode;
      if (blendMode == srcATopMode) return srcInMode;
      if (blendMode == dstOutMode) return clearMode;
      if (blendMode == xorMode) return srcOutMode;
    } else if (paint.getAlpha() == 0.0f) {
      if (blendMode == srcMode) return clearMode;
      if (blendMode == srcOverMode) return dstMode;
      if (blendMode == dstOverMode) return dstMode;
      if (blendMode == srcInMode) return clearMode;
      if (blendMode == dstInMode) return clearMode;
      if (blendMode == srcOutMode) return clearMode;
      if (blendMode == dstOutMode) return dstMode;
      if (blendMode == srcATopMode) return dstMode;
      if (blendMode == dstATopMode) return clearMode;
      if (blendMode == xorMode) return dstMode;
  }
  return blendMode;
}

template<typename Proc> void blend_shader_row(const GBitmap& bm, const GPixel row[], int x, int y, int width, Proc blend) {
  auto pixel = bm.getAddr(x, y);  

  if (blend == srcMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = srcMode(row[i], pixel[i]);
    }
  } else if (blend == dstMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = dstMode(row[i], pixel[i]);
    }
  } else if (blend == srcOverMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = srcOverMode(row[i], pixel[i]);
    }
  } else if (blend == dstOverMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = dstOverMode(row[i], pixel[i]);
    }
  } else if (blend == srcInMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = srcInMode(row[i], pixel[i]);
    }
  } else if (blend == dstInMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = dstInMode(row[i], pixel[i]);
    }
  } else if (blend == srcOutMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = srcOutMode(row[i], pixel[i]);
    }
  } else if (blend == dstOutMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = dstOutMode(row[i], pixel[i]);
    }
  } else if (blend == srcATopMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = srcATopMode(row[i], pixel[i]);
    }
  } else if (blend == dstATopMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = dstATopMode(row[i], pixel[i]);
    }
  } else if (blend == xorMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = xorMode(row[i], pixel[i]);
    }
  } else if (blend == clearMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = clearMode(row[i], pixel[i]);
    }
  } else {
    for (int i = 0; i < width; i++) {
      pixel[i] = blend(row[i], pixel[i]);
    }
  }
}

template<typename Proc> void blend_row(const GBitmap& bm, const GPixel& src, int x, int y, int width, Proc blend) {
  auto pixel = bm.getAddr(x, y);  

  if (blend == srcMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = srcMode(src, pixel[i]);
    }
  } else if (blend == dstMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = dstMode(src, pixel[i]);
    }
  } else if (blend == srcOverMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = srcOverMode(src, pixel[i]);
    }
  } else if (blend == dstOverMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = dstOverMode(src, pixel[i]);
    }
  } else if (blend == srcInMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = srcInMode(src, pixel[i]);
    }
  } else if (blend == dstInMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = dstInMode(src, pixel[i]);
    }
  } else if (blend == srcOutMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = srcOutMode(src, pixel[i]);
    }
  } else if (blend == dstOutMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = dstOutMode(src, pixel[i]);
    }
  }
   else if (blend == srcATopMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = srcATopMode(src, pixel[i]);
    }
  }
   else if (blend == dstATopMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = dstATopMode(src, pixel[i]);
    }
  }
   else if (blend == xorMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = xorMode(src, pixel[i]);
    }
  } else if (blend == clearMode) {
    for (int i = 0; i < width; i++) {
      pixel[i] = clearMode(src, pixel[i]);
    }
  } else {
    for (int i = 0; i < width; i++) {
      pixel[i] = blend(src, pixel[i]);
    }
  }
}

// HANDLE COLORS

struct GPremulColor {
  int r, g, b, a;

  static GPremulColor RGBA(int r, int g, int b, int a) {
    return { r, g, b, a };
  }
};

GPremulColor premul(const GColor& color) {
  int r = GRoundToInt(color.r * color.a * 255);
  int g = GRoundToInt(color.g * color.a * 255);
  int b = GRoundToInt(color.b * color.a * 255);
  int a = GRoundToInt(color.a * 255);

  return GPremulColor::RGBA(r, g, b, a);
}

GPixel color_to_pixel(const GColor& color) {
  GPremulColor prem = premul(color);
  return GPixel_PackARGB(prem.a, prem.r, prem.g, prem.b);
}

// DRAW SHAPES

void blend_sect(const GIRect sect, const GBitmap& bm, const GPixel& src, BlendProc blend) {
  if (sect.left >= bm.width()) return;

  for (int y = sect.top; y < sect.bottom; y++) {
    blend_row(bm, src, sect.left, y, sect.width(), blend);
  }
}

void fill_convex_polygon(const GBitmap& bm, std::vector<Segment> &segments, GPixel src, BlendProc blend) {
  Segment& a = segments[segments.size() - 1];
  Segment& b = segments[segments.size() - 2];

  bool isALeft = a.x < b.x;

  for (int y = a.top; y < bm.height(); y++) {
    if (!b.isInbounds(y)) {
      segments.erase(segments.end() - 2);
      if (segments.size() <= 1) return;

      b = segments[segments.size() - 2];

      isALeft = a.x < b.x;
    }

    if (!a.isInbounds(y)) {
      segments.erase(segments.end() - 1);

      if (segments.size() <= 0) return;

      a = segments[segments.size() - 2];

      isALeft = a.x < b.x;
    }

    // assert(a.isInbounds(y));
    // assert(b.isInbounds(y));

    int ax = a.nextIntersect();
    int bx = b.nextIntersect();

    int start = isALeft ? ax : bx;
    int end = isALeft ? bx : ax;

    // assert(start >= 0 && end >= start);

    if (start < bm.width()) blend_row(bm, src, start, y, (end - start), blend);
  }
}

void shade_fill_convex_polygon(const GBitmap& bm, std::vector<Segment> &segments, GShader* sh, BlendProc blend) {

  Segment& a = segments[segments.size() - 1];
  Segment& b = segments[segments.size() - 2];

  bool isALeft = a.x < b.x;

  for (int y = a.top; y < bm.height(); y++) {
    if (!b.isInbounds(y)) {
      segments.erase(segments.end() - 2);
      if (segments.size() <= 1) return;

      b = segments[segments.size() - 2];

      isALeft = a.x < b.x;
    }

    if (!a.isInbounds(y)) {
      segments.erase(segments.end() - 1);

      if (segments.size() <= 0) return;

      a = segments[segments.size() - 2];

      isALeft = a.x < b.x;
    }

    // assert(a.isInbounds(y));
    // assert(b.isInbounds(y));

    int ax = a.nextIntersect();
    int bx = b.nextIntersect();

    int start = isALeft ? ax : bx;
    int end = isALeft ? bx : ax;

    // assert(start >= 0 && end >= start);

    if (start < bm.width()) {
      GPixel row[end - start];
      sh->shadeRow(start, y, (end - start), row);
      blend_shader_row(bm, row, start, y, (end - start), blend);
    }
  }
}

struct SegmentComparator {
  bool operator() (const Segment& a, const Segment& b) const {
    if (a.top != b.top) return a.top > b.top;

    return a.x > b.x;
  }
};

struct XComparator {
  bool operator() (const Segment& p0, const Segment& p1) const { return p0.x > p1.x; }
};

void fill_path(const GBitmap& bm, std::vector<Segment> segments, const GPixel& src, BlendProc blend) {
  assert(segments.size() > 0);

  int yMin = segments[segments.size() - 1].top;

  for (int y = yMin; y < bm.height(); y++) {
    if (segments.size() == 0) break;

    size_t i = 0;
    int l = 0;
    int r = 0;
    int fill = 0;

    Segment* e = &(segments[segments.size() - i - 1]);

    while (i < segments.size() && e->isInbounds(y)) {
      int x = GRoundToInt(e->getIntersect());

      if (fill == 0) l = x;

      fill += e->winding;

      if (fill == 0 && l < bm.width()) {
        r = x;
        blend_row(bm, src, l, y, (r - l), blend);
      }

      if (e->isInbounds(y + 1)) {
        e->nextIntersect();
        i += 1;
      } else {
        segments.erase(segments.end() - i - 1);
      }

      e = &(segments[segments.size() - i - 1]);
    }

    // assert(fill == 0);

    if (segments.size() == 0) break;

    while (i < segments.size() && segments[segments.size() - i - 1].isInbounds(y + 1)) {
      i++;
    }

    std::sort(segments.end() - i, segments.end(), XComparator());
  }
}

void shade_fill_path(const GBitmap& bm, std::vector<Segment> segments, GShader* sh, BlendProc blend) {
  assert(segments.size() > 0);

  int yMin = segments[segments.size() - 1].top;

  for (int y = yMin; y < bm.height(); y++) {
    if (segments.size() == 0) break;

    size_t i = 0;
    int l = 0;
    int r = 0;
    int fill = 0;

    Segment* e = &(segments[segments.size() - i - 1]);

    while (i < segments.size() && e->isInbounds(y)) {
      int x = GRoundToInt(e->getIntersect());

      if (fill == 0) l = x;

      fill += e->winding;

      if (fill == 0 && l < bm.width()) {
        r = x;
        GPixel row[r - l];
        sh->shadeRow(l, y, (r - l), row);
        blend_shader_row(bm, row, l, y, (r - l), blend);
      }

      if (e->isInbounds(y + 1)) {
        e->nextIntersect();
        i += 1;
      } else {
        segments.erase(segments.end() - i - 1);
      }

      e = &(segments[segments.size() - i - 1]);
    }

    // assert(fill == 0);

    if (segments.size() == 0) break;

    while (i < segments.size() && segments[segments.size() - i - 1].isInbounds(y + 1)) {
      i++;
    }

    std::sort(segments.end() - i, segments.end(), XComparator());
  }
}

