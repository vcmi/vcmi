/*
 * nn_model.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "nn_model.h"

#include "schema/base.h"
#include "schema/v15/constants.h"
#include "schema/v15/graph.h"
#include "vstd/CLoggerBase.h"
#include "json/JsonNode.h"

#include <onnxruntime_cxx_api.h>

namespace MMAI::BAI::V15
{

namespace
{
	namespace S15 = MMAI::Schema::V15;
	using ET = S15::Graph::ElementType;
	using IGraph = S15::Graph::IGraph;
	using INode = S15::Graph::INode;
	using IEdge = S15::Graph::IEdge;

	template<class... Args>
	[[noreturn]] inline void throwf(const std::string & fmt, Args &&... args)
	{
		boost::format f("NNModel: " + fmt);
		(void)std::initializer_list<int>{((f % std::forward<Args>(args)), 0)...};
		throw std::runtime_error(f.str());
	}

	template<typename T>
	void assertValidTensor(const std::string & name, const Ort::Value & tensor, int ndim)
	{
		auto type_info = tensor.GetTensorTypeAndShapeInfo();
		auto shape = type_info.GetShape();
		auto dtype = type_info.GetElementType();

		if(shape.size() != ndim)
			throwf("assertValidTensor: %s: bad ndim: want: %d, have: %d", name, ndim, shape.size());

		if constexpr(std::is_same_v<T, float>)
		{
			if(dtype != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
				throwf("assertValidTensor: %s: bad dtype: want: %d, have: %d", name, EI(ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT), EI(dtype));
		}
		else if constexpr(std::is_same_v<T, int>)
		{
			if(dtype != ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32)
				throwf("assertValidTensor: %s: bad dtype: want: %d, have: %d", name, EI(ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32), EI(dtype));
		}
		else if constexpr(std::is_same_v<T, int64_t>)
		{
			if(dtype != ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64)
				throwf("assertValidTensor: %s: bad dtype: want: %d, have: %d", name, EI(ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64), EI(dtype));
		}
		else if constexpr(std::is_same_v<T, bool>)
		{
			if(dtype != ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL)
				throwf("assertValidTensor: %s: bad dtype: want: %d, have: %d", name, EI(ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL), EI(dtype));
		}
		else
		{
			throwf("assertValidTensor: %s: can only work with bool, int and float", name);
		}
	}

	template<typename T>
	T toScalar(const std::string & name, const Ort::Value & tensor)
	{
		auto type_info = tensor.GetTensorTypeAndShapeInfo();
		auto shape = type_info.GetShape();
		auto dtype = type_info.GetElementType();

		if constexpr(std::is_same_v<T, float>)
		{
			if(dtype != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
				throwf("toScalar: %s: bad dtype: want: %d, have: %d", name, EI(ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT), EI(dtype));
		}
		else if constexpr(std::is_same_v<T, int64_t>)
		{
			if(dtype != ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64)
				throwf("toScalar: %s: bad dtype: want: %d, have: %d", name, EI(ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64), EI(dtype));
		}
		else
		{
			throwf("toScalar: %s: unsupported scalar type", name);
		}

		int64_t numel = 1;
		for(auto d : shape)
			numel *= d;

		if(numel != 1)
			throwf("toScalar: %s: bad numel: want: 1, have: %d", name, numel);

		return tensor.GetTensorData<T>()[0];
	}

	template<typename T>
	std::vector<T> toVec1D(const std::string & name, const Ort::Value & tensor, int64_t numel)
	{
		assertValidTensor<T>(name, tensor, 1);

		auto type_info = tensor.GetTensorTypeAndShapeInfo();
		auto shape = type_info.GetShape();

		if(shape.at(0) != numel)
			throwf("toVec1D: %s: bad numel: want: %d, have: %d", name, numel, shape.at(0));

		const T * data = tensor.GetTensorData<T>();

		auto res = std::vector<T>{};
		res.reserve(numel);
		res.assign(data, data + numel);
		return res;
	}

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

	struct Sample
	{
		int index;
		double confidence;
		double prob; // original (non-tempered) probability
	};

	std::pair<Sample, Sample> categorical(
		const std::vector<float> & probs,
		float temperature,
		std::mt19937 & rng,
		const std::vector<int64_t> & activeActionIds,
		const S15::Graph::IGraph * graph
	)
	{
		auto sample = Sample{};
		auto greedy = Sample{};

		if(temperature < 0.0f)
			throwf("sample: negative temperature");

		bool isGreedy = temperature < 1e-5;

		// Greedy sample: argmax, first tie.
		{
			int best = 0;
			for(int i = 0; i < probs.size(); ++i)
			{
				// Only print probs here if greedy is enabled
				// Otherwise, tempered probs are printed later instead
				if(isMMAIVerbose() && isGreedy)
				{
					const auto action = activeActionIds.at(i);
					const auto * actnode = graph->getNode(ET::NODE_ACTION, action);
					std::cout << "i=" << i << " action=" << action << " prob=" << std::fixed << std::setprecision(3) << probs[i] << " " << actnode->name()
							  << "\n";
				}

				if(probs[i] > probs[best])
					best = i; // '>' keeps the first tie
			}

			greedy.index = best;
			greedy.prob = probs[best];
			greedy.confidence = 1.0f;
		}

		if(isGreedy)
			return {greedy, greedy};

		// Stochastic sample (only if temperature > 0)
		// Sample with weights w_i = exp(log(p_i)/T), and return original probs[idx].
		std::vector<double> logw(probs.size(), -std::numeric_limits<double>::infinity());
		double max_logw = -std::numeric_limits<double>::infinity();
		bool valid = false;

		for(std::size_t i = 0; i < probs.size(); ++i)
		{
			float p = probs[i];
			if(p < 0.0f)
				throwf("sample: negative probabilities");

			if(p > 0.0f)
			{
				valid = true;
				double lw = std::log(p) / temperature;
				logw[i] = lw;
				max_logw = std::max(lw, max_logw);
			}
		}

		if(!valid)
			throwf("sample: all probabilities are 0");

		std::vector<double> weights(probs.size(), 0.0);
		double wsum = 0.0;

		for(std::size_t i = 0; i < probs.size(); ++i)
		{
			if(std::isfinite(logw[i]))
			{
				// shift by max for numerical stability
				double wi = std::exp(logw[i] - max_logw);
				weights[i] = wi;
				wsum += wi;
			}
		}

		if(wsum <= 0.0)
			throwf("sample: negative weight sum: %f", wsum);

		std::discrete_distribution<int> dist(weights.begin(), weights.end());

		if(isMMAIVerbose())
		{
			const auto actual_probs = dist.probabilities();
			for(std::size_t i = 0; i < actual_probs.size(); ++i)
			{
				const auto action = activeActionIds.at(i);
				const auto * actnode = graph->getNode(ET::NODE_ACTION, action);
				std::cout << "i=" << i << " action=" << action << " prob=" << std::fixed << std::setprecision(3) << actual_probs[i] << " " << actnode->name()
						  << "\n";
			}
		}

		int idx = dist(rng);
		sample.index = idx;
		sample.prob = probs[idx];
		sample.confidence = weights[idx] / wsum;

		return {sample, greedy};
	}

	ET nodeTypeByName(const std::string & name)
	{
		for(const auto & [type, candidateName, _size] : Schema::V15::NODE_TYPES)
			if(name == candidateName)
				return type;

		throwf("unknown node type name: %s", name);
	}

	int nodeAttrSize(ET type)
	{
		for(const auto & [candidateType, _name, size] : Schema::V15::NODE_TYPES)
			if(candidateType == type)
				return size;

		throwf("unknown node element type: %d", EU(type));
	}

	int edgeAttrSize(ET type)
	{
		for(const auto & [candidateType, _rel, _ends, size] : Schema::V15::EDGE_TYPES)
			if(candidateType == type)
				return size;

		throwf("unknown edge element type: %d", EU(type));
	}

	ET edgeTypeByNames(const std::string & srcName, const std::string & relName, const std::string & dstName)
	{
		const auto srcType = nodeTypeByName(srcName);
		const auto dstType = nodeTypeByName(dstName);

		for(const auto & [type, candidateRelName, ends, _size] : Schema::V15::EDGE_TYPES)
		{
			const auto & [candidateSrcType, candidateDstType] = ends;
			if(srcType == candidateSrcType && dstType == candidateDstType && relName == candidateRelName)
				return type;
		}

		throwf("unknown edge type tuple: (%s, %s, %s)", srcName, relName, dstName);
	}

	std::string readMetadata(const std::shared_ptr<NNContainer> & container, const std::string & key)
	{
		Ort::AllocatedStringPtr v = container->metadata.LookupCustomMetadataMapAllocated(key.c_str(), container->allocator);
		if(!v)
			throwf("metadata key '%s' missing", key);

		return std::string(v.get());
	}
}

Schema::Side NNModel::readSide() const
{
	/*
	 * side
	 *   dtype=int
	 *   shape=scalar
	 *
	 * Battlefield side the model was trained on (see Schema::Side enum).
	 */
	Schema::Side res;
	const auto vs = readMetadata(container, "side");
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

std::vector<NNModel::NodeInputSpec> NNModel::readNodeOrder() const
{
	/*
	 * node_order metadata from rl/v15/export.py, e.g.:
	 *   ["Global", "Player", "Unit", "Hex", "Action"]
	 */
	auto res = std::vector<NodeInputSpec>{};
	const auto jsonstr = readMetadata(container, "node_order");

	try
	{
		auto jn = JsonNode(jsonstr.data(), jsonstr.size(), "<ONNX metadata: node_order>");

		for(auto & jv : jn.Vector())
		{
			const auto & name = jv.String();
			const auto type = nodeTypeByName(name);
			res.push_back(NodeInputSpec{type, name, nodeAttrSize(type)});
		}
	}
	catch(const std::exception & e)
	{
		throwf(std::string("failed to parse 'node_order' JSON: ") + e.what());
	}

	if(res.empty())
		throwf("readNodeOrder: empty node_order");

	return res;
}

std::vector<NNModel::EdgeInputSpec> NNModel::readEdgeOrder() const
{
	/*
	 * edge_order metadata from rl/v15/export.py, e.g.:
	 *   [["Global", "To", "Player"], ...]
	 *
	 * It may omit ignored edges, so this metadata is the source of truth for
	 * the flattened edge input order.
	 */
	auto res = std::vector<EdgeInputSpec>{};
	const auto jsonstr = readMetadata(container, "edge_order");

	try
	{
		auto jn = JsonNode(jsonstr.data(), jsonstr.size(), "<ONNX metadata: edge_order>");

		for(auto & jv : jn.Vector())
		{
			const auto & parts = jv.Vector();
			if(parts.size() != 3)
				throwf("readEdgeOrder: bad edge tuple size: want: 3, have: %zu", parts.size());

			const auto srcName = parts[0].String();
			const auto relName = parts[1].String();
			const auto dstName = parts[2].String();
			const auto type = edgeTypeByNames(srcName, relName, dstName);
			res.push_back(EdgeInputSpec{type, srcName, relName, dstName, edgeAttrSize(type)});
		}
	}
	catch(const std::exception & e)
	{
		throwf(std::string("failed to parse 'edge_order' JSON: ") + e.what());
	}

	if(res.empty())
		throwf("readEdgeOrder: empty edge_order");

	return res;
}

std::vector<const char *> NNModel::readInputNames()
{
	/*
	 * Model inputs:
	 *   x_<node type> tensors, one for each node_order entry
	 *        dtype=float, shape=[N_node, node_attr_size]
	 *   edge_index_flat
	 *        dtype=int64, shape=[2, E*]
	 *   edge_attr_flat
	 *        dtype=float, shape=[E*, max edge attr size]
	 *   edge_lengths
	 *        dtype=int32, shape=[edge_order.size()]
	 *   active_action_ids
	 *        dtype=int64, shape=[A]
	 */
	std::vector<const char *> res;
	auto count = container->session->GetInputCount();
	const auto expectedCount = nodeOrder.size() + 4;
	if(count != expectedCount)
		throwf("wrong input count: want: %d, have: %lld", expectedCount, count);

	inputNamePtrs.reserve(count);
	res.reserve(count);
	for(size_t i = 0; i < count; ++i)
	{
		inputNamePtrs.emplace_back(container->session->GetInputNameAllocated(i, container->allocator));
		res.push_back(inputNamePtrs.back().get());
	}

	for(size_t i = 0; i < nodeOrder.size(); ++i)
	{
		const auto expectedName = "x_" + nodeOrder[i].name;
		if(expectedName != res[i])
			throwf("wrong input name at %d: want: %s, have: %s", i, expectedName, res[i]);
	}

	const std::array<std::string, 4> tailNames = {"edge_index_flat", "edge_attr_flat", "edge_lengths", "active_action_ids"};
	for(size_t i = 0; i < tailNames.size(); ++i)
	{
		const auto ind = nodeOrder.size() + i;
		if(tailNames[i] != res[ind])
			throwf("wrong input name at %d: want: %s, have: %s", ind, tailNames[i], res[ind]);
	}

	return res;
}

std::vector<const char *> NNModel::readOutputNames()
{
	/*
	 * Model outputs:
	 *   [0] action
	 *        dtype=int64, shape=[] or [1]
	 *   [1] active action probabilities
	 *        dtype=float, shape=[A]
	 *   [2] value
	 *        dtype=float, shape=[1]
	 *   [3] active action ids echo
	 *        dtype=int64, shape=[A]
	 */
	std::vector<const char *> res;
	auto count = container->session->GetOutputCount();
	if(count != 4)
		throwf("wrong output count: want: %d, have: %lld", 4, count);

	outputNamePtrs.reserve(count);
	res.reserve(count);

	for(size_t i = 0; i < count; ++i)
	{
		outputNamePtrs.emplace_back(container->session->GetOutputNameAllocated(i, container->allocator));
		res.push_back(outputNamePtrs.back().get());
	}

	const std::array<std::string, 4> expectedNames = {"action", "active_probs", "value", "active_action_ids_out"};
	for(size_t i = 0; i < expectedNames.size(); ++i)
		if(expectedNames[i] != res[i])
			throwf("wrong output name at %d: want: %s, have: %s", i, expectedNames[i], res[i]);

	return res;
}

NNModel::NNModel(const std::shared_ptr<NNContainer> & container, float temperature, uint64_t seed) : container(container), temperature(temperature)
{
	NestedLogTag _("NN");
	logAi->info("Params: seed=%1%, temperature=%2%, path=%3%", seed, temperature, container->path);

	if(container->version != version)
		throwf("Bad version: want: %d, have: %d", version, container->version);

	rng = std::mt19937(seed);
	side = readSide();
	nodeOrder = readNodeOrder();
	edgeOrder = readEdgeOrder();
	edgeAttrFlatSize = 0;
	for(const auto & edge : edgeOrder)
		edgeAttrFlatSize = std::max(edgeAttrFlatSize, edge.attrSize);
	inputNames = readInputNames();
	outputNames = readOutputNames();

	logAi->info("MMAI version %d initialized on side=%d", version, EI(side));
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
	NestedLogTag _("getAction");
	auto timer = ScopedTimer("call");
	auto any = s->getSupplementaryData();

	if(s->version() != version)
		throwf("getAction: unsupported IState version: want: %d, have: %d", version, s->version());

	if(!any.has_value())
		throw std::runtime_error("extractSupplementaryData: supdata is empty");
	auto err = MMAI::Schema::AnyCastError(any, typeid(const S15::ISupplementaryData *));
	if(!err.empty())
		throwf("getAction: anycast failed: %s", err);

	const auto * sup = std::any_cast<const S15::ISupplementaryData *>(any);
	if(!sup)
		throwf("getAction: null supplementary data");

	if(sup->getType() != S15::ISupplementaryData::Type::REGULAR)
		throwf("getAction: unsupported supplementary data type: %d", EI(sup->getType()));

	const auto * graph = sup->getGraph();
	if(!graph)
		throwf("getAction: null graph");

	const auto * gnode = graph->getNodes(ET::NODE_GLOBAL).at(0);
	using GA = S15::Graph::NodeAttributes::Global;

	if(gnode->rawAttributes().at(EU(GA::BATTLE_WINNER)) != EU(S15::CombatResult::NONE))
	{
		timer.name = boost::str(boost::format("MMAI action: %d (battle ended)") % MMAI::Schema::ACTION_RESET);
		return MMAI::Schema::ACTION_RESET;
	}

	auto inputs = prepareInputs(graph);
	auto outputs = container->session->Run(Ort::RunOptions(), inputNames.data(), inputs.data(), inputs.size(), outputNames.data(), outputNames.size());

	if(outputs.size() != 4)
		throwf("getAction: bad output size: want: 4, have: %d", outputs.size());

	const auto activeActionIds = graph->getActiveActionIds();
	const auto modelGreedyAction = toScalar<int64_t>("action", outputs[0]);
	const auto activeProbs = toVec1D<float>("active_probs", outputs[1], static_cast<int64_t>(activeActionIds.size()));
	const auto value = toScalar<float>("value", outputs[2]);
	const auto activeActionIdsOut = toVec1D<int64_t>("active_action_ids_out", outputs[3], static_cast<int64_t>(activeActionIds.size()));

	if(activeActionIds.empty())
		throwf("getAction: no active actions");

	if(activeActionIdsOut != activeActionIds)
		throwf("getAction: active action ids output mismatch");

	const auto [sample, greedy] = categorical(activeProbs, temperature, rng, activeActionIds, graph);

	if(sample.prob == 0)
		throwf("getAction: sample has 0 probability");

	const auto saction = activeActionIds.at(sample.index);
	const auto gaction = activeActionIds.at(greedy.index);

	if(modelGreedyAction != gaction)
		throwf("getAction: model greedy action mismatch: output: %d, probabilities: %d", modelGreedyAction, gaction);

	const auto sname = graph->getNode(ET::NODE_ACTION, saction)->name();
	const auto gname = graph->getNode(ET::NODE_ACTION, gaction)->name();

	logAi->debug(
		"greedy: %d (prob=%.2f conf=%.2f value=%.4f). Detail: active_index=%d %s", gaction, greedy.prob, greedy.confidence, value, greedy.index, gname
	);

	logAi->debug(
		"sample: %d (prob=%.2f conf=%.2f value=%.4f). Detail: active_index=%d %s", saction, sample.prob, sample.confidence, value, sample.index, sname
	);

	timer.name = boost::str(boost::format("MMAI action: %d (confidence=%.2f): %s") % saction % sample.confidence % sname);
	return static_cast<int>(saction);
};

double NNModel::getValue(const MMAI::Schema::IState * s)
{
	// This quantifies how good is the current state as perceived by the model
	// (not used, not implemented)
	return 0;
}

std::vector<Ort::Value> NNModel::prepareInputs(const S15::Graph::IGraph * graph)
{
	NestedLogTag _("prepareInputs");

	auto tensors = std::vector<Ort::Value>{};
	tensors.reserve(nodeOrder.size() + 4);

	std::ostringstream nodeShapeLog;

	for(const auto & nodeSpec : nodeOrder)
	{
		const auto nodes = graph->getNodes(nodeSpec.type);
		auto flat = std::vector<float>{};
		flat.resize(nodes.size() * static_cast<size_t>(nodeSpec.attrSize), 0.0f);

		for(size_t i = 0; i < nodes.size(); ++i)
		{
			auto out = std::span<float>(flat.data() + (i * static_cast<size_t>(nodeSpec.attrSize)), static_cast<size_t>(nodeSpec.attrSize));
			const auto encoded = nodes[i]->encode(out);
			if(encoded != nodeSpec.attrSize)
				throwf("unexpected encoded node size for %s: want: %d, have: %d", nodeSpec.name, nodeSpec.attrSize, encoded);
		}

		tensors.push_back(toTensor("x_" + nodeSpec.name, flat, {static_cast<int64_t>(nodes.size()), static_cast<int64_t>(nodeSpec.attrSize)}));

		nodeShapeLog << nodeSpec.name << "={" << nodes.size() << ", " << nodeSpec.attrSize << "} ";
	}

	auto lengths = std::vector<int>{};
	lengths.reserve(edgeOrder.size());

	auto ei_flat_src = std::vector<int64_t>{};
	auto ei_flat_dst = std::vector<int64_t>{};
	auto ea_flat = std::vector<float>{};

	std::ostringstream edgeLengthLog;

	for(const auto & edgeSpec : edgeOrder)
	{
		const auto edges = graph->getEdges(edgeSpec.type);
		lengths.push_back(static_cast<int>(edges.size()));
		edgeLengthLog << edgeSpec.srcName << "_" << edgeSpec.relName << "_" << edgeSpec.dstName << "=" << edges.size() << " ";

		for(const auto * edge : edges)
		{
			const auto [src, dst] = edge->endpoints();
			ei_flat_src.push_back(graph->getNodeIndex(src));
			ei_flat_dst.push_back(graph->getNodeIndex(dst));

			const auto oldSize = ea_flat.size();
			ea_flat.resize(oldSize + static_cast<size_t>(edgeAttrFlatSize), 0.0f);

			if(edgeSpec.attrSize > 0)
			{
				auto out = std::span<float>(ea_flat.data() + oldSize, static_cast<size_t>(edgeSpec.attrSize));
				const auto encoded = edge->encode(out);
				if(encoded != edgeSpec.attrSize)
					throwf(
						"unexpected encoded edge size for (%s, %s, %s): want: %d, have: %d",
						edgeSpec.srcName,
						edgeSpec.relName,
						edgeSpec.dstName,
						edgeSpec.attrSize,
						encoded
					);
			}
		}
	}

	const auto sumE = ei_flat_src.size();
	auto ei_flat = std::vector<int64_t>{};
	ei_flat.reserve(2 * sumE);
	ei_flat.insert(ei_flat.end(), ei_flat_src.begin(), ei_flat_src.end());
	ei_flat.insert(ei_flat.end(), ei_flat_dst.begin(), ei_flat_dst.end());

	if(ei_flat_dst.size() != sumE)
		throwf("unexpected ei_flat_dst.size(): want: %d, have: %d", sumE, ei_flat_dst.size());
	if(ea_flat.size() != sumE * static_cast<size_t>(edgeAttrFlatSize))
		throwf("unexpected ea_flat.size(): want: %d, have: %d", sumE * static_cast<size_t>(edgeAttrFlatSize), ea_flat.size());

	auto activeActionIds = graph->getActiveActionIds();
	if(activeActionIds.empty())
		throwf("prepareInputs: no active actions");

	tensors.push_back(toTensor("edge_index_flat", ei_flat, {2, static_cast<int64_t>(sumE)}));
	tensors.push_back(toTensor("edge_attr_flat", ea_flat, {static_cast<int64_t>(sumE), static_cast<int64_t>(edgeAttrFlatSize)}));
	tensors.push_back(toTensor("edge_lengths", lengths, {static_cast<int64_t>(edgeOrder.size())}));
	tensors.push_back(toTensor("active_action_ids", activeActionIds, {static_cast<int64_t>(activeActionIds.size())}));

	logAi->debug("Node shapes: " + nodeShapeLog.str());
	logAi->debug("Edge lengths: " + edgeLengthLog.str());
	logAi->debug(
		"Model input shapes: edgeIndex={2, %d} edgeAttrs={%d, %d} activeActionIds={%d}",
		sumE,
		sumE,
		edgeAttrFlatSize,
		static_cast<int64_t>(activeActionIds.size())
	);

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
	auto res = Ort::Value::CreateTensor<T>(container->allocator, shape.data(), shape.size());
	T * dst = res.template GetTensorMutableData<T>();
	if(!vec.empty())
		std::memcpy(dst, vec.data(), vec.size() * sizeof(T));
	return res;
}

} // namespace MMAI::BAI::V15
