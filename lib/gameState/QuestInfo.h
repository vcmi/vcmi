/*
 * QuestInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "int3.h"
#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class CQuest;
class CGObjectInstance;
class CGameInfoCallback;

struct DLL_LINKAGE QuestInfo //universal interface for human and AI
{
	ObjectInstanceID obj; //related object, Seer Hut or Border Guard

	QuestInfo() = default;
	explicit QuestInfo(ObjectInstanceID Obj)
		: obj(Obj)
	{}

	const CQuest * getQuest(CGameInfoCallback *cb) const;
	const CGObjectInstance * getObject(CGameInfoCallback *cb) const;
	int3 getPosition(CGameInfoCallback *cb) const;

	bool operator== (const QuestInfo & qi) const
	{
		return obj == qi.obj;
	}

	template <typename Handler> void serialize(Handler &h)
	{
		h & obj;
	}
};

VCMI_LIB_NAMESPACE_END
