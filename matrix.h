#include "include/GMatrix.h"
#include <iostream>

GMatrix::GMatrix() {
  fMat[0] = 1.0f;     fMat[2] = 0.0f;     fMat[4] = 0.0f;
  fMat[1] = 0.0f;     fMat[3] = 1.0f;     fMat[5] = 0.0f;
}

GMatrix GMatrix::Translate(float tx, float ty) { 
  // [ 1   0  t_x] [x]   [x + t_y]
  // [ 0   1  t_y] [y] = [y + t_y]
  // [ 0   0   1 ] [1]
  return GMatrix(1.0f, 0.0f, tx, 0.0f, 1.0f, ty);
}

GMatrix GMatrix::Scale(float sx, float sy) {
  // [s_x  0   0] [x]   [s_x * x]
  // [ 0  s_y  0] [y] = [s_y * y]
  // [ 0   0   1] [1]
  return GMatrix(sx, 0.0f, 0.0f, 0.0f, sy, 0.0f);
}

GMatrix GMatrix::Rotate(float radians) {
  float a = cos(radians);
  float b = sin(radians);

  // [cos0  -sin0   0] [x]
  // [sin0   cos0   0] [y]
  // [ 0       0    1] [1]

  // x' = x * cos0 - y * sin0
  // y' = x * sin0 + y * cos0
  //    = x * (cos0, sin0) + y * (-sin0, cos0)

  return GMatrix(a, -b, 0.0f, b, a, 0.0f);
}

GMatrix GMatrix::Concat(const GMatrix& a, const GMatrix& b) {

  // aa * ba + ac * bb
  float ma = a[0] * b[0] + a[2] * b[1];
  // ab * ba + ad * bb
  float mb = a[1] * b[0] + a[3] * b[1];
  // aa * bc + ac * bd
  float mc = a[0] * b[2] + a[2] * b[3];
  // ab * bc + ad * bd
  float md = a[1] * b[2] + a[3] * b[3];
  // aa * be + ac * bf + ae
  float me = a[0] * b[4] + a[2] * b[5] + a[4];
  // ab * be + ad * bf + af
  float mf = a[1] * b[4] + a[3] * b[5] + a[5];

  return GMatrix(ma, mc, me, mb, md, mf);
}

// can return {} or GMatrix object
std::optional<GMatrix> GMatrix::invert() const {
  GMatrix curr = (*this);

  // interpret as area of parallelogram
  float det = curr[0] * curr[3] - curr[1] * curr[2];

  if (det == 0.0f) return {};

  float invdet = 1.0f / det;

  float ma = curr[3] * invdet;
  float mb = - curr[1] * invdet;
  float mc = - curr[2] * invdet;
  float md = curr[0] * invdet;
  float me = (curr[2] * curr[5] - curr[3] * curr[4]) * invdet;
  float mf = (curr[1] * curr[4] - curr[0] * curr[5]) * invdet;

  return GMatrix(ma, mc, me, mb, md, mf);
}

void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const {
  GMatrix curr = (*this);

  for (int i = 0; i < count; i++) {
    float x = src[i].x * curr[0] + src[i].y * curr[2] + curr[4];
    float y = src[i].x * curr[1] + src[i].y * curr[3] + curr[5];

    dst[i] = { x, y };
  }
}
