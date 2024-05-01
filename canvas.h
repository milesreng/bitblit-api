#ifndef _g_canvas_h_
#define _g_canvas_h_

#include "include/GCanvas.h"
#include "include/GRect.h"
#include "include/GColor.h"
#include "include/GBitmap.h"
#include <iostream>

class MyCanvas : public GCanvas {
  public:
    MyCanvas(const GBitmap& device) : fDevice(device), ctm({ GMatrix() }) {}

    void save() override;
    void restore() override;
    void concat(const GMatrix& matrix) override;

    void clear(const GColor& color) override;

    void drawRect(const GRect&, const GPaint&) override;
    void drawConvexPolygon(const GPoint[], int count, const GPaint&) override;

    void drawPath(const GPath& path, const GPaint&) override;

    void drawMesh(const GPoint verts[], const GColor colors[], const GPoint texs[],
                          int count, const int indices[], const GPaint&) override;

    void drawQuad(const GPoint verts[4], const GColor colors[4], const GPoint texs[4],
                          int level, const GPaint&) override;

    void drawCubicQuad(const GPoint verts[12], const GColor colors[4], const GPoint texs[4],
                          int level, const GPaint&) override;

  private:
    const GBitmap fDevice;
    std::vector<GMatrix> ctm {};
};

#endif