/*
 * factory.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "callback/CBattleGameInterface.h"
#include "schema/base.h"
#include <onnxruntime_cxx_api.h>

namespace MMAI::BAI
{

struct NNContainer
{
	std::unique_ptr<Ort::Session> session = nullptr;
	Ort::MemoryInfo meminfo;
	Ort::ModelMetadata metadata;
	OrtAllocator * allocator;
	int version;
};

// Factory method for versioned derived NNModel (e.g. BAI::V13::NNModel)
std::shared_ptr<MMAI::Schema::IModel> CreateNNModel(const std::string & path, float temperature = 1.0, uint64_t seed = 0);

std::shared_ptr<CBattleGameInterface>
CreateBAI(Schema::IModel * model, const std::shared_ptr<Environment> & env, const std::shared_ptr<CBattleCallback> & cb, bool enableSpellsUsage);

}
