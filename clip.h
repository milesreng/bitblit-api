#include "include/GBitmap.h"
#include "include/GPoint.h"
#include "Segment.h"

float compute_x(const GPoint& a, const GPoint& b, float y) {
  return a.x + (b.x - a.x) * (y - a.y) / (b.y - a.y);
}

float compute_y(const GPoint& a, const GPoint& b, float x) {
  return a.y + (b.y - a.y) * (x - a.x) / (b.x - a.x);
}

// Given two points where p0.y < p1.y, return the lesser y-value, clipped.
GPoint get_top_point(const GPoint& p0, const GPoint& p1, float bound) {
  if (GRoundToInt(p0.y) >= bound) { return p0; }
  return { compute_x(p0, p1, bound), bound };
}

// Given two points where p0.x < p1.x, return the lesser x-value, clipped.
GPoint get_left_point(const GPoint& p0, const GPoint& p1, float bound) {
  if (GRoundToInt(p0.x) >= bound) { return p0; }
  return { bound, compute_y(p0, p1, bound) };
}

// Given two points where p0.y < p1.y, return the greater y-value, clipped.
GPoint get_bottom_point(const GPoint& p0, const GPoint& p1, float bound) {
  if (GRoundToInt(p1.y) <= bound) { return p1; }
  return { compute_x(p0, p1, bound), bound };
}

// Given two points where p0.x < p1.x, return the greater x-value, clipped.
GPoint get_right_point(const GPoint& p0, const GPoint& p1, float bound) {
  if (GRoundToInt(p0.x) <= bound) { return p0; }
  return { bound, compute_y(p0, p1, bound) };
}

// Returns true iff the point is contained in the bitmap.
bool point_in_bm(const GBitmap& bm, const GPoint& p) {
  if (GRoundToInt(p.y) < 0 || GRoundToInt(p.y) >= bm.height()) return false;
  if (GRoundToInt(p.x) < 0 || GRoundToInt(p.x) >= bm.width()) return false;

  return true;
}

// Returns true iff the segment is fully contained in the bitmap.
bool segment_in_bm(const GBitmap& bm, const GPoint& a, const GPoint& b) {
  if (GRoundToInt(a.y) < 0 || GRoundToInt(b.y) >= bm.height()) return false;

  if (a.x <= b.x) { return !(a.x < 0.0f || b.x > bm.width()); }
  return !(b.x < 0.0f || a.x > bm.width());
}

void insert_segment(std::vector<Segment> &segments, const GPoint& p0, const GPoint& p1, bool swapped) {
  assert(GRoundToInt(p0.y) != GRoundToInt(p1.y));

  swapped ? segments.push_back(Segment(p1, p0)) : segments.push_back(Segment(p0, p1));
}

void clip_segment(const GBitmap& bm, std::vector<Segment> &segments, GPoint p0, GPoint p1) {
  if (GRoundToInt(p0.y) == GRoundToInt(p1.y)) return;

  // track the order of the segment endpoints
  bool swapped = false;

  if (p0.y > p1.y) {
    std::swap(p0, p1);
    swapped = !swapped;
  }

  assert(p0.y < p1.y);

  // eliminate segments that are vertically out of bounds
  if (GRoundToInt(p1.y) <= 0 || GRoundToInt(p0.y) >= bm.height()) return;

  if (segment_in_bm(bm, p0, p1)) {            // segment is fully contained in bitmap
    insert_segment(segments, p0, p1, swapped);
    return;
  }

  p0 = get_top_point(p0, p1, 0.0f);
  p1 = get_bottom_point(p0, p1, (float) bm.height());

  if (p0.x > p1.x) {
    std::swap(p0, p1);
    swapped = !swapped;
  }

  assert(p0.x <= p1.x);

  if (p0.x >= bm.width()) {
    insert_segment(segments, { (float) bm.width(), p0.y }, { (float) bm.width(), p1.y }, swapped);
  } else if (p1.x <= 0.0f) {
    insert_segment(segments, { 0.0f, p0.y }, { 0.0f, p1.y }, swapped);
  } else {
    GPoint left = get_left_point(p0, p1, 0.0f);
    GPoint right = get_right_point(p0, p1, (float) bm.width());

    if (GRoundToInt(left.y) != GRoundToInt(right.y)) {
      insert_segment(segments, left, right, swapped);
    }

    int winding = segments[segments.size() - 1].winding;

    if (winding > 0) {
      if (GRoundToInt(left.y) != GRoundToInt(p0.y)) segments.push_back(Segment(left, { left.x, p0.y }));
      if (GRoundToInt(right.y) != GRoundToInt(p1.y)) segments.push_back(Segment({ right.x, p1.y }, right));
    } else {
      if (GRoundToInt(left.y) != GRoundToInt(p0.y)) segments.push_back(Segment(left, { left.x, p0.y }));
      if (GRoundToInt(right.y) != GRoundToInt(p1.y)) segments.push_back(Segment(right, { right.x, p1.y }));
    }
  }
}

/**
 * Given an ordered list of points, convert adjacent points to line segments and 
 * clip based on bitmap bounds
*/
void pts_to_segments(const GBitmap& bm, std::vector<Segment> &segments, const GPoint* pts, int count) {
  for (int i = 0; i < count; i++) {
    int j = (i + 1) % count;
    clip_segment(bm, segments, pts[i], pts[j]);
  }
}