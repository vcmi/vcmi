/*
 * CSerializer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CSerializer.h"

#include "../CGameState.h"
#include "../mapping/CMap.h"
#include "../CHeroHandler.h"
#include "../mapObjects/CGHeroInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

//must be instantiated in .cpp file for access to complete types of all member fields
CSerializer::~CSerializer() = default;

void CSerializer::addStdVecItems(CGameState *gs, LibClasses *lib)
{
	registerVectoredType<CGObjectInstance, ObjectInstanceID>(&gs->map->objects,
		[](const CGObjectInstance &obj){ return obj.id; });
	registerVectoredType<CHero, HeroTypeID>(&lib->heroh->objects,
		[](const CHero &h){ return h.getId(); });
	registerVectoredType<CGHeroInstance, HeroTypeID>(&gs->map->allHeroes,
		[](const CGHeroInstance &h){ return h.type->getId(); });
	registerVectoredType<CCreature, CreatureID>(&lib->creh->objects,
		[](const CCreature &cre){ return cre.getId(); });
	registerVectoredType<CArtifact, ArtifactID>(&lib->arth->objects,
		[](const CArtifact &art){ return art.getId(); });
	registerVectoredType<CArtifactInstance, ArtifactInstanceID>(&gs->map->artInstances,
		[](const CArtifactInstance &artInst){ return artInst.id; });
	registerVectoredType<CQuest, si32>(&gs->map->quests,
		[](const CQuest &q){ return q.qid; });

	smartVectorMembersSerialization = true;
}

VCMI_LIB_NAMESPACE_END
