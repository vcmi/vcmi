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
#include "schema/v15/types.h"

#include <onnxruntime_cxx_api.h>

namespace MMAI::BAI::V15
{

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
	struct NodeInputSpec
	{
		MMAI::Schema::V15::Graph::ElementType type;
		std::string name;
		int attrSize;
	};

	struct EdgeInputSpec
	{
		MMAI::Schema::V15::Graph::ElementType type;
		std::string srcName;
		std::string relName;
		std::string dstName;
		int attrSize;
	};

	std::shared_ptr<NNContainer> container;
	float temperature;
	Schema::Side side;
	std::mt19937 rng;

	const int version = 15;

	// AllocatedStringPtrs manage the string lifetime
	// but names passed to model.Run must be const char*
	std::vector<Ort::AllocatedStringPtr> inputNamePtrs;
	std::vector<Ort::AllocatedStringPtr> outputNamePtrs;
	std::vector<const char *> inputNames;
	std::vector<const char *> outputNames;
	std::vector<NodeInputSpec> nodeOrder;
	std::vector<EdgeInputSpec> edgeOrder;
	int edgeAttrFlatSize = 0;

	std::vector<Ort::Value> prepareInputs(const MMAI::Schema::V15::Graph::IGraph * graph);

	template<typename T>
	Ort::Value toTensor(const std::string & name, std::vector<T> & vec, const std::vector<int64_t> & shape);

	Schema::Side readSide() const;
	std::vector<NodeInputSpec> readNodeOrder() const;
	std::vector<EdgeInputSpec> readEdgeOrder() const;
	std::vector<const char *> readInputNames();
	std::vector<const char *> readOutputNames();
};

}
