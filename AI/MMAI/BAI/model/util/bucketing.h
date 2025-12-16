/*
 * bucketing.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BAI/model/util/common.h"

namespace MMAI::BAI::bucketing
{
struct Requirements
{
	std::array<size_t, LT_COUNT> e_req{};
	std::array<size_t, LT_COUNT> k_req{};
};

struct BucketChoice
{
	int index = -1;
	std::array<int32_t, LT_COUNT> emax{};
	std::array<int32_t, LT_COUNT> kmax{};
};

struct BucketData
{
	int size_index = -1; // chosen index in all_sizes
	std::array<int32_t, LT_COUNT> emax{}; // chosen emax per link type
	std::array<int32_t, LT_COUNT> kmax{}; // chosen kmax per link type

	std::vector<float> edgeAttrs_flat; // length sum(emax)
	std::array<std::vector<int32_t>, 2> edgeIndex_flat; // each length sum(emax)
	std::array<std::vector<int32_t>, 165> neighbourhoods_flat; // each length sum(kmax)
};

class BucketBuilder
{
public:
	BucketBuilder(const std::array<IndexContainer, LT_COUNT> & containers, const std::vector<std::vector<std::vector<int32_t>>> & all_sizes);

	BucketData build_bucket_data() const;

private:
	const std::array<IndexContainer, LT_COUNT> & containers_;
	const std::vector<std::vector<std::vector<int32_t>>> & all_sizes_;

	Requirements compute_requirements() const;
	bool bucket_satisfies(const std::vector<std::vector<int32_t>> & sz, const Requirements & req) const;

	void log_bucket_choice(int chosen, const Requirements & req) const;
	BucketChoice choose_bucket(const Requirements & req) const;
	void build_edges_flat(const std::array<int32_t, LT_COUNT> & emax, BucketData & bdata) const;
	void build_neighbors_flat(const std::array<int32_t, LT_COUNT> & kmax, BucketData & bdata) const;
};
}
