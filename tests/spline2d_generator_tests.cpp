// #include <gtest/gtest.h>

// #include "utilities/spline2d_generator.h"

// using namespace Spline2D;

// TEST(Spline, GeneratesPoints) {
//     std::vector<Corner> c = {Corner(1), Corner(1), Corner(1)};

//     auto pts = generateSpline2D(c, 1.0);

//     ASSERT_EQ(pts.size(), 4);
// }

// TEST(Spline, MovesForward) {
//     std::vector<Corner> c = {Corner(1), Corner(1), Corner(1)};

//     auto pts = generateSpline2D(c, 1.0);

//     EXPECT_GT(pts.back().x, 0);
// }

// TEST(Spline, RotationChangesDirection) {
//     std::vector<Corner> c = {
//         Corner(1), Corner(9)  // сильный поворот
//     };

//     auto pts = generateSpline2D(c, 1.0);

//     EXPECT_NE(pts.back().y, 0);
// }