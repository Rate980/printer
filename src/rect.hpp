#ifndef _RECT_HPP_
#define _RECT_HPP_
#include <stdint.h>

struct Point
{
    uint16_t x;
    uint16_t y;
    Point(uint16_t x, uint16_t y) : x(x), y(y) {}
};

struct Rect
{
    uint16_t width;
    uint16_t height;
    Rect(uint16_t w, uint16_t h) : width(w), height(h) {}
};

struct offsetRect
{
    Point offset;
    Rect rect;
    offsetRect(Point offset, Rect rect) : offset(offset), rect(rect) {}
    offsetRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) : offset(Point(x, y)), rect(Rect(w, h)) {}
    offsetRect(Point Offset, uint16_t w, uint16_t h) : offset(Offset), rect(Rect(w, h)) {}
    offsetRect(uint16_t x, uint16_t y, Rect rect) : offset(Point(x, y)), rect(rect) {}
};

#endif // _RECT_HPP_
