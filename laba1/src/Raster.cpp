#include "Raster.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

Color Color::fromBytes(int red, int green, int blue, int alpha) {
    for (int channel : {red, green, blue, alpha})
        if (channel < 0 || channel > 255)
            throw std::invalid_argument("Color channel must be in 0..255");
    return {red / 255.f, green / 255.f, blue / 255.f, alpha / 255.f};
}

int Color::toByte(float channel) {
    return static_cast<int>(std::lround(255.f * std::clamp(channel, 0.f, 1.f)));
}

int Color::grayscale() const {
    return toByte(0.299f * r + 0.587f * g + 0.114f * b);
}

Color Color::operator+(const Color& other) const {
    return {r + other.r, g + other.g, b + other.b, a + other.a};
}

Color Color::operator*(float factor) const {
    return {r * factor, g * factor, b * factor, a * factor};
}

Color Color::lerp(const Color& other, float t) const {
    t = std::clamp(t, 0.f, 1.f);
    return *this * (1 - t) + other * t;
}

Color Color::blend(const Color& source, float coverage) const {
    const float srcAlpha = source.a * std::clamp(coverage, 0.f, 1.f);
    const float bgAlpha = a * (1 - srcAlpha);
    const float alpha = srcAlpha + bgAlpha;
    if (alpha == 0)
        return {0, 0, 0, 0};

    Color mixed = (source * srcAlpha + *this * bgAlpha) * (1 / alpha);
    mixed.a = alpha;
    return mixed;
}

Image::Image(int width, int height, Color background) {
    resize(width, height, background);
}

bool Image::contains(int x, int y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

std::size_t Image::index(int x, int y) const {
    if (!contains(x, y))
        throw std::out_of_range("Pixel is outside the image");
    return static_cast<std::size_t>(y) * width_ + x;
}

Color& Image::at(int x, int y) {
    return pixels_[index(x, y)];
}

const Color& Image::at(int x, int y) const {
    return pixels_[index(x, y)];
}

void Image::clear(Color background) {
    std::fill(pixels_.begin(), pixels_.end(), background);
}

void Image::resize(int width, int height, Color background) {
    if (width < 1 || height < 1 || width > 4096 || height > 4096)
        throw std::invalid_argument("Image dimensions must be in 1..4096");

    std::vector<Color> pixels(static_cast<std::size_t>(width) * height, background);
    pixels_.swap(pixels);
    width_ = width;
    height_ = height;
}

void Canvas::plot(int x, int y, float coverage) {
    if (image_.contains(x, y) && coverage > 0) {
        Color& pixel = image_.at(x, y);
        pixel = pixel.blend(pen_, coverage);
    }
}

void Canvas::lineWu(double x0, double y0, double x1, double y1) {
    const bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    auto ink = [&](int x, int y, double coverage) {
        if (steep)
            plot(y, x, static_cast<float>(coverage));
        else
            plot(x, y, static_cast<float>(coverage));
    };
    const auto fraction = [](double value) { return value - std::floor(value); };
    auto pair = [&](int x, double y, double weight) {
        const int lower = static_cast<int>(std::floor(y));
        const double f = fraction(y);
        ink(x, lower, (1 - f) * weight);
        ink(x, lower + 1, f * weight);
    };

    if (x0 == x1) {
        ink(static_cast<int>(std::lround(x0)), static_cast<int>(std::lround(y0)), 1);
        return;
    }

    const double slope = (y1 - y0) / (x1 - x0);
    const int first = static_cast<int>(std::lround(x0));
    const int last = static_cast<int>(std::lround(x1));
    if (first == last) {
        pair(first, (y0 + y1) / 2, x1 - x0);
        return;
    }
    pair(first, y0 + slope * (first - x0), 1 - fraction(x0 + 0.5));
    const int limit = steep ? image_.height() : image_.width();
    for (int x = std::max(first + 1, 0); x < std::min(last, limit); ++x)
        pair(x, y0 + slope * (x - x0), 1);
    pair(last, y0 + slope * (last - x0), fraction(x1 + 0.5));
}

void Canvas::lineThick(Point from, Point to, int thickness) {
    if (thickness < 1)
        throw std::invalid_argument("Thickness must be positive");
    if (thickness == 1) {
        const int dx = std::abs(to.x - from.x);
        const int dy = -std::abs(to.y - from.y);
        const int sx = from.x < to.x ? 1 : -1;
        const int sy = from.y < to.y ? 1 : -1;
        int error = dx + dy;
        while (true) {
            plot(from.x, from.y);
            if (from.x == to.x && from.y == to.y)
                break;
            const int doubled = 2 * error;
            if (doubled >= dy) {
                error += dy;
                from.x += sx;
            }
            if (doubled <= dx) {
                error += dx;
                from.y += sy;
            }
        }
        return;
    }

    const double dx = to.x - from.x;
    const double dy = to.y - from.y;
    const double length = std::hypot(dx, dy);
    const double ux = length == 0 ? 1 : dx / length;
    const double uy = length == 0 ? 0 : dy / length;
    const double radius = thickness / 2.0;
    const int pad = thickness + 1;
    const int minX = std::max(0, std::min(from.x, to.x) - pad);
    const int maxX = std::min(image_.width() - 1, std::max(from.x, to.x) + pad);
    const int minY = std::max(0, std::min(from.y, to.y) - pad);
    const int maxY = std::min(image_.height() - 1, std::max(from.y, to.y) + pad);
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const double along = (x - from.x) * ux + (y - from.y) * uy;
            const double across = -(x - from.x) * uy + (y - from.y) * ux;
            if (along >= -radius && along < length + radius &&
                across >= -radius && across < radius)
                plot(x, y);
        }
    }
}

namespace {

class TokenWriter {
public:
    explicit TokenWriter(std::ostream& stream) : stream_(stream) {}

    void write(int sample) {
        const std::string token = std::to_string(sample);
        if (column_ + 1 + token.size() > 70) {
            stream_ << '\n';
            column_ = 0;
        }
        if (column_ != 0) {
            stream_ << ' ';
            ++column_;
        }
        stream_ << token;
        column_ += token.size();
    }

private:
    std::ostream& stream_;
    std::size_t column_ = 0;
};

template<class WritePixel>
void writePNM(const Image& image, std::ostream& stream, const char* magic, WritePixel writePixel) {
    stream << magic << '\n' << image.width() << ' ' << image.height() << "\n255\n";
    TokenWriter writer(stream);
    const std::size_t count = static_cast<std::size_t>(image.width()) * image.height();
    for (std::size_t i = 0; i < count; ++i)
        writePixel(writer, image.data()[i]);
    stream << '\n';
    if (!stream)
        throw std::runtime_error("Cannot write image data");
}

}

void P3Exporter::write(const Image& image, std::ostream& stream) const {
    writePNM(image, stream, "P3", [](TokenWriter& writer, Color color) {
        writer.write(Color::toByte(color.r));
        writer.write(Color::toByte(color.g));
        writer.write(Color::toByte(color.b));
    });
}

void P2Exporter::write(const Image& image, std::ostream& stream) const {
    writePNM(image, stream, "P2", [](TokenWriter& writer, Color color) {
        writer.write(color.grayscale());
    });
}
