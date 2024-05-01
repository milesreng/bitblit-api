#include "include/GRect.h"
#include "include/GPixel.h"
#include "pointMath.h"
#include "Segment.h"
#include "path.h"
#include <iostream>

GIRect clip_rect(const GRect& rect, const GBitmap& bitmap) {
  int startX = std::max(GRoundToInt(rect.left), 0);
  int startY = std::max(GRoundToInt(rect.top), 0);
  int endX = std::min(GRoundToInt(rect.right), bitmap.width());
  int endY = std::min(GRoundToInt(rect.bottom), bitmap.height());

  return GIRect::LTRB(startX, startY, endX, endY);
}

// returns whether or not at least one segment was added
bool clip_segment(const GBitmap& bm, std::vector<Segment> &segments, GPoint p0, GPoint p1) {
  // eliminate horizontal segments
  if (GRoundToInt(p0.y) == GRoundToInt(p1.y)) return false;

  // std::cout << "clipping at " << p0.x << ", " << p0.y << " -> " << p1.x << ", " << p1.y << std::endl;

  bool swapped = false;

  if (p0.y > p1.y) {
    std::swap(p0, p1); 
    swapped = !swapped;
  }
  assert(p0.y < p1.y);

  // eliminate segments vertically out of bounds
  if (GRoundToInt(p1.y) <= 0 || GRoundToInt(p0.y) >= bm.height()) return false;

  if (is_segment_contained(bm, p0, p1)) {
    // std::cout << "inserting at " << p0.x << ", " << p0.y << " -> " << p1.x << ", " << p1.y << " swapped: " << swapped << std::endl;
    insert_segment(segments, p0, p1, swapped);
    return true; 
  }

  p0 = get_top_point(p0, p1, 0.0f);
  p1 = get_bottom_point(p0, p1, (float) bm.height());

  if (p0.x > p1.x) {
    std::swap(p0, p1);
    swapped = !swapped;
  }
  assert(p0.x <= p1.x);

  if (p0.x >= bm.width()) {
    // std::cout << "inserting at " << bm.width() << ", " << p0.y << " -> " << bm.width() << ", " << p1.y << " swapped: " << swapped << std::endl;
    insert_segment(segments, { (float) bm.width(), p0.y }, { (float) bm.width(), p1.y }, swapped);
  } else if (p1.x <= 0.0f) {
    // std::cout << "inserting at " << 0.0f << ", " << p0.y << " -> " << 0.0f << ", " << p1.y << " swapped: " << swapped << std::endl;
    insert_segment(segments, { 0.0f, p0.y }, { 0.0f, p1.y }, swapped);
  } else {
    GPoint left = get_left_point(p0, p1, 0.0f);
    GPoint right = get_right_point(p0, p1, (float) bm.width());

    if (GRoundToInt(left.y) != GRoundToInt(right.y)) insert_segment(segments, left, right, swapped);

    int winding = segments[segments.size() - 1].winding;

    if (winding > 0) {
      if (GRoundToInt(left.y) != GRoundToInt(p0.y)) segments.push_back(Segment(left, { left.x, p0.y }));
      if (GRoundToInt(right.y) != GRoundToInt(p1.y)) segments.push_back(Segment({ right.x, p1.y }, right));
    } else {
      if (GRoundToInt(left.y) != GRoundToInt(p0.y)) segments.push_back(Segment(left, { left.x, p0.y }));
      if (GRoundToInt(right.y) != GRoundToInt(p1.y)) segments.push_back(Segment(right, { right.x, p1.y }));
    }

  }

  return true;
}

void clip_quad_curve(const GBitmap& bm, std::vector<Segment> &segments, GPoint a, GPoint b, GPoint c) {
  float eX = (a.x - (2 * b.x) + c.x) / 4;
  float eY = (a.y - (2 * b.y) + c.y) / 4;
  float eLen = sqrt(eX * eX + eY * eY);

  int num_segs = (int) ceil(sqrt(eLen * 4));
  float dt = 1.0f / num_segs;

  GPoint prev = a;
  GPoint curr;
  GPoint src[3] = { a, b, c };
  GPoint dst[5];

  for (float i = 0.0f; i <= 1.0f; i += dt) {
    GPath::ChopQuadAt(src, dst, i);

    curr = dst[2];
    clip_segment(bm, segments, prev, curr);

    prev = curr;
  }
}

void clip_cubic_curve(const GBitmap& bm, std::vector<Segment> &segments, GPoint a, GPoint b, GPoint c, GPoint d) {
  GPoint e0 = a - (2 * b) + c;
  GPoint e1 = b - (2 * c) + d;

  GPoint e;
  e.x = std::max(abs(e0.x), abs(e1.x));
  e.y = std::max(abs(e0.y), abs(e1.y));
  
  float eLen = sqrt(e.x * e.x + e.y * e.y);

  int num_segs = (int) ceil(sqrt((3 * eLen) * 16));
  float dt = 1.0f / num_segs;

  GPoint prev = a;
  GPoint curr;
  GPoint src[4] = { a, b, c, d };
  GPoint dst[7];

  for (float i = 0.0f; i <= 1.0f; i += dt) {
    GPath::ChopCubicAt(src, dst, i);

    curr = dst[3];
    clip_segment(bm, segments, prev, curr);

    prev = curr;
  }
}

void pts_to_segments(const GBitmap& bm, std::vector<Segment> &segments, const GPoint* pts, int count) {
  for (int i = 0; i < count; i++) {
    int j = (i + 1) % count;
    clip_segment(bm, segments, pts[i], pts[j]);
  }
}

int clampX(const GBitmap& bm, float x) {
  if (x < 0.0f) return 0.0f;
  if (x >= bm.width()) return bm.width() - 1;

  return GRoundToInt(x);
}

int clampY(const GBitmap& bm, float y) {
  if (y < 0.0f) return 0.0f;
  if (y >= bm.height()) return bm.height() - 1;

  return GRoundToInt(y);
}


int mirrorX(const GBitmap& bm, float x) {
  x = x < 0.0f ? -x : x;

  int map = GRoundToInt(x) % (bm.width() * 2); 

  if (map >= bm.width()) return (bm.width() * 2) - map - 1;
  return map;
}

int mirrorY(const GBitmap& bm, float y) {
  y = y < 0.0f ? -y : y;

  int map = GRoundToInt(y) % (bm.height() * 2); 

  if (map >= bm.height()) return (bm.height() * 2) - map - 1;
  return map;
}

int repeatX(const GBitmap& bm, float x) { 
  int res;
  if (x < 0.0f) {
    res = bm.width() - (GRoundToInt(abs(x)) % bm.width()); 
  } else {
    res = GRoundToInt(x) % bm.width();
  }

  if (res == bm.width()) return bm.width() - 1;

  return res;
}

int repeatY(const GBitmap& bm, float y) { 
  int res;
  if (y < 0.0f) {
    res = bm.height() - (GRoundToInt(abs(y)) % bm.height()); 
  } else {
    res = GRoundToInt(y) % bm.height();
  }

  if (res == bm.height()) return bm.height() - 1;

  return res;
 }


GMatrix compute_basis(GPoint a, GPoint b, GPoint c) {
  return GMatrix(
    b.x - a.x,      c.x - a.x,      a.x,
    b.y - a.y,      c.y - a.y,      a.y
  );
}

GColor col_weighted_avg(float u, float v, GColor a, GColor b, GColor c, GColor d) {
  GColor col = (1 - u) * (1 - v) * a + u * (1 - v) * b + u * v * c + (1 - u) * v * d;
  col.r = col.r > 1.0f ? 1.0f : (col.r < 0.0f ? 0.0f : col.r);
  col.g = col.g > 1.0f ? 1.0f : (col.g < 0.0f ? 0.0f : col.g);
  col.b = col.b > 1.0f ? 1.0f : (col.b < 0.0f ? 0.0f : col.b);
  col.a = col.a > 1.0f ? 1.0f : (col.a < 0.0f ? 0.0f : col.a);

  return col;
}

GPoint pt_weighted_avg(float u, float v, GPoint a, GPoint b, GPoint c, GPoint d) {
  return (1 - u) * (1 - v) * a + u * (1 - v) * b + u * v * c + (1 - u) * v * d;
}

GColor col_coons_avg(float u, float v, GColor a, GColor b, GColor c, GColor d) {
  // GColor col = d * (1 - u) * v - b * (1 - v) * u - c * u * v;
  GColor col = (1 - u) * (1 - v) * a + u * (1 - v) * b + u * v * c + (1 - u) * v * d;
  col.r = col.r > 1.0f ? 1.0f : (col.r < 0.0f ? 0.0f : col.r);
  col.g = col.g > 1.0f ? 1.0f : (col.g < 0.0f ? 0.0f : col.g);
  col.b = col.b > 1.0f ? 1.0f : (col.b < 0.0f ? 0.0f : col.b);
  col.a = col.a > 1.0f ? 1.0f : (col.a < 0.0f ? 0.0f : col.a);

  return col;
}

GPoint pt_coons_avg(float u, float v, const GPoint pts[12]) {
  if (u == 0.0f && v == 0.0f) return pts[0];
  if (u == 1.0f && v == 0.0f) return pts[3];
  if (u == 1.0f && v == 1.0f) return pts[6];
  if (u == 0.0f && v == 1.0f) return pts[9];

  // −P(0, 1)(1 − µ)ν − P(1, 0)µ(1 − ν) − P(1, 1)µν
  GPoint top[4] = {pts[0], pts[1], pts[2], pts[3] };
  GPoint right[4] = {pts[3], pts[4], pts[5], pts[6] };
  GPoint bottom[4] = {pts[9], pts[8], pts[7], pts[6] };
  GPoint left[4] = {pts[0], pts[11], pts[10], pts[9] };

  if (u == 0.0f) return get_cubic_curve_point(left, v);
  if (u == 1.0f) return get_cubic_curve_point(right, v);
  if (v == 0.0f) return get_cubic_curve_point(top, u);
  if (v == 1.0f) return get_cubic_curve_point(bottom, u);

  GPoint a = get_cubic_curve_point(top, u);
  GPoint b = get_cubic_curve_point(bottom, u);

  GPoint ab = (a + b) * 0.5f;

  GPoint c = get_cubic_curve_point(left, v);
  GPoint d = get_cubic_curve_point(right, v);

  GPoint cd = (c + d) * 0.5f;

  GPoint mid = pt_weighted_avg(u, v, pts[0], pts[3], pts[6], pts[9]);

  return (ab + cd) - mid;
 
  // return (ab + cd + mid) * (1/3);
}