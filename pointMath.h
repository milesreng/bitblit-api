#include "include/GPoint.h"

float compute_x(const GPoint& a, const GPoint& b, float y) {
  return a.x + (b.x - a.x) * (y - a.y) / (b.y - a.y);
}

float compute_y(const GPoint& a, const GPoint& b, float x) {
  return a.y + (b.y - a.y) * (x - a.x) / (b.x - a.x);
}

float compute_t_from_y(const GPoint& a, const GPoint& b, const GPoint& c, float y) {
  float inner = -2 * a.y * c.y + a.y * y + b.y * b.y - 2 * b.y * y + 2 * c.y * y;

  float denom = a.y - 2 * b.y + 2 * c.y;

  float t = ((a.y - b.y) + inner) / denom;

  if (t < 0.0f) {
    t = ((a.y - b.y) - inner) / denom;
  }

  assert(t >= 0.0f && t <= 1.0f);

  return t;
}

// given two points and a y-boundary, return the lesser y-value, clipped
// THIS RETURNS THE CORRECT VALUE AS LONG AS THE ASSERT IS TRUE
GPoint get_top_point(const GPoint& p0, const GPoint& p1, float bound) {
  assert(p0.y < p1.y);

  if (GRoundToInt(p0.y) >= bound) {
    return p0;
  }

  return { compute_x(p0, p1, bound), bound };
}

// given two points and a y-boundary, return the greater y-value, clipped
GPoint get_bottom_point(const GPoint& p0, const GPoint& p1, float bound) {
  assert(p0.y < p1.y);

  if (GRoundToInt(p1.y) <= bound) {
    return p1;
  }

  return { compute_x(p0, p1, bound), bound };
}

// given two points and an x-boundary, return the lesser x-value, clipped
GPoint get_left_point(const GPoint& p0, const GPoint& p1, float bound) {
  assert(p0.x <= p1.x);         // it is possible to have vertical lines

  if (GRoundToInt(p0.x) >= bound) {
    return p0;
  }

  return { bound, compute_y(p0, p1, bound) };
}

GPoint get_right_point(const GPoint& p0, const GPoint& p1, float bound) {
  assert(p0.x <= p1.x);

  if (GRoundToInt(p1.x) <= bound) {
    return p1;
  }

  return { bound, compute_y(p0, p1, bound) };
}