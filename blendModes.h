#include "include/GPixel.h"

static inline uint8_t GDiv255(unsigned prod) {
  return (prod + 128) * 257 >> 16;
}

int abs(int x) {
  int m = x >> 31;
  return (x ^ m) - m;
}

int computeOver(int src, int dst, int alpha) {
  return src + GDiv255((255 - alpha) * dst);
}

int computeIn(int src, int dstAlpha) {
  return GDiv255(src * dstAlpha);
}

int computeOut(int src, int dstAlpha) {
  return GDiv255((255 - dstAlpha) * src);
}

int computeATop(int src, int dst, int srcAlpha, int dstAlpha) {
  return GDiv255(dstAlpha * src + (255 - srcAlpha) * dst);
}

int computeXor(int src, int dst, int srcAlpha, int dstAlpha) {
  return GDiv255((255 - srcAlpha) * dst + (255 - dstAlpha) * src);
}

// kClear,    //!<     0
GPixel clearMode(const GPixel src, const GPixel dst) {
  return GPixel_PackARGB(0, 0, 0, 0);
}

// kSrc,      //!<     S
GPixel srcMode(const GPixel src, const GPixel dst) {
  return src;
}

// kDst,      //!<     D
GPixel dstMode(const GPixel src, const GPixel dst) {
  return dst;
}

// kSrcOver,  //!<     S + (1 - Sa)*D
GPixel srcOverMode(const GPixel src, const GPixel dst) {
  int srcAlpha = GPixel_GetA(src);

  int a = computeOver(srcAlpha, GPixel_GetA(dst), srcAlpha);
  int r = computeOver(GPixel_GetR(src), GPixel_GetR(dst), srcAlpha);
  int g = computeOver(GPixel_GetG(src), GPixel_GetG(dst), srcAlpha);
  int b = computeOver(GPixel_GetB(src), GPixel_GetB(dst), srcAlpha);

  return GPixel_PackARGB(a, r, g, b);
}

// kDstOver,  //!<     D + (1 - Da) * S
GPixel dstOverMode(const GPixel src, const GPixel dst) {
  int dstAlpha = GPixel_GetA(dst);

  int a = computeOver(dstAlpha, GPixel_GetA(src), dstAlpha);
  int r = computeOver(GPixel_GetR(dst), GPixel_GetR(src), dstAlpha);
  int g = computeOver(GPixel_GetG(dst), GPixel_GetG(src), dstAlpha);
  int b = computeOver(GPixel_GetB(dst), GPixel_GetB(src), dstAlpha);

  return GPixel_PackARGB(a, r, g, b);
}

// kSrcIn,    //!<     Da * S
GPixel srcInMode(const GPixel src, const GPixel dst) {
  int dstAlpha = GPixel_GetA(dst);

  int a = computeIn(GPixel_GetA(src), dstAlpha);
  int r = computeIn(GPixel_GetR(src), dstAlpha);
  int g = computeIn(GPixel_GetG(src), dstAlpha);
  int b = computeIn(GPixel_GetB(src), dstAlpha);

  return GPixel_PackARGB(a, r, g, b);
}

 // kDstIn,    //!<     Sa * D
GPixel dstInMode(const GPixel src, const GPixel dst) {
  int srcAlpha = GPixel_GetA(src);

  int a = computeIn(GPixel_GetA(dst), srcAlpha);
  int r = computeIn(GPixel_GetR(dst), srcAlpha);
  int g = computeIn(GPixel_GetG(dst), srcAlpha);
  int b = computeIn(GPixel_GetB(dst), srcAlpha);

  return GPixel_PackARGB(a, r, g, b);
}

// kSrcOut,   //!<     (1 - Da)*S
GPixel srcOutMode(const GPixel src, const GPixel dst) {
  int dstAlpha = GPixel_GetA(dst);

  int a = computeOut(GPixel_GetA(src), dstAlpha);
  int r = computeOut(GPixel_GetR(src), dstAlpha);
  int g = computeOut(GPixel_GetG(src), dstAlpha);
  int b = computeOut(GPixel_GetB(src), dstAlpha);

  return GPixel_PackARGB(a, r, g, b);
}

// kDstOut,   //!<     (1 - Sa)*D
GPixel dstOutMode(const GPixel src, const GPixel dst) {
  int srcAlpha = GPixel_GetA(src);

  int a = computeOut(GPixel_GetA(dst), srcAlpha);
  int r = computeOut(GPixel_GetR(dst), srcAlpha);
  int g = computeOut(GPixel_GetG(dst), srcAlpha);
  int b = computeOut(GPixel_GetB(dst), srcAlpha);

  return GPixel_PackARGB(a, r, g, b);
}

// kSrcATop,  //!<     Da*S + (1 - Sa)*D
GPixel srcATopMode(const GPixel src, const GPixel dst) {
  int srcAlpha = GPixel_GetA(src);
  int dstAlpha = GPixel_GetA(dst);

  int a = computeATop(srcAlpha, dstAlpha, srcAlpha, dstAlpha);
  int r = computeATop(GPixel_GetR(src), GPixel_GetR(dst), srcAlpha, dstAlpha);
  int g = computeATop(GPixel_GetG(src), GPixel_GetG(dst), srcAlpha, dstAlpha);
  int b = computeATop(GPixel_GetB(src), GPixel_GetB(dst), srcAlpha, dstAlpha);

  return GPixel_PackARGB(a, r, g, b);
}

// kDstATop,  //!<     Sa*D + (1 - Da)*S
GPixel dstATopMode(const GPixel src, const GPixel dst) {
  int srcAlpha = GPixel_GetA(src);
  int dstAlpha = GPixel_GetA(dst);

  int a = computeATop(dstAlpha, srcAlpha, dstAlpha, srcAlpha);
  int r = computeATop(GPixel_GetR(dst), GPixel_GetR(src), dstAlpha, srcAlpha);
  int g = computeATop(GPixel_GetG(dst), GPixel_GetG(src), dstAlpha, srcAlpha);
  int b = computeATop(GPixel_GetB(dst), GPixel_GetB(src), dstAlpha, srcAlpha);

  return GPixel_PackARGB(a, r, g, b);
}

// kXor,      //!<     (1 - Sa)*D + (1 - Da)*S
GPixel xorMode(const GPixel src, const GPixel dst) {
  int srcAlpha = GPixel_GetA(src);
  int dstAlpha = GPixel_GetA(dst);

  if (srcAlpha == 255 && dstAlpha == 255) return clearMode(src, dst);

  int a = computeXor(srcAlpha, dstAlpha, srcAlpha, dstAlpha);
  int r = computeXor(GPixel_GetR(src), GPixel_GetR(dst), srcAlpha, dstAlpha);
  int g = computeXor(GPixel_GetG(src), GPixel_GetG(dst), srcAlpha, dstAlpha);
  int b = computeXor(GPixel_GetB(src), GPixel_GetB(dst), srcAlpha, dstAlpha);

  return GPixel_PackARGB(a, r, g, b);
}

