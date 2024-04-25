#include "include/GPixel.h"
#include "include/GColor.h"

GPixel color_to_pixel(GColor color) {
  int a = GRoundToInt(color.a * 255);
  int r = GRoundToInt(color.r * color.a * 255);
  int g = GRoundToInt(color.g * color.a * 255);
  int b = GRoundToInt(color.b * color.a * 255);

  return GPixel_PackARGB(a, r, g, b);
}