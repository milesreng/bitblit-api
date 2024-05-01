#include "include/GPath.h"

void GPath::addRect(const GRect& r, Direction dir) {
  GPoint p0 = { r.left, r.top };
  GPoint p1 = { r.right, r.top };
  GPoint p2 = { r.right, r.bottom };
  GPoint p3 = { r.left, r.bottom };

  GPath::moveTo(p0);

  if (dir == kCW_Direction) {
    GPath::lineTo(p1);    GPath::lineTo(p2);    GPath::lineTo(p3);
  } else {
    GPath::lineTo(p3);    GPath::lineTo(p2);    GPath::lineTo(p1);
  }
}

void GPath::addPolygon(const GPoint* pts, int count) {
  if (count < 3) return;

  GPath::moveTo(pts[0]);

  for (int i = 1; i < count; i++) {
    GPath::lineTo(pts[i]);
  }
}

void compute_unit_circle(GPoint circle[]) {
  circle[0] = {0.0f, 1.0f};

  circle[1] = {0.41421, 1.0f};
  circle[2] = {1.0f/sqrtf(2), 1.0f/sqrtf(2)};
  circle[3] = {circle[1].y, circle[1].x};

  circle[4] = {1.0f, 0.0f};

  circle[5] = {circle[3].x, -circle[3].y};
  circle[6] = {circle[2].x, -circle[2].y};
  circle[7] = {circle[1].x, -circle[1].y};

  circle[8] = {0.0f, -1.0f};

  circle[9]  = {-circle[7].x, circle[7].y};
  circle[10] = {-circle[6].x, circle[6].y};
  circle[11] = {-circle[5].x, circle[5].y};

  circle[12] = {-1.0f, 0.0f};

  circle[13] = {circle[11].x, -circle[11].y};
  circle[14] = {circle[10].x, -circle[10].y};
  circle[15] = {circle[9].x, -circle[9].y};

}

void GPath::addCircle(const GPoint center, float radius, Direction dir) {
  // Append a new contour respecting the Direction. The contour should be an approximate
  // circle (8 quadratic curves will suffice) with the specified center and radius.

  GPoint circle[16];
  compute_unit_circle(circle);

  GMatrix mat = GMatrix::Translate(center.x, center.y) * GMatrix::Scale(radius, radius);

  for (int i = 0; i < 16; i++) {
    circle[i] = mat * circle[i];
  }

  GPath::moveTo(circle[0]);

  // build the unit circle using 8 quadratic curves
  if (dir == kCCW_Direction) {
    GPath::quadTo(circle[1], circle[2]);      GPath::quadTo(circle[3], circle[4]);
    GPath::quadTo(circle[5], circle[6]);      GPath::quadTo(circle[7], circle[8]);
    GPath::quadTo(circle[9], circle[10]);     GPath::quadTo(circle[11], circle[12]);
    GPath::quadTo(circle[13], circle[14]);    GPath::quadTo(circle[15], circle[0]);
  } else { 
    GPath::quadTo(circle[15], circle[14]);    GPath::quadTo(circle[13], circle[12]);
    GPath::quadTo(circle[11], circle[10]);    GPath::quadTo(circle[9], circle[8]);
    GPath::quadTo(circle[7], circle[6]);      GPath::quadTo(circle[5], circle[4]);
    GPath::quadTo(circle[3], circle[2]);      GPath::quadTo(circle[1], circle[0]);
  }
}

GPoint get_quad_curve_point(const GPoint* pts, float t) {
  float x = pts[0].x * pow(1-t, 2) + 2 * pts[1].x * t * (1-t) + pts[2].x * t * t;
  float y = pts[0].y * pow(1-t, 2) + 2 * pts[1].y * t * (1-t) + pts[2].y * t * t;

  return { x, y };
}

GPoint get_cubic_curve_point(const GPoint* pts, float t) {
  float x = pts[0].x * pow(1-t, 3) + 3 * pts[1].x * t * pow(1-t, 2) + 3 * pts[2].x * (1-t) * t * t + pts[3].x * pow(t, 3);
  float y = pts[0].y * pow(1-t, 3) + 3 * pts[1].y * t * pow(1-t, 2) + 3 * pts[2].y * (1-t) * t * t + pts[3].y * pow(t, 3);

  return { x, y };
}

GRect GPath::bounds() const {

  if (fPts.size() < 2) return GRect::WH(0, 0);

  float l = fPts[0].x;        float r = fPts[0].x;
  float t = 0.0f;             float b = fPts[0].y;

  float tVal;
  GPoint tan;
  GPoint pts[4];
  GPath::Edger iter(*this);

  while (auto v = iter.next(pts)) {
    switch (v.value()) {
      case GPath::kLine: // pts[0..1]

        if (l > pts[1].x) l = pts[1].x;     if (r < pts[1].x) r = pts[1].x;
        if (t > pts[1].y) t = pts[1].y;     if (b < pts[1].y) b = pts[1].y;

        break;

      case GPath::kQuad: // pts[0..2]

        if (l > pts[2].x) l = pts[2].x;     if (r < pts[2].x) r = pts[2].x;
        if (t > pts[2].y) t = pts[2].y;     if (b < pts[2].y) b = pts[2].y;

        // t = (A - B) / (A - 2B + C)
        tVal = (pts[0].x - pts[1].x) / (pts[0].x - 2 * pts[1].x + pts[2].x);

        // assert(tVal >= 0.0f && tVal <= 1.0f);

        tan = get_quad_curve_point(pts, tVal);

        if (l > tan.x) l = tan.x;           if (r < tan.x) r = tan.x;
        if (t > tan.y) t = tan.y;           if (b < tan.y) b = tan.y;

        break;

      case GPath::kCubic: // pts[0..3]

        if (l > pts[3].x) l = pts[3].x;     if (r < pts[3].x) r = pts[3].x;
        if (t > pts[3].y) t = pts[3].y;     if (b < pts[3].y) b = pts[3].y;

        // t = (A - B) / (A - 2B + C)
        tVal = (pts[0].x - pts[1].x) / (pts[0].x - 2 * pts[1].x + pts[2].x);

        // assert(tVal >= 0.0f && tVal <= 1.0f);

        tan = get_cubic_curve_point(pts, tVal);

        if (l > tan.x) l = tan.x;           if (r < tan.x) r = tan.x;
        if (t > tan.y) t = tan.y;           if (b < tan.y) b = tan.y;

        break;

      default:
        break;
    }
  }

  return GRect::LTRB(l, t, r, b);
}

void GPath::transform(const GMatrix& m) {
  for (int i = 0; i < GPath::countPoints(); i++) {
    GPoint* p = &(fPts[i]);
    *p = m * (*p);
  }
}

/**
     *  Given 0 < t < 1, subdivide the src[] quadratic bezier at t into two new quadratics in dst[]
     *  such that
     *  0...t is stored in dst[0..2]
     *  t...1 is stored in dst[2..4]
     */
void GPath::ChopQuadAt(const GPoint src[3], GPoint dst[5], float t) {
  // assert(t >= 0.0f && t <= 1.0f);

  // Q(t)       =   A       (1 - t)   ^  2     +  2     B      t   (1 - t)  +  C      t   ^ 2
  GPoint q = src[0] + (src[1] - src[0]) * t;
  GPoint r = src[1] + (src[2] - src[1]) * t;
  GPoint s = q + (r - q) * t;

  dst[0] = src[0];
  dst[1] = q;
  dst[2] = s;
  dst[3] = r;
  dst[4] = src[2];

}

    /**
     *  Given 0 < t < 1, subdivide the src[] cubic bezier at t into two new cubics in dst[]
     *  such that
     *  0...t is stored in dst[0..3]
     *  t...1 is stored in dst[3..6]
     */
void GPath::ChopCubicAt(const GPoint src[4], GPoint dst[7], float t) {
  // assert(t >= 0.0f && t <= 1.0f);

  GPoint chopAt = get_cubic_curve_point(src, t);

  GPoint f = src[2] * t + src[1] * (1-t);
  GPoint g = src[3] * t + src[2] * (1-t);

  dst[0] = src[0];
  dst[1] = src[1] * t + src[0] * (1-t);
  dst[2] = f * t + dst[1] * (1-t);
  dst[3] = chopAt;
  dst[4] = g * t + f * (1-t);
  dst[5] = g;
  dst[6] = src[3];
}