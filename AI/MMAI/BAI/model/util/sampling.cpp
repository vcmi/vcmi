/*
 * sampling.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

#include <onnxruntime_cxx_api.h>

#include "BAI/model/util/sampling.h"

namespace MMAI::BAI::sampling
{
std::vector<int64_t> shape_of(const Ort::Value & v)
{
	if(!v.IsTensor())
		throwf("sampling: expected tensor Ort::Value");
	Ort::TensorTypeAndShapeInfo info = v.GetTensorTypeAndShapeInfo();
	return info.GetShape();
}

template<typename T>
std::vector<T> to_vector(const Ort::Value & v)
{
	if(!v.IsTensor())
		throwf("sampling: expected tensor Ort::Value");
	Ort::TensorTypeAndShapeInfo info = v.GetTensorTypeAndShapeInfo();
	const std::vector<int64_t> shp = info.GetShape();
	size_t n = 1;
	for(auto d : shp)
	{
		if(d < 0)
			throwf("sampling: dynamic dim not supported here");
		n *= static_cast<size_t>(d);
	}
	const T * p = v.GetTensorData<T>(); // pointer used only for the copy
	return std::vector<T>(p, p + n); // everything below uses vectors only
}

std::vector<double> softmax(const std::vector<double> & logits)
{
	if(logits.empty())
		return {};
	double m = -std::numeric_limits<double>::infinity();
	for(double logit : logits)
		m = std::max(m, logit);
	std::vector<double> exps(logits.size(), 0.0);
	double sum = 0.0;
	for(size_t i = 0; i < logits.size(); ++i)
	{
		const double v = logits.at(i) - m;
		const double e = std::isfinite(v) ? std::exp(v) : 0.0;
		exps.at(i) = e;
		sum += e;
	}
	if(std::fabs(sum) < 1e-8)
		return std::vector<double>(logits.size(), 0.0);
	for(double & exp : exps)
		exp /= sum;
	return exps;
}

int argmax(const std::vector<double> & xs)
{
	if(xs.empty())
		throwf("sampling: argmax on empty vector");
	size_t best = 0;
	for(size_t i = 1; i < xs.size(); ++i)
	{
		if(xs.at(i) > xs.at(best))
			best = i;
	}
	return static_cast<int>(best);
}

std::vector<double> make_masked_logits(const std::vector<float> & logits_1d, const std::vector<int32_t> & mask_1d)
{
	const size_t K = logits_1d.size();
	const double neginf = -std::numeric_limits<double>::infinity();

	std::vector<double> masked_logits(K, neginf);
	for(size_t i = 0; i < K; ++i)
	{
		if(mask_1d.at(i))
		{
			masked_logits.at(i) = static_cast<double>(logits_1d.at(i));
		}
	}
	return masked_logits;
}

SampleResult sample_uniform_over_mask(const std::vector<int32_t> & mask_1d, int n_valid, std::mt19937 & rng)
{
	const size_t K = mask_1d.size();
	std::vector<double> probs(K, 0.0);
	const double p = 1.0 / static_cast<double>(n_valid);

	for(size_t i = 0; i < K; ++i)
	{
		if(mask_1d.at(i))
		{
			probs.at(i) = p;
		}
	}

	std::discrete_distribution<int> dist(probs.begin(), probs.end());
	const int idx_chosen = dist(rng);
	const double p_chosen = probs.at(static_cast<size_t>(idx_chosen));

	return {.index = idx_chosen, .prob = p_chosen, .fallback = false};
}

SampleResult sample_softmax_over_mask(const std::vector<double> & masked_logits, const std::vector<int32_t> & mask_1d, double temperature, std::mt19937 & rng)
{
	const size_t K = masked_logits.size();
	const double neginf = -std::numeric_limits<double>::infinity();

	std::vector<double> scaled(K, neginf);
	for(size_t i = 0; i < K; ++i)
	{
		if(mask_1d.at(i))
		{
			scaled.at(i) = masked_logits.at(i) / temperature;
		}
	}

	const std::vector<double> probs = softmax(scaled);
	if(!std::ranges::all_of(
		   probs,
		   [](double prob)
		   {
			   return std::isfinite(prob);
		   }
	   ))
		throwf("sampling: non-finite probabilities");

	std::discrete_distribution<int> dist(probs.begin(), probs.end());
	const int idx_chosen = dist(rng);
	const double p_chosen = probs.at(static_cast<size_t>(idx_chosen));

	return {.index = idx_chosen, .prob = p_chosen, .fallback = false};
}

// Masked categorical sampling given a logits vector
SampleResult
sample_masked_logits(const std::vector<float> & logits_1d, const std::vector<int32_t> & mask_1d, bool throw_if_empty, double temperature, std::mt19937 & rng)
{
	const size_t K = logits_1d.size();
	if(K == 0 || mask_1d.size() != K)
		throwf("sampling: invalid logits/mask sizes");
	if(temperature < 0.0)
		throwf("sampling: negative temperature");

	const int n_valid = std::ranges::count_if(
		mask_1d,
		[](int v)
		{
			return v != 0;
		}
	);

	if(n_valid == 0)
	{
		if(throw_if_empty)
			throwf("sampling: no valid options available");
		return {.index = 0, .prob = 0.0, .fallback = true};
	}

	const std::vector<double> masked_logits = sampling::make_masked_logits(logits_1d, mask_1d);

	if(temperature > 1e8)
	{
		return sampling::sample_uniform_over_mask(mask_1d, n_valid, rng);
	}

	if(temperature < 1e-8)
	{
		const int idx_chosen = argmax(masked_logits);
		return {.index = idx_chosen, .prob = 1.0, .fallback = false};
	}

	return sampling::sample_softmax_over_mask(masked_logits, mask_1d, temperature, rng);
}

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
sample_triplet(const MaskedLogits & act0_logits, const MaskedLogits & hex1_logits, const MaskedLogits & hex2_logits, double temperature, std::mt19937 & rng)
{
	const std::vector<int64_t> s_a0 = shape_of(act0_logits.logits);
	const std::vector<int64_t> s_h1 = shape_of(hex1_logits.logits);
	const std::vector<int64_t> s_h2 = shape_of(hex2_logits.logits);
	const std::vector<int64_t> s_m0 = shape_of(act0_logits.mask);
	const std::vector<int64_t> s_m1 = shape_of(hex1_logits.mask);
	const std::vector<int64_t> s_m2 = shape_of(hex2_logits.mask);

	if(s_a0 != std::vector<int64_t>({1, 4}))
		throwf("sampling: act0_logits must be [1,4]");
	if(s_h1 != std::vector<int64_t>({1, 165}))
		throwf("sampling: hex1_logits must be [1,165]");
	if(s_h2 != std::vector<int64_t>({1, 165}))
		throwf("sampling: hex2_logits must be [1,165]");
	if(s_m0 != std::vector<int64_t>({1, 4}))
		throwf("sampling: mask_act0 must be [1,4]");
	if(s_m1 != std::vector<int64_t>({1, 4, 165}))
		throwf("sampling: mask_hex1 must be [1,4,165]");
	if(s_m2 != std::vector<int64_t>({1, 4, 165, 165}))
		throwf("sampling: mask_hex2 must be [1,4,165,165]");

	// Materialize host vectors and squeeze batch
	std::vector<float> a0_log = to_vector<float>(act0_logits.logits); // 4
	std::vector<float> h1_log = to_vector<float>(hex1_logits.logits); // 165
	std::vector<float> h2_log = to_vector<float>(hex2_logits.logits); // 165

	std::vector<int32_t> m_a0 = to_vector<int32_t>(act0_logits.mask); // 4
	std::vector<int32_t> m_h1 = to_vector<int32_t>(hex1_logits.mask); // 4*165
	std::vector<int32_t> m_h2 = to_vector<int32_t>(hex2_logits.mask); // 4*165*165

	// ---- act0 ----
	const SampleResult act0 = sample_masked_logits(a0_log, m_a0, true, temperature, rng);

	// ---- hex1 mask slice for chosen act0 ----
	const auto h1_row_offset = static_cast<size_t>(act0.index) * static_cast<size_t>(165);
	std::vector<int32_t> m_h1_for_act0(static_cast<size_t>(165), 0);
	for(size_t k = 0; k < static_cast<size_t>(165); ++k)
	{
		m_h1_for_act0.at(k) = m_h1.at(h1_row_offset + k);
	}

	// ---- hex1 ----
	const SampleResult hex1 = sample_masked_logits(h1_log, m_h1_for_act0, false, temperature, rng);

	// ---- hex2 mask slice for (act0, hex1) ----
	const size_t base = (act0.index * 165 + hex1.index) * static_cast<size_t>(165);
	std::vector<int32_t> m_h2_for_pair(static_cast<size_t>(165), 0);
	for(size_t k = 0; k < static_cast<size_t>(165); ++k)
	{
		m_h2_for_pair.at(k) = m_h2.at(base + k);
	}

	// ---- hex2 ----
	const SampleResult hex2 = sample_masked_logits(h2_log, m_h2_for_pair, false, temperature, rng);

	// ---- joint confidence ----
	const double confidence = act0.prob * (hex1.fallback ? 1.0 : hex1.prob) * (hex2.fallback ? 1.0 : hex2.prob);

	return {.act0 = act0.index, .hex1 = hex1.index, .hex2 = hex2.index, .confidence = confidence};
}
}
