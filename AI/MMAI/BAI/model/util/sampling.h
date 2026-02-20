/*
 * sampling.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <onnxruntime_cxx_api.h>

#include "BAI/model/util/common.h"

namespace MMAI::BAI::sampling
{
struct SampleResult
{
	int index;
	double prob;
	bool fallback;
};

struct TripletSample
{
	int act0;
	int hex1;
	int hex2;
	double confidence;
};

std::vector<int64_t> shape_of(const Ort::Value & v);

template<typename T>
std::vector<T> to_vector(const Ort::Value & v);

std::vector<double> softmax(const std::vector<double> & logits);
int argmax(const std::vector<double> & xs);
int count_valid(const std::vector<int32_t> & mask_1d);
std::vector<double> make_masked_logits(const std::vector<float> & logits_1d, const std::vector<int32_t> & mask_1d);

SampleResult sample_uniform_over_mask(const std::vector<int32_t> & mask_1d, int n_valid, std::mt19937 & rng);

SampleResult sample_softmax_over_mask(const std::vector<double> & masked_logits, const std::vector<int32_t> & mask_1d, double temperature, std::mt19937 & rng);

// Masked categorical sampling given a logits vector
SampleResult
sample_masked_logits(const std::vector<float> & logits_1d, const std::vector<int32_t> & mask_1d, bool throw_if_empty, double temperature, std::mt19937 & rng);

//
// Samples a {action, hex1, hex2} triplet given output logits and masks
//
// Expected shapes:
//   act0_logits: [1, 4]            float32
//   hex1_logits: [1, 165]          float32
//   hex2_logits: [1, 165]          float32
//   mask_act0:   [1, 4]            int32
//   mask_hex1:   [1, 4, 165]       int32
//   mask_hex2:   [1, 4, 165, 165]  int32
//
TripletSample
sample_triplet(const MaskedLogits & act0_logits, const MaskedLogits & hex1_logits, const MaskedLogits & hex2_logits, double temperature, std::mt19937 & rng);
}
