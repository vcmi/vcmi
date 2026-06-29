/*
 * NNModel.h, part of VCMI engine
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
#include "schema/base.h"
#include "schema/v13/types.h"

namespace MMAI::BAI
{

class NNModel : public MMAI::Schema::IModel
{
public:
	explicit NNModel(const std::string & path, float _temperature, uint64_t seed);

	Schema::ModelType getType() override;
	std::string getName() override;
	int getVersion() override;
	Schema::Side getSide() override;
	int getAction(const MMAI::Schema::IState * s) override;
	double getValue(const MMAI::Schema::IState * s) override;

private:
	std::string path;
	float temperature;
	std::string name;
	int version;
	Schema::Side side;

	std::mt19937 rng;
	Vec3D<int32_t> actionTable;

	// AllocatedStringPtrs manage the string lifetime
	// but names passed to model.Run must be const char*
	std::vector<Ort::AllocatedStringPtr> inputNamePtrs;
	std::vector<Ort::AllocatedStringPtr> outputNamePtrs;
	Vec3D<int32_t> bucketSizes;
	bool isDynamic;
	std::vector<const char *> inputNames;
	std::vector<const char *> outputNames;

	std::unique_ptr<Ort::Session> model = nullptr;
	Ort::AllocatorWithDefaultOptions allocator;
	Ort::MemoryInfo meminfo;

	std::vector<Ort::Value> prepareInputsV13(const MMAI::Schema::IState * state, const MMAI::Schema::V13::ISupplementaryData * sup);

	template<typename T>
	Ort::Value toTensor(const std::string & name, std::vector<T> & vec, const std::vector<int64_t> & shape);

	std::unique_ptr<Ort::Session> loadModel(const std::string & path, const Ort::SessionOptions & opts);
	int readVersion(const Ort::ModelMetadata & md) const;
	Schema::Side readSide(const Ort::ModelMetadata & md) const;
	Vec3D<int32_t> readBucketSizes(const Ort::ModelMetadata & md) const;
	Vec3D<int32_t> readActionTable(const Ort::ModelMetadata & md) const;
	bool readIsDynamic(const Ort::ModelMetadata & md) const;
	std::vector<const char *> readInputNames(int want);
	std::vector<const char *> readOutputNames();
};

}
