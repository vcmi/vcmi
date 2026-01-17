/*
 * factory.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "factory.h"
#include "callback/CBattleGameInterface.h"
#include "filesystem/Filesystem.h"
#include <onnxruntime_c_api.h>

#include "BAI/v13/BAI.h"
#include "BAI/v13/nn_model.h"

namespace MMAI::BAI
{

namespace
{
	std::unique_ptr<Ort::Session> load(const std::string & path)
	{
		/*
         * IMPORTANT:
         * There seems to be an UB in the model unless either of the below is set:
         *  a) GraphOptimizationLevel::ORT_DISABLE_ALL
         *  b) DisableMemPattern
         *
         * Mem pattern does not impact performance => disable.
         * Graph optimization causes < 30% speedup => not worth the risk, disable.
         *
         */
		auto opts = Ort::SessionOptions();
		opts.DisableMemPattern();
		opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_DISABLE_ALL);
		opts.SetExecutionMode(ORT_SEQUENTIAL); // ORT_SEQUENTIAL = no inter-op parallelism
		opts.SetIntraOpNumThreads(1); // Inter-op threads matter in ORT_PARALLEL
		opts.SetIntraOpNumThreads(4); // Parallelism inside kernels/operators

		static const auto env = Ort::Env{ORT_LOGGING_LEVEL_WARNING, "vcmi"};
		const auto rpath = ResourcePath(path, EResType::AI_MODEL);
		const auto * rhandler = CResourceHandler::get();
		if(!rhandler->existsResource(rpath))
			throw std::runtime_error("NNBase: resource does not exist: " + rpath.getName());

		const auto & [data, length] = rhandler->load(rpath)->readAll();
		return std::make_unique<Ort::Session>(env, data.get(), length, opts);
	}

	int readVersion(Ort::Session * session, OrtAllocator * allocator, const Ort::ModelMetadata & md)
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
			throw std::runtime_error("NNBase: readVersion: no such key");

		std::string vs(v.get());
		try
		{
			res = std::stoi(vs);
		}
		catch(...)
		{
			throw std::runtime_error("NNBase: readVersion: not an int: " + vs);
		}

		return res;
	}

	std::shared_ptr<NNContainer> CreateNNContainer(const std::string & path)
	{
		auto session = load(path);
		auto meminfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
		auto metadata = session->GetModelMetadata();
		auto allocator = Ort::AllocatorWithDefaultOptions();
		auto version = readVersion(session.get(), allocator, metadata);
		return std::make_shared<NNContainer>(std::move(session), std::move(meminfo), std::move(metadata), std::move(allocator), version);
	}

}

// Factory method for versioned derived NNModel (e.g. NNModel::V1)
std::shared_ptr<MMAI::Schema::IModel> CreateNNModel(const std::string & path, float temperature, uint64_t seed)
{
	auto container = CreateNNContainer(path);

	if(container->version == 13)
		return std::make_shared<V13::NNModel>(container, temperature, seed);
	else
		throw std::runtime_error("CreateNNModel: unsupported schema version: " + std::to_string(container->version));
}

// Factory method for versioned derived BAI (e.g. BAI::V1)
std::shared_ptr<CBattleGameInterface>
CreateBAI(Schema::IModel * model, const std::shared_ptr<Environment> & env, const std::shared_ptr<CBattleCallback> & cb, bool enableSpellsUsage)
{
	std::shared_ptr<CBattleGameInterface> res;
	auto version = model->getVersion();

	if(version == 13)
		return std::make_shared<V13::BAI>(model, version, env, cb, enableSpellsUsage);
	else
		throw std::runtime_error("CreateBAI: unsupported schema version: " + std::to_string(version));

	return res;
}

}
