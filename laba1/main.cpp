#include "Shapes.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

struct EndOfInput {};

std::string readLine(const char* prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line))
        throw EndOfInput{};
    return line;
}

template<class... Values>
void readValues(const char* prompt, Values&... values) {
    std::istringstream input(readLine(prompt));
    std::string extra;
    if (!(input >> ... >> values) || input >> extra)
        throw std::invalid_argument("Invalid input. Enter the requested values.");
}

int readInt(const char* prompt, int minimum, int maximum) {
    int value;
    readValues(prompt, value);
    if (value < minimum || value > maximum)
        throw std::invalid_argument("Number is outside the allowed range");
    return value;
}

Point readPoint(const char* prompt) {
    Point point{};
    readValues(prompt, point.x, point.y);
    if (point.x < -8192 || point.x > 8192 || point.y < -8192 || point.y > 8192)
        throw std::invalid_argument("Coordinates must be in -8192..8192");
    return point;
}

Color rgb(int r, int g, int b) {
    return Color::fromBytes(r, g, b);
}

Color readColor(const char* prompt) {
    int r, g, b;
    readValues(prompt, r, g, b);
    return rgb(r, g, b);
}

void save(const Image& image, const IPNMExporter& exporter, const std::filesystem::path& path) {
    std::ofstream stream(path);
    if (!stream)
        throw std::runtime_error("Cannot open output file: " + path.string());
    exporter.write(image, stream);
    stream.close();
    if (!stream)
        throw std::runtime_error("Cannot finish writing file: " + path.string());
}

void savePair(const Image& image, const std::filesystem::path& folder, const char* name) {
    save(image, P3Exporter{}, folder / (std::string(name) + ".ppm"));
    save(image, P2Exporter{}, folder / (std::string(name) + ".pgm"));
}

void drawRectangles(Canvas& canvas) {
    const Point center = readPoint("Rectangle center (x y): ");
    const int width = readInt("Rectangle width (1..8192): ", 1, 8192);
    const int height = readInt("Rectangle height (1..8192): ", 1, 8192);
    const Color border = readColor("Border RGB: ");
    const bool filled = readInt("Fill (0 - no, 1 - yes): ", 0, 1) == 1;
    Color fill;
    if (filled)
        fill = readColor("Fill RGB: ");
    const int thickness = readInt("Border thickness (1..256): ", 1, 256);
    Rectangle(center, width, height, border, fill, thickness, filled).draw(canvas);

    const Point squareCenter = readPoint("Squares center (x y): ");
    const int side = readInt("First square side (1..8192): ", 1, 8192);
    const int count = readInt("Square count (1..256): ", 1, 256);
    const int step = readInt("Square step (1..4096): ", 1, 4096);
    const Color start = readColor("First square RGB: ");
    const Color end = readColor("Last square RGB: ");
    NestedSquares(squareCenter, side, count, step, start, end).draw(canvas);
}

void drawLine(Canvas& canvas) {
    const Point from = readPoint("Point A (x y): ");
    const Point to = readPoint("Point B (x y): ");
    const int mode = readInt("Algorithm (1 - Wu, 2 - Bresenham/thick): ", 1, 2);
    const Color color = readColor("Line RGB: ");
    if (mode == 1) {
        LineSegment(from, to, color, LineSegment::Mode::Wu).draw(canvas);
    } else {
        const int t = readInt("Thickness (1..256): ", 1, 256);
        LineSegment(from, to, color, LineSegment::Mode::Thick, t).draw(canvas);
    }
}

void drawPolygon(Canvas& canvas) {
    const int count = readInt("Vertex count (3..100): ", 3, 100);
    std::vector<Point> vertices;
    for (int i = 0; i < count; ++i)
        vertices.push_back(readPoint("Vertex (x y): "));
    const Color border = readColor("Polygon outline RGB: ");
    const int thickness = readInt("Outline thickness (1..256): ", 1, 256);
    Polygon(std::move(vertices), border, thickness).draw(canvas);
    const Point seed = readPoint("Fill seed (x y): ");
    const Color color = readColor("Fill RGB: ");
    double tolerance;
    readValues("RGB tolerance (0..442): ", tolerance);
    const std::size_t countFilled = FloodFill::fill(canvas.image(), seed, color, tolerance);
    std::cout << "Filled pixels: " << countFilled << '\n';
}

void drawBoard(Canvas& canvas) {
    const int columns = readInt("Columns (1..128): ", 1, 128);
    const int rows = readInt("Rows (1..128): ", 1, 128);
    const Color first = readColor("First cell RGB: ");
    const Color second = readColor("Second cell RGB: ");
    const bool arcs = readInt("Pattern (1 - arcs, 2 - diagonals): ", 1, 2) == 1;
    TruchetBoard(columns, rows, first, second, arcs).draw(canvas);
}

void saveSamples(const std::filesystem::path& folder) {
    std::filesystem::create_directories(folder);
    const Color white = rgb(255, 255, 255);
    const Color ink = rgb(23, 41, 70);
    const Color orange = rgb(177, 69, 7);
    const Color teal = rgb(0, 111, 112);
    const Color blue = rgb(29, 73, 158);
    const Color violet = rgb(146, 91, 188);
    const Color lightTeal = rgb(199, 225, 222);
    Image image(640, 320, white);
    Canvas canvas(image);

    Rectangle({146, 160}, 208, 138, teal, lightTeal, 5, true).draw(canvas);
    NestedSquares({474, 160}, 244, 10, 11, blue, orange).draw(canvas);
    savePair(image, folder, "01-rectangles");

    image.resize(640, 240, white);
    LineSegment({34, 185}, {600, 42}, orange, LineSegment::Mode::Wu).draw(canvas);
    LineSegment({45, 44}, {609, 142}, teal, LineSegment::Mode::Wu).draw(canvas);
    LineSegment({317, 212}, {373, 22}, blue, LineSegment::Mode::Wu).draw(canvas);
    savePair(image, folder, "02-wu");

    image.clear(white);
    LineSegment({34, 187}, {598, 47}, orange, LineSegment::Mode::Thick, 21).draw(canvas);
    LineSegment({45, 38}, {610, 164}, teal, LineSegment::Mode::Thick, 11).draw(canvas);
    LineSegment({317, 208}, {373, 26}, blue, LineSegment::Mode::Thick, 5).draw(canvas);
    savePair(image, folder, "03-thick");

    image.resize(640, 350, white);
    Polygon({{89, 90}, {237, 42}, {337, 112}, {463, 65}, {553, 168},
             {496, 292}, {347, 273}, {259, 313}, {108, 248}},
            ink, 5).draw(canvas);
    const std::size_t filled = FloodFill::fill(image, {316, 178}, violet, 0);
    std::cout << "Polygon fill: " << filled << " pixels\n";
    savePair(image, folder, "04-flood-fill");

    image.resize(640, 360, white);
    TruchetBoard(9, 5, white, lightTeal, true, ink).draw(canvas);
    savePair(image, folder, "05-truchet-arcs");

    image.clear(white);
    TruchetBoard(9, 5, white, lightTeal, false, ink).draw(canvas);
    savePair(image, folder, "06-truchet-diagonals");
}

void menu() {
    using Draw = void (*)(Canvas&);
    const Draw actions[] = {drawRectangles, drawLine, drawPolygon, drawBoard};
    while (true) {
        std::cout << "\n=== Netpbm Raster Generator (C++17) ===\n"
                  << "1. Rectangle and nested squares\n"
                  << "2. Wu line or thick line\n"
                  << "3. Polygon and flood fill\n"
                  << "4. Variant 2: Truchet board\n"
                  << "0. Exit\n";
        try {
            const int choice = readInt("Choose an option: ", 0, 4);
            if (choice == 0)
                return;

            const int width = readInt("Image width (1..4096): ", 1, 4096);
            const int height = readInt("Image height (1..4096): ", 1, 4096);
            Image image(width, height, readColor("Background RGB: "));
            Canvas canvas(image);
            actions[choice - 1](canvas);

            const int format = readInt("Format (1 - PPM P3, 2 - PGM P2): ", 1, 2);
            std::unique_ptr<IPNMExporter> exporter;
            if (format == 1)
                exporter = std::make_unique<P3Exporter>();
            else
                exporter = std::make_unique<P2Exporter>();
            const std::string path = readLine("Output file name: ");
            if (path.empty())
                throw std::invalid_argument("File name cannot be empty");
            save(image, *exporter, path);
            std::cout << "Saved " << path << '\n';
        } catch (const EndOfInput&) {
            return;
        } catch (const std::exception& error) {
            std::cerr << "Error: " << error.what() << '\n';
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc == 1) {
            menu();
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--samples") {
            saveSamples(argv[2]);
            return 0;
        }
        std::cerr << "Usage: " << argv[0] << " [--samples DIRECTORY]\n";
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
    }
    return 1;
}
