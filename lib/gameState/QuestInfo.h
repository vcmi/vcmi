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
#include "../mapObjects/CGObjectInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

class CQuest;
class CGObjectInstance;
class IGameInfoCallback;

struct DLL_LINKAGE QuestInfo //universal interface for human and AI
{
	ObjectInstanceID obj; //related object, Seer Hut or Border Guard

	QuestInfo() = default;
	explicit QuestInfo(ObjectInstanceID Obj)
		: obj(Obj)
	{}

	const CQuest * getQuest(IGameInfoCallback *cb) const;
	const CGObjectInstance * getObject(IGameInfoCallback *cb) const;
	int3 getPosition(IGameInfoCallback *cb) const;

	bool operator== (const QuestInfo & qi) const
	{
		return obj == qi.obj;
	}

	template <typename Handler> void serialize(Handler &h)
	{

		if (h.hasFeature(Handler::Version::NO_RAW_POINTERS_IN_SERIALIZER))
		{
			h & obj;
		}
		else
		{
			std::shared_ptr<CQuest> questUnused;
			std::shared_ptr<CGObjectInstance> objectPtr;
			int3 tileUnused;
			h & questUnused;
			h & objectPtr;
			h & tileUnused;

			if (objectPtr)
				obj = objectPtr->id;
		}
	}
};

VCMI_LIB_NAMESPACE_END
