// SPDX-FileCopyrightText: (c) 2024 Tenstorrent AI ULC
//
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ttnn/tensor/tensor.hpp"
#include "ttnn/tensor/types.hpp"
#include "ttnn/tensor/xtensor/conversion_utils.hpp"
#include "ttnn/tensor/xtensor/xtensor_all_includes.hpp"

namespace ttnn {
namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::ttnn::experimental::xtensor::from_xtensor;
using ::ttnn::experimental::xtensor::get_shape_from_xarray;
using ::ttnn::experimental::xtensor::span_to_xtensor_view;
using ::ttnn::experimental::xtensor::to_xtensor;
using ::ttnn::experimental::xtensor::xtensor_to_span;

TensorSpec get_tensor_spec(const ttnn::SimpleShape& shape) {
    return TensorSpec(shape, TensorLayout(DataType::FLOAT32, Layout::ROW_MAJOR, MemoryConfig{}));
}

TEST(XtensorConversionTest, SpanToXtensor) {
    std::vector<int> data = {1, 2, 3, 4, 5, 6};
    tt::stl::Span<const int> data_span(data.data(), data.size());
    ttnn::SimpleShape shape({2, 3});

    auto result = span_to_xtensor_view(data_span, shape);

    // Check shape
    EXPECT_THAT(result.shape(), ElementsAre(2, 3));

    // Check data
    int expected_val = 1;
    for (size_t i = 0; i < result.shape()[0]; ++i) {
        for (size_t j = 0; j < result.shape()[1]; ++j) {
            EXPECT_EQ(result(i, j), expected_val++);
        }
    }
}

TEST(XtensorConversionTest, XtensorToSpan) {
    xt::xarray<float> arr = {{1.0f, 2.0f}, {3.0f, 4.0f}};
    EXPECT_THAT(xtensor_to_span(arr), ElementsAre(1.0f, 2.0f, 3.0f, 4.0f));
}

TEST(XtensorConversionTest, GetShape) {
    EXPECT_THAT(
        get_shape_from_xarray(xt::xarray<int>::from_shape({2, 3, 4, 5, 6})), Eq(ttnn::SimpleShape{2, 3, 4, 5, 6}));
    EXPECT_THAT(get_shape_from_xarray(xt::xarray<int>::from_shape({7})), Eq(ttnn::SimpleShape{7}));
}

TEST(XtensorConversionTest, FromXtensorInvalidShape) {
    xt::xarray<float> arr = {{1.0f, 2.0f}, {3.0f, 4.0f}};
    EXPECT_ANY_THROW(from_xtensor(arr, get_tensor_spec(ttnn::SimpleShape{3, 3})));
}

TEST(XtensorConversionTest, Roundtrip) {
    const std::vector<ttnn::SimpleShape> shapes{
        ttnn::SimpleShape{1},
        ttnn::SimpleShape{1, 1, 1, 1},
        ttnn::SimpleShape{1, 1, 1, 10},
        ttnn::SimpleShape{1, 32, 32, 16},
        ttnn::SimpleShape{1, 40, 3, 128},
        ttnn::SimpleShape{2, 2},
        ttnn::SimpleShape{1, 1, 1, 1, 10},
    };

    for (const auto& shape : shapes) {
        const auto tensor_spec = get_tensor_spec(shape);
        xt::xarray<float> input = xt::arange<float>(shape.volume());
        xt::dynamic_shape<std::size_t> new_shape(shape.cbegin(), shape.cend());
        input.reshape(new_shape);

        auto output = to_xtensor<float>(from_xtensor(input, tensor_spec));
        EXPECT_TRUE(xt::allclose(input, output));
    }
}

}  // namespace
}  // namespace ttnn