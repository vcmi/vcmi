/*
 * QuestInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "QuestInfo.h"

#include "../callback/IGameInfoCallback.h"
#include "../callback/CGameInfoCallback.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapObjects/Quest.h"

const Quest * QuestInfo::getQuest(IGameInfoCallback *cb) const
{
	const Quest * quest = getObject(cb)->activeQuestForLog();
	assert(quest);

	return quest;
}

const CGObjectInstance * QuestInfo::getObject(IGameInfoCallback *cb) const
{
	return cb->getObjInstance(obj);
}

int3 QuestInfo::getPosition(IGameInfoCallback *cb) const
{
	return getObject(cb)->visitablePos();
}

std::vector<int3> QuestInfo::getMarkerTiles(CGameInfoCallback *cb) const
{
	std::vector<int3> result;

	const Rewardable::Limiter & limiter = getQuest(cb)->mission;

	// the quest source itself (kept visible to the holder via the fog-of-war override)
	result.push_back(getPosition(cb));

	// objects the player must destroy (kill creature / kill hero)
	for(const auto & targetID : limiter.destroyedObjects)
		if(const auto * target = cb->getObj(targetID, false))
			result.push_back(target->visitablePos());

	// Heroes are matched by the limiter itself (heroAllowed), which unifies the
	// level / primary / hero-type / hero-class / creatures / artifact cases into one
	// predicate. We only sweep for heroes when the limiter actually constrains them.
	const bool markArtifacts = !limiter.artifacts.empty();
	const bool markHeroes = markArtifacts
		|| limiter.heroLevel > 0
		|| limiter.heroExperience > 0
		|| std::any_of(limiter.primary.begin(), limiter.primary.end(), [](si32 v){ return v > 0; })
		|| !limiter.heroes.empty()
		|| !limiter.heroClasses.empty()
		|| !limiter.creatures.empty();
	const bool markKeys = !limiter.requiredKeys.empty();

	if(markArtifacts || markHeroes || markKeys)
	{
		for(const auto * visitable : cb->getAllVisitableObjs())
		{
			if(markHeroes && visitable->ID == Obj::HERO)
			{
				if(limiter.heroAllowed(dynamic_cast<const CGHeroInstance *>(visitable)))
					result.push_back(visitable->visitablePos());
			}
			else if(markArtifacts && visitable->ID == Obj::ARTIFACT)
			{
				if(const auto * art = dynamic_cast<const CGArtifact *>(visitable))
					if(vstd::contains(limiter.artifacts, art->getArtifactType()))
						result.push_back(visitable->visitablePos());
			}
			else if(markKeys && visitable->ID == Obj::KEYMASTER)
			{
				if(vstd::contains(limiter.requiredKeys, visitable->subID))
					result.push_back(visitable->visitablePos());
			}
		}
	}

	return result;
}
