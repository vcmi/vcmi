/*
 * bucketing.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BAI/model/util/bucketing.h"

#include <boost/range/numeric.hpp>

namespace MMAI::BAI::bucketing
{
BucketBuilder::BucketBuilder(const std::array<IndexContainer, LT_COUNT> & containers, const std::vector<std::vector<std::vector<int32_t>>> & all_sizes)
	: containers_(containers), all_sizes_(all_sizes)
{
}

BucketData BucketBuilder::build_bucket_data() const
{
	BucketData bdata{};

	const Requirements req = compute_requirements();
	const BucketChoice choice = choose_bucket(req);

	if(choice.index < 0)
		throwf("too many units on the battlefield");

	bdata.size_index = choice.index;
	bdata.emax = choice.emax;
	bdata.kmax = choice.kmax;

	build_edges_flat(bdata.emax, bdata);
	build_neighbors_flat(bdata.kmax, bdata);

	return bdata;
}

Requirements BucketBuilder::compute_requirements() const
{
	Requirements req{};
	for(int l = 0; l < LT_COUNT; ++l)
	{
		req.e_req[l] = containers_[l].edgeAttrs.size();
		size_t km = 0;

		for(int v = 0; v < 165; ++v)
			km = std::max(km, containers_[l].neighbourhoods[v].size());

		req.k_req[l] = km;
	}
	return req;
}

bool BucketBuilder::bucket_satisfies(const std::vector<std::vector<int32_t>> & sz, const Requirements & req) const
{
	if(static_cast<int>(sz.size()) != LT_COUNT)
		return false;

	for(int l = 0; l < LT_COUNT; ++l)
	{
		if(sz[l].size() != 2)
			return false;

		const int32_t emax_l = sz[l][0];
		const int32_t kmax_l = sz[l][1];

		if(emax_l < static_cast<int32_t>(req.e_req[l]) || kmax_l < static_cast<int32_t>(req.k_req[l]))
		{
			return false;
		}
	}
	return true;
}

void BucketBuilder::log_bucket_choice(int chosen, const Requirements & req) const
{
	logAi->debug("Size: %d", chosen);
	for(int i = 0; i < LT_COUNT; ++i)
	{
		logAi->debug("  %d: [%ld, %ld] -> [%lld, %lld]", i, req.e_req[i], req.k_req[i], all_sizes_[chosen][i][0], all_sizes_[chosen][i][1]);
	}
}

BucketChoice BucketBuilder::choose_bucket(const Requirements & req) const
{
	BucketChoice choice{};

	for(int s = 0; s < static_cast<int>(all_sizes_.size()); ++s)
	{
		const auto & sz = all_sizes_[static_cast<size_t>(s)];
		if(!bucket_satisfies(sz, req))
			continue;

		choice.index = s;
		for(int l = 0; l < LT_COUNT; ++l)
		{
			choice.emax[l] = sz[l][0];
			choice.kmax[l] = sz[l][1];
		}
		break;
	}

	if(choice.index >= 0)
		log_bucket_choice(choice.index, req);

	return choice;
}

void BucketBuilder::build_edges_flat(const std::array<int32_t, LT_COUNT> & emax, BucketData & bdata) const
{
	const size_t sum_emax = boost::accumulate(emax, static_cast<size_t>(0));

	bdata.edgeIndex_flat.at(0).clear();
	bdata.edgeIndex_flat.at(1).clear();
	bdata.edgeAttrs_flat.clear();

	bdata.edgeIndex_flat.at(0).reserve(sum_emax);
	bdata.edgeIndex_flat.at(1).reserve(sum_emax);
	bdata.edgeAttrs_flat.reserve(sum_emax);

	for(int l = 0; l < LT_COUNT; ++l)
	{
		const auto & edgeIndex = containers_[l].edgeIndex;
		const auto & edgeAttrs = containers_[l].edgeAttrs;

		bdata.edgeIndex_flat.at(0).insert(bdata.edgeIndex_flat.at(0).end(), edgeIndex.at(0).begin(), edgeIndex.at(0).end());
		bdata.edgeIndex_flat.at(1).insert(bdata.edgeIndex_flat.at(1).end(), edgeIndex.at(1).begin(), edgeIndex.at(1).end());
		bdata.edgeAttrs_flat.insert(bdata.edgeAttrs_flat.end(), edgeAttrs.begin(), edgeAttrs.end());

		auto need = static_cast<size_t>(emax[l]) - edgeIndex.at(0).size();
		if(need > 0)
			bdata.edgeIndex_flat.at(0).insert(bdata.edgeIndex_flat.at(0).end(), need, 0);

		need = static_cast<size_t>(emax[l]) - edgeIndex.at(1).size();
		if(need > 0)
			bdata.edgeIndex_flat.at(1).insert(bdata.edgeIndex_flat.at(1).end(), need, 0);

		need = static_cast<size_t>(emax[l]) - edgeAttrs.size();
		if(need > 0)
			bdata.edgeAttrs_flat.insert(bdata.edgeAttrs_flat.end(), need, 0.0f);
	}

	if(bdata.edgeIndex_flat.at(0).size() != sum_emax)
		throwf("edgeIndex_flat.at(0) size mismatch: want: %d, have: %zu", sum_emax, bdata.edgeIndex_flat.at(0).size());

	if(bdata.edgeIndex_flat.at(1).size() != sum_emax)
		throwf("edgeIndex_flat.at(1) size mismatch: want: %d, have: %zu", sum_emax, bdata.edgeIndex_flat.at(1).size());

	if(bdata.edgeAttrs_flat.size() != sum_emax)
		throwf("edgeAttrs_flat size mismatch: want: %d, have: %zu", sum_emax, bdata.edgeAttrs_flat.size());
}

void BucketBuilder::build_neighbors_flat(const std::array<int32_t, LT_COUNT> & kmax, BucketData & bdata) const
{
	const size_t sum_kmax = boost::accumulate(kmax, static_cast<size_t>(0));

	for(int v = 0; v < 165; ++v)
	{
		auto & dst = bdata.neighbourhoods_flat[static_cast<size_t>(v)];
		dst.clear();
		dst.reserve(sum_kmax);

		for(int l = 0; l < LT_COUNT; ++l)
		{
			const auto & src = containers_[l].neighbourhoods[v];
			dst.insert(dst.end(), src.begin(), src.end());
			const auto need = static_cast<size_t>(kmax[l]) - src.size();
			if(need > 0)
				dst.insert(dst.end(), need, -1);
		}

		if(dst.size() != sum_kmax)
			throwf("neighbourhoods_flat row size mismatch: want: %zu, have: %zu", sum_kmax, dst.size());
	}
}
}
