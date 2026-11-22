// Build this harness together with the candidate LibreCAD_3 spline.cpp under
// AddressSanitizer. The rest of lckernel may be a non-instrumented shared
// library; this checks builder removal code, not the whole application.
#include <gtest/gtest.h>
#include <cad/builders/spline.h>
#include <limits>
#include <stdexcept>
#include <vector>

TEST(SplineBuilderRemoval, BoundsAndWeightAssociation) {
    lc::builder::SplineBuilder builder;
    builder.setControlPoints({{1,2,3},{4,5,6},{7,8,9},{10,11,12}});
    builder.setWeights({1,2,3,4});
    builder.removeControlPoint(-2);
    EXPECT_EQ(builder.weights(),(std::vector<double>{1,2,4}));
    EXPECT_EQ(builder.controlPoints().size(),3u);
    EXPECT_THROW(builder.removeControlPoint(std::numeric_limits<int>::min()),std::out_of_range);
    EXPECT_THROW(builder.removeControlPoint(3),std::out_of_range);
    EXPECT_EQ(builder.weights(),(std::vector<double>{1,2,4}));
    builder.setWeights({7});
    EXPECT_THROW(builder.removeControlPoint(0),std::logic_error);
    EXPECT_EQ(builder.weights(),(std::vector<double>{7}));
    EXPECT_EQ(builder.controlPoints().size(),3u);
    builder.setWeights({1,2,4});
    builder.removeControlPoint(0);
    EXPECT_EQ(builder.weights(),(std::vector<double>{2,4}));
    EXPECT_DOUBLE_EQ(builder.controlPoints().front().x(),4);
    EXPECT_THROW(builder.removeKnotPoint(std::numeric_limits<int>::min()),std::out_of_range);
    EXPECT_THROW(builder.removeFitPoint(-1),std::out_of_range);
    builder.addKnotPoint(1); builder.removeKnotPoint(-1);
    builder.addFitPoint({1,2,3}); builder.removeFitPoint(0);
    EXPECT_TRUE(builder.knotPoints().empty());
    EXPECT_TRUE(builder.fitPoints().empty());
}
