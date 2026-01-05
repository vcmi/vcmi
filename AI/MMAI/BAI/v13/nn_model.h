/*
 * nn_model.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BAI/factory.h"
#include "schema/base.h"
#include "schema/v13/types.h"

#include <onnxruntime_cxx_api.h>

namespace MMAI::BAI::V13
{

template<class T>
using Vec2D = std::vector<std::vector<T>>;

template<class T>
using Vec3D = std::vector<std::vector<std::vector<T>>>;

class NNModel : public MMAI::Schema::IModel
{

public:
	explicit NNModel(const std::shared_ptr<NNContainer> & container, float temperature, uint64_t seed);

	Schema::ModelType getType() override;
	std::string getName() override;
	int getVersion() override;
	Schema::Side getSide() override;
	int getAction(const MMAI::Schema::IState * s) override;
	double getValue(const MMAI::Schema::IState * s) override;

private:
	std::shared_ptr<NNContainer> container;
	float temperature;
	Schema::Side side;
	std::mt19937 rng;

	const int version = 13;

	// AllocatedStringPtrs manage the string lifetime
	// but names passed to model.Run must be const char*
	std::vector<Ort::AllocatedStringPtr> inputNamePtrs;
	std::vector<Ort::AllocatedStringPtr> outputNamePtrs;
	std::vector<const char *> inputNames;
	std::vector<const char *> outputNames;
	Vec3D<int32_t> actionTable;

	std::vector<Ort::Value> prepareInputsV13(const MMAI::Schema::IState * state, const MMAI::Schema::V13::ISupplementaryData * sup);

	template<typename T>
	Ort::Value toTensor(const std::string & name, std::vector<T> & vec, const std::vector<int64_t> & shape);

	Schema::Side readSide() const;
	Vec3D<int32_t> readActionTable() const;
	std::vector<const char *> readInputNames();
	std::vector<const char *> readOutputNames();
};

}
