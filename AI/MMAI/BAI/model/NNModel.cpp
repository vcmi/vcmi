/*
 * NNModel.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "BAI/model/util/bucketing.h"
#include "BAI/model/util/common.h"
#include "BAI/model/util/sampling.h"
#include "NNModel.h"
#include "filesystem/Filesystem.h"
#include "vstd/CLoggerBase.h"
#include "json/JsonNode.h"

#include <algorithm>
#include <onnxruntime_c_api.h>
#include <onnxruntime_cxx_api.h>

namespace MMAI::BAI
{

namespace
{
	struct ScopedTimer
	{
		std::string name;
		std::chrono::steady_clock::time_point t0;
		explicit ScopedTimer(const std::string & n) : name(n), t0(std::chrono::steady_clock::now()) {}

		ScopedTimer(const ScopedTimer &) = delete;
		ScopedTimer & operator=(const ScopedTimer &) = delete;
		ScopedTimer(ScopedTimer &&) = delete;
		ScopedTimer & operator=(ScopedTimer &&) = delete;
		~ScopedTimer()
		{
			auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
			logAi->info("%s: %lld ms", name, dt);
		}
	};

	std::array<std::vector<int32_t>, 165> buildNeighbourhoods_unpadded(const std::vector<int64_t> & dst)
	{
		// Validate and count degrees per node
		std::array<int, 165> deg{};
		for(auto e : dst)
		{
			auto v = static_cast<int>(e);
			if(v < 0 || v >= 165)
				throwf("dst contains node id out of range: %d", v);
			++deg[v];
		}

		std::array<std::vector<int32_t>, 165> res{};
		for(int v = 0; v < 165; ++v)
			res[v].reserve(deg[v]);
		for(size_t e = 0; e < dst.size(); ++e)
		{
			auto v = static_cast<int>(dst[e]);
			res[v].push_back(static_cast<int32_t>(e));
		}

		return res;
	}
}

std::unique_ptr<Ort::Session> NNModel::loadModel(const std::string & path, const Ort::SessionOptions & opts)
{
	static const auto env = Ort::Env{ORT_LOGGING_LEVEL_WARNING, "vcmi"};
	const auto rpath = ResourcePath(path, EResType::AI_MODEL);
	const auto * rhandler = CResourceHandler::get();
	if(!rhandler->existsResource(rpath))
		throwf("resource does not exist: %s", rpath.getName());

	const auto & [data, length] = rhandler->load(rpath)->readAll();
	return std::make_unique<Ort::Session>(env, data.get(), length, opts);
}

int NNModel::readVersion(const Ort::ModelMetadata & md) const
{
	/*
	 * version
	 *   dtype=int
	 *   shape=scalar
	 *
	 * Version of the model (current implementation is at version 13).
	 * If needed, NNModel may be extended to support other versions as well.
	 *
	 */
	int res = -1;

	Ort::AllocatedStringPtr v = md.LookupCustomMetadataMapAllocated("version", allocator);
	if(!v)
		throwf("readVersion: no such key");

	std::string vs(v.get());
	try
	{
		res = std::stoi(vs);
	}
	catch(...)
	{
		throwf("readVersion: not an int: %s", vs);
	}

	if(res != 13)
		throwf("readVersion: want: 13, have: %d (%s)", res, vs);

	return res;
}

Schema::Side NNModel::readSide(const Ort::ModelMetadata & md) const
{
	/*
	 * side
	 *   dtype=int
	 *   shape=scalar
	 *
	 * Battlefield side the model was trained on (see Schema::Side enum).
	 *
	 */
	Schema::Side res;
	Ort::AllocatedStringPtr v = md.LookupCustomMetadataMapAllocated("side", allocator);
	if(!v)
		throw std::runtime_error("metadata error: side: no such key");
	std::string vs(v.get());
	try
	{
		res = static_cast<Schema::Side>(std::stoi(vs));
	}
	catch(...)
	{
		throw std::runtime_error("metadata error: side: not an int");
	}

	return res;
}

Vec3D<int32_t> NNModel::readBucketSizes(const Ort::ModelMetadata & md) const
{
	/*
	 * all_sizes
	 *   dtype=int
	 *   shape=[5, 7, 2]:
	 *     d1: bucket size (S, M, L, XL, XXL)
	 *     d2: edge type (see Schema::V13::LinkType enum)
	 *     d3: pairs of [Emax, Kmax]:
	 *      Emax = max number of outbound node edges
	 *      Kmax = max number of inbound node edges
	 *
	 * Stats (10K steps):
	 *
	 *   Outbound edges (E)   avg   max   p99   p90   p75   p50   p25
	 * -----------------------------------------------------------------
	 *             ADJACENT   888   888   888   888   888   888   888
	 *                REACH   355   988   820   614   478   329   209
	 *           RANGED_MOD   408   2403  1285  646   483   322   162
	 *          ACTS_BEFORE   51    268   203   118   75    35    15
	 *        MELEE_DMG_REL   43    198   160   103   60    31    14
	 *        RETAL_DMG_REL   27    165   113   67    38    18    8
	 *       RANGED_DMG_REL   12    133   60    29    18    9     4
	 *
	 *    Inbound edges (K)   avg   max   p99   p90   p75   p50   p25
	 * -----------------------------------------------------------------
	 *             ADJACENT   5.4   6     6     6     6     6     6
	 *                REACH   2.2   13    10    8     6     4     3
	 *           RANGED_MOD   2.5   15    8     4     3     2     1
	 *          ACTS_BEFORE   0.3   23    19    15    12    8     5
	 *        MELEE_DMG_REL   0.3   10    9     8     7     5     3
	 *        RETAL_DMG_REL   0.2   10    9     8     6     5     3
	 *       RANGED_DMG_REL   0.1   8     6     3     2     2     1
	 *
	 * Approx. sizes are S=p50 / M=p90 / L=p99 / XL=max / XXL=2*max
	 * Exact values defined in the vcmi-gym project and are subject to change.
	 * NOTE: bucketed inputs are deprecated and will soon be removed.
	 *
	 */

	Vec3D<int32_t> res = {};
	Ort::AllocatedStringPtr ab = md.LookupCustomMetadataMapAllocated("all_sizes", allocator);
	if(!ab)
		throw std::runtime_error("metadata key 'all_sizes' missing");
	const std::string jsonstr(ab.get());
	try
	{
		auto jn = JsonNode(jsonstr.data(), jsonstr.size(), "<ONNX metadata: all_sizes>");

		if(!jn.isVector())
			throwf("readBucketSizes: bad JsonType: want: %d, have: %d", EI(JsonNode::JsonType::DATA_VECTOR), EI(jn.getType()));

		for(auto & jv0 : jn.Vector())
		{
			auto vec1 = std::vector<std::vector<int32_t>>{};
			for(auto & jv1 : jv0.Vector())
			{
				auto vec2 = std::vector<int32_t>{};
				for(auto & jv2 : jv1.Vector())
				{
					if(!jv2.isNumber())
					{
						throwf("readBucketSizes: invalid data type: want: %d, got: %d", EI(JsonNode::JsonType::DATA_INTEGER), EI(jv2.getType()));
					}
					vec2.push_back(static_cast<int32_t>(jv2.Integer()));
				}
				vec1.emplace_back(vec2);
			}
			res.emplace_back(vec1);
		}
	}
	catch(const std::exception & e)
	{
		throw std::runtime_error(std::string("readBucketSizes: failed to parse JSON: ") + e.what());
	}

	if(res.size() != 5)
		throwf("readBucketSizes: bad size for d1: want: 5, have: %zu", res.size());
	if(res[0].size() != 7)
		throwf("readBucketSizes: bad size for d2: want: 7, have: %zu", res[0].size());
	if(res[0][0].size() != 2)
		throwf("readBucketSizes: bad size for d3: want: 2, have: %zu", res[0][0].size());

	return res;
}

Vec3D<int32_t> NNModel::readActionTable(const Ort::ModelMetadata & md) const
{
	/*
	 * action_table
	 *   dtype=int
	 *   shape=[4, 165, 165]:
	 *     d1: action (WAIT, MOVE, AMOVE, SHOOT)
	 *     d2: target hex for MOVE, AMOVE (hex to move to) or SHOOT
	 *     d3: target hex for AMOVE (hex to melee-attack at after moving)
	 *
	 */

	Vec3D<int32_t> res = {};
	Ort::AllocatedStringPtr ab = md.LookupCustomMetadataMapAllocated("action_table", allocator);
	if(!ab)
		throwf("readActionTable: metadata key 'action_table' missing");
	const std::string jsonstr(ab.get());

	try
	{
		auto jn = JsonNode(jsonstr.data(), jsonstr.size(), "<ONNX metadata: all_sizes>");

		for(auto & jv0 : jn.Vector())
		{
			auto vec1 = std::vector<std::vector<int32_t>>{};
			for(auto & jv1 : jv0.Vector())
			{
				auto vec2 = std::vector<int32_t>{};
				for(auto & jv2 : jv1.Vector())
				{
					if(!jv2.isNumber())
					{
						throwf("invalid data type: want: %d, got: %d", EI(JsonNode::JsonType::DATA_INTEGER), EI(jv2.getType()));
					}
					vec2.push_back(static_cast<int32_t>(jv2.Integer()));
				}
				vec1.emplace_back(vec2);
			}
			res.emplace_back(vec1);
		}
	}
	catch(const std::exception & e)
	{
		throwf(std::string("failed to parse 'action_table' JSON: ") + e.what());
	}

	if(res.size() != 4)
		throwf("readActionTable: bad size for d1: want: 4, have: %zu", res.size());
	if(res[0].size() != 165)
		throwf("readActionTable: bad size for d2: want: 165, have: %zu", res[0].size());
	if(res[0][0].size() != 165)
		throwf("readActionTable: bad size for d3: want: 165, have: %zu", res[0][0].size());

	return res;
}

bool NNModel::readIsDynamic(const Ort::ModelMetadata & md) const
{
	/*
	 * is_dynamic
	 *   dtype=int
	 *   shape=scalar
	 *
	 * Might not be present on older models (return false in this case).
	 */

	Ort::AllocatedStringPtr v = md.LookupCustomMetadataMapAllocated("is_dynamic", allocator);
	return v && std::string(v.get()) == "1";
}

std::vector<const char *> NNModel::readInputNames(int want)
{
	/*
	 * Model inputs (4):
	 *   [0] battlefield state
	 *        dtype=float
	 *        shape=[S] where S=Schema::V13::BATTLEFIELD_STATE_SIZE
	 * 	 [1] edge index
	 *        dtype=int32
	 *        shape=[2, E*] where E is the number of edges
	 * 	 [2] edge attributes
	 *        dtype=float
	 *        shape=[E*, 1] where E
	 * 	 [3] node neighbourhoods
	 *        dtype=int
	 *        shape=[165, K*] where K is the max number of inbound edges per hex
	 * 	 [4] size
	 *        dtype=int
	 *        shape=[7, 2]
	 */
	std::vector<const char *> res;
	auto count = model->GetInputCount();
	if(count != want)
		throwf("wrong input count: want: %d, have: %lld", want, count);

	inputNamePtrs.reserve(count);
	res.reserve(count);
	for(size_t i = 0; i < count; ++i)
	{
		inputNamePtrs.emplace_back(model->GetInputNameAllocated(i, allocator));
		res.push_back(inputNamePtrs.back().get());
	}

	return res;
}

std::vector<const char *> NNModel::readOutputNames()
{
	/*
	 * Model outputs (10):
     *   [0] greedy action
	 *        dtype=int
	 *        shape=[1]
     *   [1] main action logits (see readActionTable, d0)
	 *        dtype=float
	 *        shape=[4]
     *   [2] hex#1 logits (see readActionTable, d1)
	 *        dtype=float
	 *        shape=[165]
     *   [3] hex#2 logits (see readActionTable, d2)
	 *        dtype=float
	 *        shape=[165]
     *   [4] main action mask
	 *        dtype=int
	 *        shape=[4]
     *   [5] hex#1 mask
	 *        dtype=int
	 *        shape=[165]
     *   [6] hex#2 mask
	 *        dtype=int
	 *        shape=[165]
     *   [7] greedy main action
	 *        dtype=int
	 *        shape=[1]
     *   [8] greedy hex1
	 *        dtype=int
	 *        shape=[1]
     *   [9] greedy hex2
	 *        dtype=int
	 *        shape=[1]
	 *
	 * The greedy output values are unused since their stochastic counterparts
	 * are sampled here instead (see sampling::sample_triplet).
	 */
	std::vector<const char *> res;
	auto count = model->GetOutputCount();
	if(count != 10)
		throwf("wrong output count: want: %d, have: %lld", count, count);

	outputNamePtrs.reserve(count);
	res.reserve(count);

	for(size_t i = 0; i < count; ++i)
	{
		outputNamePtrs.emplace_back(model->GetOutputNameAllocated(i, allocator));
		res.push_back(outputNamePtrs.back().get());
	}

	return res;
}

/*
 * XXX:
 * hex1_logits and hex2_logits are based on a greedy act0.
 * However, if temp > 0 and a non-greedy act0 is chosen,
 * the hex logits become inconsistent with the chosen action.
 * As a temporary workaround, force greedy actions with temperature = 0.
 * Proper fix would require:
 * 1) re-exporting the model, changing its output dimensions to
 *    [4, 165] and [4, 165, 165] for hex1_logits and hex2_logits respectively
 * 2) changing the logic here to pick the proper hex logits after sampling
 */
NNModel::NNModel(const std::string & path, float _temperature, uint64_t seed)
	: path(path), temperature(0), meminfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault))
{
	logAi->info("MMAI: NNModel params: seed=%1%, temperature=%2%, model=%3%", seed, temperature, path);

	if(seed == 0)
	{
		seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		logAi->info("Generated new seed: %1%", seed);
	}

	rng = std::mt19937(seed);

	auto opts = Ort::SessionOptions();
	opts.SetIntraOpNumThreads(4);
	opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

	model = loadModel(path, opts);

	auto md = model->GetModelMetadata();
	version = readVersion(md);
	side = readSide(md);
	actionTable = readActionTable(md);
	bucketSizes = readBucketSizes(md);
	isDynamic = readIsDynamic(md);
	inputNames = readInputNames(isDynamic ? 5 : 4);
	outputNames = readOutputNames();

	logAi->info("MMAI version %d initialized on side=%d (dynamic=%d)", version, EI(side), isDynamic);
}

Schema::ModelType NNModel::getType()
{
	return Schema::ModelType::NN;
};

std::string NNModel::getName()
{
	return "MMAI_MODEL";
};

int NNModel::getVersion()
{
	return version;
};

Schema::Side NNModel::getSide()
{
	return side;
};

int NNModel::getAction(const MMAI::Schema::IState * s)
{
	auto timer = ScopedTimer("getAction");
	auto any = s->getSupplementaryData();

	if(s->version() != version)
		throwf("getAction: unsupported IState version: want: %d, have: %d", version, s->version());

	if(!any.has_value())
		throw std::runtime_error("extractSupplementaryData: supdata is empty");
	auto err = MMAI::Schema::AnyCastError(any, typeid(const MMAI::Schema::V13::ISupplementaryData *));
	if(!err.empty())
		throwf("getAction: anycast failed: %s", err);

	const auto * sup = std::any_cast<const MMAI::Schema::V13::ISupplementaryData *>(any);

	if(sup->getIsBattleEnded())
	{
		timer.name = boost::str(boost::format("MMAI action: %d (battle ended)") % MMAI::Schema::ACTION_RESET);
		return MMAI::Schema::ACTION_RESET;
	}

	auto inputs = prepareInputsV13(s, sup);
	auto outputs = model->Run(Ort::RunOptions(), inputNames.data(), inputs.data(), inputs.size(), outputNames.data(), outputNames.size());

	if(outputs.size() != 10)
		throwf("getAction: bad output size: want: 10, have: %d", outputs.size());

	// Deterministic (greedy) action
	auto action = toVector<int32_t>("getAction: t_action", outputs[0], 1).at(0);

	timer.name = "MMAI action: " + std::to_string(action);

	// Stochastic action (used instead of the greedy action if temperature > 0)
	if(temperature > 1e-8)
	{
		auto sample = sampling::sample_triplet(
			MaskedLogits{.logits = outputs[1], .mask = outputs[4]}, // act0 [4]
			MaskedLogits{.logits = outputs[2], .mask = outputs[5]}, // hex1 [165]
			MaskedLogits{.logits = outputs[3], .mask = outputs[6]}, // hex2 [165]
			temperature,
			rng
		);

		auto s_action = actionTable.at(sample.act0).at(sample.hex1).at(sample.hex2);

		if(s_action != action)
			logAi->debug("Sampled a non-greedy action: %d with confidence=%.2f", s_action, sample.confidence);

		timer.name = boost::str(boost::format("MMAI action: %d (confidence=%.2f)") % s_action % sample.confidence);
		action = s_action;
	}

	return static_cast<MMAI::Schema::Action>(action);
};

double NNModel::getValue(const MMAI::Schema::IState * s)
{
	// This quantifies how good is the current state as perceived by the model
	// (not used, not implemented)
	return 0;
}

std::vector<Ort::Value> NNModel::prepareInputsV13(const MMAI::Schema::IState * s, const MMAI::Schema::V13::ISupplementaryData * sup)
{
	auto containers = std::array<IndexContainer, LT_COUNT>{};

	int count = 0;

	for(const auto & [type, links] : sup->getAllLinks())
	{
		// assert order
		if(EI(type) != count)
			throwf("unexpected link type: want: %d, have: %d", count, EI(type));

		auto & c = containers.at(count);

		const auto srcinds = links->getSrcIndex();
		const auto dstinds = links->getDstIndex();
		const auto attrs = links->getAttributes();

		auto nlinks = srcinds.size();

		if(dstinds.size() != nlinks)
			throwf("unexpected dstinds.size() for LinkType(%d): want: %d, have: %d", EI(type), nlinks, dstinds.size());

		if(attrs.size() != nlinks)
			throwf("unexpected attrs.size() for LinkType(%d): want: %d, have: %d", EI(type), nlinks, attrs.size());

		c.edgeIndex.at(0).reserve(nlinks);
		c.edgeIndex.at(1).reserve(nlinks);
		c.edgeIndex.at(0).insert(c.edgeIndex.at(0).end(), srcinds.begin(), srcinds.end());
		c.edgeIndex.at(1).insert(c.edgeIndex.at(1).end(), dstinds.begin(), dstinds.end());

		c.edgeAttrs.reserve(nlinks);
		c.edgeAttrs.insert(c.edgeAttrs.end(), attrs.begin(), attrs.end());

		c.neighbourhoods = buildNeighbourhoods_unpadded(dstinds);

		++count;
	}

	if(count != LT_COUNT)
		throwf("unexpected links count: want: %d, have: %d", LT_COUNT, count);

	auto bdata = bucketing::BucketBuilder(containers, bucketSizes).build_bucket_data(isDynamic);

	const auto * state = s->getBattlefieldState();
	auto estate = std::vector<float>(state->size());
	std::ranges::copy(*state, estate.begin());

	int sum_e = bdata.edgeIndex_flat.at(0).size();
	int sum_k = bdata.neighbourhoods_flat.at(0).size();

	if(bdata.edgeIndex_flat.at(0).size() != sum_e)
		throwf("unexpected bdata.edgeIndex_flat.at(0).size(): want: %d, have: %d", sum_e, bdata.edgeIndex_flat.at(0).size());
	if(bdata.edgeIndex_flat.at(1).size() != sum_e)
		throwf("unexpected bdata.edgeIndex_flat.at(1).size(): want: %d, have: %d", sum_e, bdata.edgeIndex_flat.at(1).size());
	if(bdata.edgeAttrs_flat.size() != sum_e)
		throwf("unexpected bdata.edgeAttrs_flat.size(): want: %d, have: %d", sum_e, bdata.edgeAttrs_flat.size());

	for(int i = 0; i < 165; ++i)
	{
		if(bdata.neighbourhoods_flat.at(i).size() != sum_k)
			throwf("unexpected bdata.neighbourhoods_flat.at(%d).size(): want: %d, have: %d", i, sum_k, bdata.neighbourhoods_flat.at(i).size());
	}

	auto edgeIndex_flat = std::vector<int32_t>{};
	edgeIndex_flat.reserve(2 * sum_e);
	for(auto & ei : bdata.edgeIndex_flat)
		edgeIndex_flat.insert(edgeIndex_flat.end(), ei.begin(), ei.end());

	auto neighbourhoods = std::vector<int32_t>{};
	neighbourhoods.reserve(165 * sum_k);
	for(auto & nbr : bdata.neighbourhoods_flat)
		neighbourhoods.insert(neighbourhoods.end(), nbr.begin(), nbr.end());

	auto tensors = std::vector<Ort::Value>{};
	tensors.push_back(toTensor("state", estate, {static_cast<int64_t>(estate.size())}));
	tensors.push_back(toTensor("edgeIndex_flat", edgeIndex_flat, {2, sum_e}));
	tensors.push_back(toTensor("edgeAttrs_flat", bdata.edgeAttrs_flat, {sum_e, 1}));
	tensors.push_back(toTensor("nbr_flat", neighbourhoods, {165, sum_k}));

	if(isDynamic)
	{
		auto size = std::vector<int64_t>{};
		size.reserve(EI(LT_COUNT) * 2);
		for(int i = 0; i < EI(LT_COUNT); ++i)
		{
			size.push_back(bdata.size.emax.at(i));
			size.push_back(bdata.size.kmax.at(i));
		}
		tensors.push_back(toTensor("size", size, {EI(LT_COUNT), 2}));
	}

	logAi->debug("Model input shapes: state={%d} edgeIndex={2, %d} edgeAttrs={%d, 1} nbr={165, %d}", estate.size(), sum_e, sum_e, sum_k);

	return tensors;
}

template<typename T>
Ort::Value NNModel::toTensor(const std::string & name, std::vector<T> & vec, const std::vector<int64_t> & shape)
{
	// Sanity check
	int64_t numel = 1;
	for(int64_t d : shape)
		numel *= d;

	if(numel != vec.size())
		throwf("toTensor: %s: numel check failed: want: %d, have: %d", name, numel, vec.size());

	// Create a memory-owning tensor then copy data
	auto res = Ort::Value::CreateTensor<T>(allocator, shape.data(), shape.size());
	T * dst = res.template GetTensorMutableData<T>();
	std::memcpy(dst, vec.data(), vec.size() * sizeof(T));
	return res;
}

} // namespace MMAI::BAI
