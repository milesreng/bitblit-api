#ifndef Edge_DEF
#define Edge_DEF

#include "include/GPoint.h"
#include "include/GBitmap.h"
#include "include/GMath.h"
#include <iostream>
#include <vector>

struct Segment {
  int top, bottom;
  float m, x;
  int winding;

  Segment() {}

  Segment(const GPoint& p1, const GPoint& p2) {
    top = p1.y < p2.y ? GRoundToInt(p1.y) : GRoundToInt(p2.y);
    bottom = p1.y < p2.y ? GRoundToInt(p2.y) : GRoundToInt(p1.y);

    winding = p1.y < p2.y ? 1 : -1;

    // we don't want horizontal lines
    assert(top >= 0 && bottom > top);

    m = (p2.x - p1.x) / (p2.y - p1.y);
    float b = p1.x - (m * p1.y);

    x = m * (top + 0.5f) + b;
  }

  bool operator< (const Segment& line) const { return top > line.top; }

  bool isInbounds(int y) {
    return y >= top && y < bottom;
  }

  int getIntersect() {
    return GRoundToInt(x);
  }

  int nextIntersect() {
    int currX = GRoundToInt(x);
    x += m;
    return currX;
  }
};

bool is_point_contained(const GBitmap& bm, const GPoint& p) {
  if (GRoundToInt(p.y) < 0 || GRoundToInt(p.y) >= bm.height()) return false;
  if (GRoundToInt(p.x) < 0 || GRoundToInt(p.x) >= bm.width()) return false;

  return true;
}

// returns true if we do not need to clip the segment at all
bool is_segment_contained(const GBitmap& bm, const GPoint& a, const GPoint& b) {
  // we know that point a is the "top" point
  if (GRoundToInt(a.y) < 0 || GRoundToInt(b.y) >= bm.height()) return false;

  if (a.x <= b.x && (a.x < 0.0f || b.x > bm.width())) {
    return false;
  } else if (a.x > b.x && (b.x < 0.0f || a.x > bm.width())) {
    return false;
  }

  // if x and y values both in bitmap, the segment is contained
  return true;
}

void insert_segment(std::vector<Segment> &segments, const GPoint& p0, const GPoint& p1, bool swapped) {
  assert(GRoundToInt(p0.y) != GRoundToInt(p1.y));

  if (swapped) {
    segments.push_back(Segment(p1, p0));
  } else {
    segments.push_back(Segment(p0, p1));
  }
}

#endif