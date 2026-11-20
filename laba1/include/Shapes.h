#pragma once

#include "Raster.h"

class IShape {
public:
    virtual ~IShape() = default;
    virtual void draw(Canvas& canvas) const = 0;
};

class Rectangle : public IShape {
public:
    Rectangle(Point center, int width, int height, Color border, Color fill,
              int thickness, bool filled);
    void draw(Canvas& canvas) const override;

private:
    Point center_;
    int width_, height_;
    Color border_, fill_;
    int thickness_;
    bool filled_;
};

class NestedSquares : public IShape {
public:
    NestedSquares(Point center, int side, int count, int step, Color start, Color end);
    void draw(Canvas& canvas) const override;

private:
    Point center_;
    int side_, count_, step_;
    Color start_, end_;
};

class LineSegment : public IShape {
public:
    enum class Mode { Wu, Thick };
    LineSegment(Point from, Point to, Color color, Mode mode, int thickness = 1);
    void draw(Canvas& canvas) const override;

private:
    Point from_, to_;
    Color color_;
    Mode mode_;
    int thickness_;
};

class Polygon : public IShape {
public:
    Polygon(std::vector<Point> vertices, Color outline, int thickness);
    void draw(Canvas& canvas) const override;

private:
    std::vector<Point> vertices_;
    Color outline_;
    int thickness_;
};

class FloodFill {
public:
    static std::size_t fill(Image& image, Point seed, Color color, double tolerance);
};

class TruchetBoard : public IShape {
public:
    TruchetBoard(int columns, int rows, Color first, Color second, bool arcs,
                 Color pattern = Color::fromBytes(255, 226, 151));
    void draw(Canvas& canvas) const override;

private:
    int columns_, rows_;
    Color first_, second_;
    bool arcs_;
    Color pattern_;
};
