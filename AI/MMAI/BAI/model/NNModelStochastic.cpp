/*
 * NNModelStochastic.cpp, part of VCMI engine
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
#include "NNModelStochastic.h"
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
    std::vector<T> toVec1D(const std::string & name, const Ort::Value & tensor, int numel)
    {
        assertValidTensor<T>(name, tensor, 1);

        auto type_info = tensor.GetTensorTypeAndShapeInfo();
        auto shape = type_info.GetShape();

        if(shape.at(0) != numel)
            throwf("toVec1D: %s: bad numel: want: %d, have: %d", name, numel, shape.at(0));

        const T * data = tensor.GetTensorData<T>();

        auto res = std::vector<T>{};
        res.reserve(numel);
        res.assign(data, data + numel); // v now owns a copy
        return res;
    }

    template<typename T>
    Vec2D<T> toVec2D(const std::string & name, const Ort::Value & tensor, const std::pair<int64_t, int64_t> & dims)
    {
        assertValidTensor<T>(name, tensor, 2);

        const auto & [d0, d1] = dims;
        auto type_info = tensor.GetTensorTypeAndShapeInfo();
        auto shape = type_info.GetShape();

        if(shape.at(0) != d0)
            throwf("toVec2D: %s: bad dim0: want: %d, have: %d", name, d0, shape.at(0));
        if(shape.at(1) != d1)
            throwf("toVec2D: %s: bad dim1: want: %d, have: %d", name, d1, shape.at(1));

        const T * data = tensor.GetTensorData<T>();

        auto res = Vec2D<T>{};
        res.resize(static_cast<size_t>(d0));
        for(auto i = 0; i < d0; ++i)
        {
            auto & row = res[i];
            row.resize(d1);
            std::memcpy(row.data(), data + i * d1, static_cast<size_t>(d1) * sizeof(T));
        }
        return res;
    }

    struct Sample
    {
        int index;
        double confidence;
        double prob; // original (non-tempered) probability
    };

    std::pair<Sample, Sample> categorical(const std::vector<float> & probs, float temperature, std::mt19937 & rng)
    {
        auto sample = Sample{};
        auto greedy = Sample{};

        if(temperature < 0.0f)
            throwf("sample: negative temperature");

        // Greedy sample: argmax, first tie.
        {
            int best = 0;
            for(int i = 0; i < probs.size(); ++i)
                if(probs[i] > probs[best])
                    best = i; // '>' keeps the first tie

            greedy.index = best;
            greedy.prob = probs[best];
            greedy.confidence = 1.0f;
        }

        if(temperature < 1e-5)
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
        int idx = dist(rng);

        sample.index = idx;
        sample.prob = probs[idx];
        sample.confidence = weights[idx] / wsum;

        return {sample, greedy};
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
}

std::unique_ptr<Ort::Session> NNModelStochastic::loadModel(const std::string & path, const Ort::SessionOptions & opts)
{
    static const auto env = Ort::Env{ORT_LOGGING_LEVEL_WARNING, "vcmi"};
    const auto rpath = ResourcePath(path, EResType::AI_MODEL);
    const auto * rhandler = CResourceHandler::get();
    if(!rhandler->existsResource(rpath))
        throwf("resource does not exist: %s", rpath.getName());

    const auto & [data, length] = rhandler->load(rpath)->readAll();
    return std::make_unique<Ort::Session>(env, data.get(), length, opts);
}

int NNModelStochastic::readVersion(const Ort::ModelMetadata & md) const
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

Schema::Side NNModelStochastic::readSide(const Ort::ModelMetadata & md) const
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

Vec3D<int32_t> NNModelStochastic::readActionTable(const Ort::ModelMetadata & md) const
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

std::vector<const char *> NNModelStochastic::readInputNames()
{
    /*
     * Model inputs (4):
     *   [0] battlefield state
     *        dtype=float
     *        shape=[S] where S=Schema::V13::BATTLEFIELD_STATE_SIZE
     *   [1] edge index
     *        dtype=int32
     *        shape=[2, E*] where E is the number of edges
     *   [2] edge attributes
     *        dtype=float
     *        shape=[E*, 1]
     *   [3] lengths
     *        dtype=int
     *        shape=[LT_COUNT]
     */
    std::vector<const char *> res;
    auto count = model->GetInputCount();
    if(count != 4)
        throwf("wrong input count: want: %d, have: %lld", 4, count);

    inputNamePtrs.reserve(count);
    res.reserve(count);
    for(size_t i = 0; i < count; ++i)
    {
        inputNamePtrs.emplace_back(model->GetInputNameAllocated(i, allocator));
        res.push_back(inputNamePtrs.back().get());
    }

    return res;
}

std::vector<const char *> NNModelStochastic::readOutputNames()
{
    /*
     * Model outputs (6):
     *   [0] main action probabilities (see readActionTable, d0)
     *        dtype=float
     *        shape=[4]
     *   [1] hex#1 probabilities (see readActionTable, d1)
     *        dtype=float
     *        shape=[4, 165]
     *   [2] hex#2 probabilities (see readActionTable, d2)
     *        dtype=float
     *        shape=[165, 165]
     *   [3] main action mask
     *        dtype=int
     *        shape=[4]
     *   [4] hex#1 mask
     *        dtype=int
     *        shape=[4, 165]
     *   [5] hex#2 mask
     *        dtype=int
     *        shape=[165, 165]
     */
    std::vector<const char *> res;
    auto count = model->GetOutputCount();
    if(count != 6)
        throwf("wrong output count: want: %d, have: %lld", 6, count);

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
NNModelStochastic::NNModelStochastic(const std::string & path, float temperature, uint64_t seed)
    : path(path), temperature(temperature), meminfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault))
{
    logAi->info("MMAI: NNModel params: seed=%1%, temperature=%2%, model=%3%", seed, temperature, path);

    rng = std::mt19937(seed);

    /*
     * IMPORTANT:
     * There seems to be an UB in the model unless either (or both):
     *  a) DisableMemPattern
     *  b) GraphOptimizationLevel::ORT_DISABLE_ALL
     *
     * Mem pattern does not impact performance => disable.
     * Graph optimization causes < 30% speedup => not worth the risk, disable.
     *
     */
    auto opts = Ort::SessionOptions();
    opts.DisableMemPattern();
    opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_DISABLE_ALL);
    opts.SetExecutionMode(ORT_SEQUENTIAL); // ORT_SEQUENTIAL = no inter-op parallelism
    opts.SetInterOpNumThreads(1); // Inter-op threads matter in ORT_PARALLEL
    opts.SetIntraOpNumThreads(4); // Parallelism inside kernels/operators

    model = loadModel(path, opts);

    auto md = model->GetModelMetadata();
    version = readVersion(md);
    side = readSide(md);
    actionTable = readActionTable(md);
    inputNames = readInputNames();
    outputNames = readOutputNames();

    logAi->info("MMAI: version %d initialized on side=%d (stochastic=1)", version, EI(side));
}

Schema::ModelType NNModelStochastic::getType()
{
    return Schema::ModelType::NN;
};

std::string NNModelStochastic::getName()
{
    return "MMAI_MODEL";
};

int NNModelStochastic::getVersion()
{
    return version;
};

Schema::Side NNModelStochastic::getSide()
{
    return side;
};

int NNModelStochastic::getAction(const MMAI::Schema::IState * s)
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

    if(outputs.size() != 6)
        throwf("getAction: bad output size: want: 6, have: %d", outputs.size());

    const auto act0_probs = toVec1D<float>("act0_probs", outputs[0], 4); // WAIT, MOVE, AMOVE, SHOOT
    const auto hex1_probs = toVec2D<float>("hex1_probs", outputs[1], {4, 165});
    const auto hex2_probs = toVec2D<float>("hex2_probs", outputs[2], {165, 165});
    const auto act0_mask = toVec1D<int>("act0_mask", outputs[3], 4); // WAIT, MOVE, AMOVE, SHOOT
    const auto hex1_mask = toVec2D<int>("hex1_mask", outputs[4], {4, 165});
    const auto hex2_mask = toVec2D<int>("hex2_mask", outputs[5], {165, 165});

    const auto [act0_sample, act0_greedy] = categorical(act0_probs, temperature, rng);
    const auto [hex1_sample, hex1_greedy] = categorical(hex1_probs.at(act0_sample.index), temperature, rng);
    const auto [hex2_sample, hex2_greedy] = categorical(hex2_probs.at(hex1_sample.index), temperature, rng);

    if(act0_sample.prob == 0)
        throwf("getAction: act0_sample has 0 probability");
    else if(act0_mask.at(act0_sample.index) == 0)
        throwf("getAction: act0_sample is masked out");

    // Hex1 is always needed if act0 != 0 (WAIT)
    if(act0_sample.index > 0)
    {
        if(hex1_sample.prob == 0)
            throwf("getAction: hex1_sample has 0 probability");
        else if(hex1_mask.at(act0_sample.index).at(hex1_sample.index) == 0)
            throwf("getAction: hex1_sample is masked out");
    }

    // Hex2 is only needed if act0 == 2 (AMOVE)
    if(act0_sample.index == 2)
    {
        if(hex2_sample.prob == 0)
            throwf("getAction: hex2_sample has 0 probability");
        else if(hex2_mask.at(hex1_sample.index).at(hex2_sample.index) == 0)
            throwf("getAction: hex2_sample is masked out");
    }

    const auto & saction = actionTable.at(act0_sample.index).at(hex1_sample.index).at(hex2_sample.index);
    const auto & gaction = actionTable.at(act0_greedy.index).at(hex1_greedy.index).at(hex2_greedy.index);

    const auto & mask = s->getActionMask();
    if(!mask->at(saction))
        throwf("getAction: sampled action is masked"); // Incorrect mask?

    auto sconf = act0_sample.confidence * hex1_sample.confidence * hex2_sample.confidence;
    auto sprob = act0_sample.prob * hex1_sample.prob * hex2_sample.prob;

    auto gconf = act0_greedy.confidence * hex1_greedy.confidence * hex2_greedy.confidence;
    auto gprob = act0_greedy.prob * hex1_greedy.prob * hex2_greedy.prob;

    auto fmt = boost::format("%s: %d (prob=%.2f conf=%.2f). Detail: [%d %d %d] (prob=[%.2f %.2f %.2f] conf=[%.2f %.2f %.2f])");

    logAi->debug(
        boost::str(
            fmt % "MMAI (greedy)" % gaction % gprob % gconf % act0_greedy.index % hex1_greedy.index % hex2_greedy.index % act0_greedy.prob % hex1_greedy.prob
            % hex2_greedy.prob % act0_greedy.confidence % hex1_greedy.confidence % hex2_greedy.confidence
        )
    );

    logAi->debug(
        boost::str(
            fmt % "MMAI (sample)" % saction % sprob % sconf % act0_sample.index % hex1_sample.index % hex2_sample.index % act0_sample.prob % hex1_sample.prob
            % hex2_sample.prob % act0_sample.confidence % hex1_sample.confidence % hex2_sample.confidence
        )
    );

    timer.name = boost::str(boost::format("MMAI action: %d (confidence=%.2f)") % saction % sconf);
    return saction;
};

double NNModelStochastic::getValue(const MMAI::Schema::IState * s)
{
    // This quantifies how good is the current state as perceived by the model
    // (not used, not implemented)
    return 0;
}

std::vector<Ort::Value> NNModelStochastic::prepareInputsV13(const MMAI::Schema::IState * s, const MMAI::Schema::V13::ISupplementaryData * sup)
{
    auto lengths = std::vector<int>{};
    lengths.reserve(LT_COUNT);

    auto ei_flat_src = std::vector<int>{};
    auto ei_flat_dst = std::vector<int>{};
    auto ea_flat = std::vector<float>{};

    std::ostringstream oss;
    int i = 0;

    for(const auto & [type, links] : sup->getAllLinks())
    {
        // assert order
        if(EI(type) != i)
            throwf("unexpected link type: want: %d, have: %d", i, EI(type));

        const auto & srcinds = links->getSrcIndex();
        const auto & dstinds = links->getDstIndex();
        const auto & attrs = links->getAttributes();

        const auto nlinks = srcinds.size();

        if(dstinds.size() != nlinks)
            throwf("unexpected dstinds.size() for LinkType(%d): want: %d, have: %d", EI(type), nlinks, dstinds.size());

        if(attrs.size() != nlinks)
            throwf("unexpected attrs.size() for LinkType(%d): want: %d, have: %d", EI(type), nlinks, attrs.size());

        oss << nlinks << " ";

        lengths.push_back(static_cast<int>(nlinks));

        ei_flat_src.insert(ei_flat_src.end(), srcinds.begin(), srcinds.end());
        ei_flat_dst.insert(ei_flat_dst.end(), dstinds.begin(), dstinds.end());
        ea_flat.insert(ea_flat.end(), attrs.begin(), attrs.end());
        ++i;
    }

    if(i != LT_COUNT)
        throwf("unexpected links count: want: %d, have: %d", LT_COUNT, i);

    auto sum_e = ei_flat_src.size();
    auto ei_flat = std::vector<int64_t>{};

    ei_flat.reserve(2 * sum_e);
    ei_flat.insert(ei_flat.end(), ei_flat_src.begin(), ei_flat_src.end());
    ei_flat.insert(ei_flat.end(), ei_flat_dst.begin(), ei_flat_dst.end());

    const auto * state = s->getBattlefieldState();
    auto estate = std::vector<float>(state->size());
    std::ranges::copy(*state, estate.begin());

    auto tensors = std::vector<Ort::Value>{};
    tensors.push_back(toTensor("obs", estate, {static_cast<int64_t>(estate.size())}));
    tensors.push_back(toTensor("ei_flat", ei_flat, {2, static_cast<int64_t>(sum_e)}));
    tensors.push_back(toTensor("ea_flat", ea_flat, {static_cast<int64_t>(sum_e), 1}));
    tensors.push_back(toTensor("lengths", lengths, {LT_COUNT}));

    logAi->debug("NNModel: Edge lengths: [ " + oss.str() + "]");
    logAi->debug("NNModel: Input shapes: state={%d} edgeIndex={2, %d} edgeAttrs={%d, 1}", estate.size(), sum_e, sum_e);

    return tensors;
}

template<typename T>
Ort::Value NNModelStochastic::toTensor(const std::string & name, std::vector<T> & vec, const std::vector<int64_t> & shape)
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
