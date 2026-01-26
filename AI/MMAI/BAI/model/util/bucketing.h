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

constexpr int ID_NOT_FOUND = -1;
constexpr int ID_DYNAMIC = -2;

struct BucketSize
{
	std::array<int32_t, LT_COUNT> emax{};
	std::array<int32_t, LT_COUNT> kmax{};
};

struct BucketData
{
	BucketSize req;
	BucketSize size;
	int id = ID_NOT_FOUND;
	std::vector<float> edgeAttrs_flat; // length sum(emax)
	std::array<std::vector<int32_t>, 2> edgeIndex_flat; // each length sum(emax)
	std::array<std::vector<int32_t>, 165> neighbourhoods_flat; // each length sum(kmax)
};

class BucketBuilder
{
public:
	BucketBuilder(const std::array<IndexContainer, LT_COUNT> & containers, const std::vector<std::vector<std::vector<int32_t>>> & all_sizes);

	BucketData build_bucket_data(bool dynamic) const;

private:
	const std::array<IndexContainer, LT_COUNT> & containers_;
	const std::vector<std::vector<std::vector<int32_t>>> & all_sizes_;

	bool bucket_satisfies(const std::vector<std::vector<int32_t>> & sz, const BucketSize & req) const;
	void log_bucket_data(const BucketSize & req, const BucketData & bdata) const;

	BucketSize compute_requirements() const;
	BucketData build_minimal_bucket(const BucketSize & req) const;
	BucketData choose_bucket(const BucketSize & req) const;
	void build_edges_flat(BucketData & bdata) const;
	void build_neighbors_flat(BucketData & bdata) const;
};
}
