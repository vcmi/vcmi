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
#include "../mapObjects/CompoundMapObjectID.h"

class Quest;
class CGObjectInstance;
class IGameInfoCallback;
class CGameInfoCallback;

struct DLL_LINKAGE QuestInfo //universal interface for human and AI
{
	/// Either a specific object (seer hut / quest guard), or an object type+subtype whose
	/// constructor owns a shared quest (border guards/gates of one colour).
	std::variant<ObjectInstanceID, CompoundMapObjectID> identity;

	QuestInfo() : identity(ObjectInstanceID()) {}
	explicit QuestInfo(ObjectInstanceID obj) : identity(obj) {}
	explicit QuestInfo(CompoundMapObjectID type) : identity(type) {}

	const Quest * getQuest(IGameInfoCallback *cb) const;
	/// True when this entry resolves to a quest with content that can be shown in the quest log.
	bool isDisplayable(IGameInfoCallback *cb) const;
	/// The source object for an instance quest; nullptr for a shared type quest.
	const CGObjectInstance * getObject(IGameInfoCallback *cb) const;

	/// True when this entry is backed by a concrete map object (not a shared type quest).
	bool hasObjectInstance() const
	{
		const auto * obj = std::get_if<ObjectInstanceID>(&identity);
		return obj && obj->hasValue();
	}

	/// Tiles to mark on the quest-log minimap: the source object(s) plus, where the
	/// limiter implies a target, kill targets, matching keymaster tents / artifact
	/// pickups, and heroes that already satisfy the limiter. Fog-gated by `cb`.
	std::vector<int3> getMarkerTiles(CGameInfoCallback *cb) const;

	bool operator== (const QuestInfo & qi) const
	{
		return identity == qi.identity;
	}

	template <typename Handler> void serialize(Handler &h)
	{
		if(!h.hasFeature(Handler::Version::QUEST_REWORK))
		{
			// legacy layout stored only the source object id
			ObjectInstanceID obj;
			h & obj;
			identity = obj;
			return;
		}

		bool typeQuest = false;
		if(h.saving)
			typeQuest = std::holds_alternative<CompoundMapObjectID>(identity);
		h & typeQuest;

		if(typeQuest)
		{
			// serialize the colour as an identifier so it survives mod renumbering
			si32 primary = 0;
			std::string subtype;
			if(h.saving)
			{
				const auto & cid = std::get<CompoundMapObjectID>(identity);
				primary = cid.primaryID;
				subtype = MapObjectSubID::encode(MapObjectID(primary), cid.secondaryID);
			}
			h & primary;
			h & subtype;
			if(!h.saving)
				identity = CompoundMapObjectID(primary, MapObjectSubID::decode(MapObjectID(primary), subtype));
		}
		else
		{
			ObjectInstanceID obj;
			if(h.saving)
				obj = std::get<ObjectInstanceID>(identity);
			h & obj;
			if(!h.saving)
				identity = obj;
		}
	}
};
