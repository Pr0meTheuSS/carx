#pragma once

#include <cmath>
#include <stdexcept>
#include <vector>

namespace Spline2D {

enum class Direction { LEFT, RIGHT };

struct Point2D {
    double x{};
    double y{};
};

struct Corner {
    int value;
    Direction direction;
    explicit Corner(int v, Direction dir) {
        if (v < 1 || v > 9) throw std::runtime_error("corner out of range");
        value = v;
        direction = dir;
    }
};

// перевод в градусы (как у тебя было задумано)
struct Degree {
    double value;

    explicit Degree(double v) : value(v) {}

    explicit Degree(const Corner& c) { value = (c.direction == Direction::LEFT) ? 90.0 + 10.0 * (c.value - 1) : 270 - 10.0 * (c.value - 1); }
};

// поворот вектора
static inline Point2D step(Point2D dir, double angleRad) {
    return {dir.x * std::cos(angleRad) + dir.y * std::sin(angleRad), -dir.x * std::sin(angleRad) + dir.y * std::cos(angleRad)};
}

std::vector<Point2D> generateSpline2D(const std::vector<Corner>& corners, double stepSize = 1.0) {
    std::vector<Point2D> pts;
    pts.reserve(corners.size() + 1);

    Point2D pos{0, 0};
    Point2D dir{0, -1};

    pts.push_back(pos);

    for (const auto& c : corners) {
        Degree d(c);
        double rad = d.value * M_PI / 180.0;

        dir = step(dir, rad);

        pos.x += dir.x * stepSize;
        pos.y += dir.y * stepSize;

        pts.push_back(pos);
    }

    return pts;
}

}  // namespace Spline2D