#include <iostream>
#include "canvas.h"
#include "colors.h"
#include "clip.h"

#include "include/GMatrix.h"
#include "include/GPath.h"

// duplicate transformation matrix at top of stack
void MyCanvas::save() {
  GMatrix dup = ctm[ctm.size() - 1];
  ctm.push_back(dup);
}

// erase transformation matrix at top of stack
void MyCanvas::restore() {
  ctm.erase(ctm.end() - 1);
}

// transform matrix at top of stack by supplied matrix
void MyCanvas::concat(const GMatrix& matrix) {
  GMatrix& top = ctm.back();
  GMatrix res = GMatrix::Concat(top, matrix);
  top = res;
}

// fills the entire bitmap with supplied color
void MyCanvas::clear(const GColor& color) {
  GPixel src = color_to_pixel(color);

  for (int y = 0; y < fDevice.height(); y++) {
    GPixel* pixel = fDevice.getAddr(0, y);
    for (int x = 0; x < fDevice.width(); x++) {
      pixel[x] = src;
    }
  }
}

// draws the given rectangle using the paint object
void MyCanvas::drawRect(const GRect& rect, const GPaint& paint) {
  GPoint p1 = { rect.left, rect.top };
  GPoint p2 = { rect.right, rect.top };
  GPoint p3 = { rect.right, rect.bottom };
  GPoint p4 = { rect.left, rect.bottom };

  GPoint pts[4] = { p1, p2, p3, p4 };

  drawConvexPolygon(pts, 4, paint);
}

void MyCanvas::drawConvexPolygon(const GPoint* pts, int count, const GPaint& paint) {
  if (count < 3) return;

  GMatrix mat = ctm[ctm.size() - 1];
  GPoint dst[count];

  // transform points in pts using mat
  mat.mapPoints(dst, pts, count);

  std::vector<Segment> segments;

  pts_to_segments(fDevice, segments, dst, count);
  if (segments.size() < 2) return;
}