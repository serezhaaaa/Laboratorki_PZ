#include "Shapes.h"

#include <algorithm>
#include <cmath>
#include <stack>
#include <stdexcept>
#include <utility>

Rectangle::Rectangle(Point center, int width, int height, Color border, Color fill,
                     int thickness, bool filled)
    : center_(center), width_(width), height_(height), border_(border), fill_(fill),
      thickness_(thickness), filled_(filled) {
    if (width < 1 || height < 1 || thickness < 1)
        throw std::invalid_argument("Rectangle dimensions and thickness must be positive");
}

void Rectangle::draw(Canvas& canvas) const {
    const int left = center_.x - width_ / 2;
    const int top = center_.y - height_ / 2;
    const int right = left + width_;
    const int bottom = top + height_;
    const int endX = std::min(right, canvas.image().width());
    const int endY = std::min(bottom, canvas.image().height());

    for (int y = std::max(top, 0); y < endY; ++y) {
        for (int x = std::max(left, 0); x < endX; ++x) {
            const bool edge = x - left < thickness_ || right - x <= thickness_ ||
                              y - top < thickness_ || bottom - y <= thickness_;
            if (edge || filled_) {
                canvas.setPen(edge ? border_ : fill_);
                canvas.plot(x, y);
            }
        }
    }
}

NestedSquares::NestedSquares(Point center, int side, int count, int step, Color start, Color end)
    : center_(center), side_(side), count_(count), step_(step), start_(start), end_(end) {
    if (side < 1 || count < 1 || step < 1)
        throw std::invalid_argument("Square size, count and step must be positive");
}

void NestedSquares::draw(Canvas& canvas) const {
    int side = side_;
    for (int i = 0; i < count_ && side > 0; ++i) {
        float t = 0;
        if (count_ > 1)
            t = static_cast<float>(i) / (count_ - 1);
        const Color color = start_.lerp(end_, t);
        Rectangle(center_, side, side, color, {}, 2, false).draw(canvas);
        side -= 2 * step_;
    }
}

LineSegment::LineSegment(Point from, Point to, Color color, Mode mode, int thickness)
    : from_(from), to_(to), color_(color), mode_(mode), thickness_(thickness) {
    if (thickness < 1)
        throw std::invalid_argument("Thickness must be positive");
}

void LineSegment::draw(Canvas& canvas) const {
    canvas.setPen(color_);
    if (mode_ == Mode::Wu)
        canvas.lineWu(from_.x, from_.y, to_.x, to_.y);
    else
        canvas.lineThick(from_, to_, thickness_);
}

Polygon::Polygon(std::vector<Point> vertices, Color outline, int thickness)
    : vertices_(std::move(vertices)), outline_(outline), thickness_(thickness) {
    if (vertices_.size() < 3 || thickness < 1)
        throw std::invalid_argument("Polygon needs at least three vertices and positive thickness");
}

void Polygon::draw(Canvas& canvas) const {
    canvas.setPen(outline_);
    for (std::size_t i = 0; i < vertices_.size(); ++i)
        canvas.lineThick(vertices_[i], vertices_[(i + 1) % vertices_.size()], thickness_);
}

std::size_t FloodFill::fill(Image& image, Point seed, Color color, double tolerance) {
    if (!std::isfinite(tolerance) || tolerance < 0 || tolerance > 442)
        throw std::invalid_argument("RGB tolerance must be in 0..442");
    const Color target = image.at(seed.x, seed.y);
    const double limit = tolerance * tolerance;
    std::vector<unsigned char> visited(
        static_cast<std::size_t>(image.width()) * image.height());
    std::stack<Point> pending;
    auto queuePixel = [&](Point point) {
        if (!image.contains(point.x, point.y))
            return;

        const std::size_t index = static_cast<std::size_t>(point.y) * image.width() + point.x;
        if (visited[index])
            return;
        visited[index] = 1;

        const Color pixel = image.at(point.x, point.y);
        const double r = 255 * (pixel.r - target.r);
        const double g = 255 * (pixel.g - target.g);
        const double b = 255 * (pixel.b - target.b);
        if (r * r + g * g + b * b <= limit)
            pending.push(point);
    };

    queuePixel(seed);
    std::size_t filled = 0;
    while (!pending.empty()) {
        const Point point = pending.top();
        pending.pop();
        image.at(point.x, point.y) = color;
        ++filled;
        queuePixel({point.x - 1, point.y});
        queuePixel({point.x + 1, point.y});
        queuePixel({point.x, point.y - 1});
        queuePixel({point.x, point.y + 1});
    }
    return filled;
}

TruchetBoard::TruchetBoard(int columns, int rows, Color first, Color second, bool arcs, Color pattern)
    : columns_(columns), rows_(rows), first_(first), second_(second), arcs_(arcs), pattern_(pattern) {
    if (columns < 1 || rows < 1)
        throw std::invalid_argument("Grid dimensions must be positive");
}

void TruchetBoard::draw(Canvas& canvas) const {
    Image& image = canvas.image();
    const int side = std::min(image.width() / columns_, image.height() / rows_);
    if (side < 4)
        throw std::invalid_argument("Grid cells must be at least four pixels wide");

    const int left = (image.width() - side * columns_) / 2;
    const int top = (image.height() - side * rows_) / 2;
    for (int j = 0; j < rows_; ++j) {
        for (int i = 0; i < columns_; ++i) {
            const int x = left + i * side;
            const int y = top + j * side;
            const Color color = (i + j) % 2 == 0 ? first_ : second_;
            Rectangle({x + side / 2, y + side / 2}, side, side,
                      color, color, 1, true).draw(canvas);
        }
    }

    constexpr double pi = 3.141592653589793;
    const double radius = side / 2.0;
    const int steps = std::max(4, static_cast<int>(std::ceil(pi * radius / 4)));
    auto arc = [&](double cx, double cy, double angle) {
        double x = cx + radius * std::cos(angle);
        double y = cy + radius * std::sin(angle);
        for (int i = 1; i <= steps; ++i) {
            const double next = angle + pi * i / (2 * steps);
            const double nx = cx + radius * std::cos(next);
            const double ny = cy + radius * std::sin(next);
            canvas.lineWu(x, y, nx, ny);
            x = nx;
            y = ny;
        }
    };

    canvas.setPen(pattern_);
    for (int j = 0; j < rows_; ++j) {
        for (int i = 0; i < columns_; ++i) {
            const double x = left + i * side;
            const double y = top + j * side;
            const bool flipped = (i * j) % 2 != 0;
            if (arcs_) {
                if (flipped) {
                    arc(x + side, y, pi / 2);
                    arc(x, y + side, 3 * pi / 2);
                } else {
                    arc(x, y, 0);
                    arc(x + side, y + side, pi);
                }
            } else if (flipped) {
                canvas.lineWu(x, y, x + side, y + side);
            } else {
                canvas.lineWu(x, y + side, x + side, y);
            }
        }
    }
}
