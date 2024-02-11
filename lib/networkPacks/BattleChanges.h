/*
 * BattleChanges.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class BattleChanges
{
public:
	enum class EOperation : si8
	{
		ADD,
		RESET_STATE,
		UPDATE,
		REMOVE,
	};

	JsonNode data;
	EOperation operation = EOperation::RESET_STATE;

	BattleChanges() = default;
	explicit BattleChanges(EOperation operation_)
		: operation(operation_)
	{
	}
};

class UnitChanges : public BattleChanges
{
public:
	uint32_t id = 0;
	int64_t healthDelta = 0;

	UnitChanges() = default;
	UnitChanges(uint32_t id_, EOperation operation_)
		: BattleChanges(operation_)
		, id(id_)
	{
	}

	template <typename Handler> void serialize(Handler & h)
	{
		h & id;
		h & healthDelta;
		h & data;
		h & operation;
	}
};

class ObstacleChanges : public BattleChanges
{
public:
	uint32_t id = 0;

	ObstacleChanges() = default;

	ObstacleChanges(uint32_t id_, EOperation operation_)
		: BattleChanges(operation_),
		id(id_)
	{
	}

	template <typename Handler> void serialize(Handler & h)
	{
		h & id;
		h & data;
		h & operation;
	}
};

VCMI_LIB_NAMESPACE_END

