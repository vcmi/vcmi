/*
 * common.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <onnxruntime_cxx_api.h>

#include "schema/v13/types.h"

namespace MMAI::BAI
{

template<typename T>
using Vec2D = std::vector<std::vector<T>>;

template<typename T>
using Vec3D = std::vector<std::vector<std::vector<T>>>;

constexpr int LT_COUNT = EI(MMAI::Schema::V13::LinkType::_count);

struct MaskedLogits
{
	const Ort::Value & logits;
	const Ort::Value & mask;
};

struct IndexContainer
{
	std::array<std::vector<int32_t>, 2> edgeIndex;
	std::vector<float> edgeAttrs;
	std::array<std::vector<int32_t>, 165> neighbourhoods;
};

template<class... Args>
[[noreturn]] inline void throwf(const std::string & fmt, Args &&... args)
{
	boost::format f("NNModel: " + fmt);
	(void)std::initializer_list<int>{((f % std::forward<Args>(args)), 0)...};
	throw std::runtime_error(f.str());
}

// tensor-to-vector convenience
template<typename T>
std::vector<T> toVector(const std::string & name, const Ort::Value & tensor, int numel)
{
	// Expect int32 tensor of shape {1}
	auto type_info = tensor.GetTensorTypeAndShapeInfo();
	auto dtype = type_info.GetElementType();

	if constexpr(std::is_same_v<T, float>)
	{
		if(dtype != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
			throwf("t2v: %s: bad dtype: want: %d, have: %d", name, EI(ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT), EI(dtype));
	}
	else if constexpr(std::is_same_v<T, int>)
	{
		if(dtype != ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32)
			throwf("t2v: %s: bad dtype: want: %d, have: %d", name, EI(ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32), EI(dtype));
	}
	else
	{
		throwf("t2v: %s: can only work with float and int", name);
	}

	auto shape = type_info.GetShape();
	if(shape.size() != 1)
		throwf("t2v: %s: expected ndim=1, got: %d", name, shape.size());

	if(shape != std::vector<int64_t>{numel})
		throwf("t2v: %s: bad shape", name);

	const T * data = tensor.GetTensorData<T>();

	auto res = std::vector<T>{};
	res.reserve(numel);
	res.assign(data, data + numel); // v now owns a copy
	return res;
}

}
