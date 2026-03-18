/*
 * RmgFuzzSpec.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "FuzzEnvironment.h"

#include "../lib/mapping/CMap.h"

#include <ctime>
#include <memory>

namespace fuzzing
{
struct RmgGenerationSpec
{
	int seed;
	bool singleThread;
	int parallelism;
	std::time_t creationDateTime;
};

RmgGenerationSpec decodeRmgGenerationSpec(ByteReader & input);
std::unique_ptr<CMap> generateMap(const RmgGenerationSpec & spec);
std::unique_ptr<CMap> generateMapWithParallelism(const RmgGenerationSpec & spec, int parallelism);
}
