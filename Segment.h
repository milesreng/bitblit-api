#ifndef Segment_DEF
#define Segment_DEF

#include "include/GPoint.h"
#include "include/GBitmap.h"
#include "include/GMath.h"
#include <iostream>
#include <vector>

/**
  * Defines a line segment given two points, with a winding value
  * indicating the "direction" of the line segment
**/

struct Segment {
  int top, bottom;
  float m, x;
  int winding;

  Segment() {}

  Segment(const GPoint& p1, const GPoint& p2) {
    top = p1.y < p2.y ? GRoundToInt(p1.y) : GRoundToInt(p2.y);
    bottom = p1.y < p2.y ? GRoundToInt(p2.y) : GRoundToInt(p1.y);

    winding = p1.y < p2.y ? 1 : -1;

    assert(top >= 0 && bottom > top);

    m = (p2.x - p1.x) / (p2.y - p1.y);
    float b = p1.x - (m * p1.y);

    x = m * (top + 0.5f) + b;
  }

  bool operator< (const Segment& line) const { return top > line.top; }

  bool isInbounds(int y) { return y >= top && y < bottom; }

  int getIntersect() { return GRoundToInt(x); }

  int nextIntersect() {
    int currX = GRoundToInt(x);
    x += m;
    return currX;
  }
};

#endif