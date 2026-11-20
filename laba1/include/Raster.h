#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <vector>

struct Color {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 1;

    static Color fromBytes(int red, int green, int blue, int alpha = 255);
    static int toByte(float channel);
    int grayscale() const;
    Color lerp(const Color& other, float t) const;
    Color blend(const Color& source, float coverage) const;
    Color operator+(const Color& other) const;
    Color operator*(float factor) const;
};

struct Point {
    int x;
    int y;
};

class Image {
public:
    Image(int width, int height, Color background);
    int width() const { return width_; }
    int height() const { return height_; }
    bool contains(int x, int y) const;
    Color& at(int x, int y);
    const Color& at(int x, int y) const;
    void clear(Color background);
    void resize(int width, int height, Color background);
    Color* data() { return pixels_.data(); }
    const Color* data() const { return pixels_.data(); }

private:
    std::size_t index(int x, int y) const;
    int width_ = 0;
    int height_ = 0;
    std::vector<Color> pixels_;
};

class Canvas {
public:
    explicit Canvas(Image& image) : image_(image) {}
    Image& image() { return image_; }
    void setPen(Color color) { pen_ = color; }
    void plot(int x, int y, float coverage = 1);
    void lineWu(double x0, double y0, double x1, double y1);
    void lineThick(Point from, Point to, int thickness);

private:
    Image& image_;
    Color pen_;
};

class IPNMExporter {
public:
    virtual ~IPNMExporter() = default;
    virtual void write(const Image& image, std::ostream& stream) const = 0;
};

class P3Exporter : public IPNMExporter {
public:
    void write(const Image& image, std::ostream& stream) const override;
};

class P2Exporter : public IPNMExporter {
public:
    void write(const Image& image, std::ostream& stream) const override;
};
