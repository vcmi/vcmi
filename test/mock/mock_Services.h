/*
 * mock\mock_Services.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once


#include <vcmi/Services.h>

class ServicesMock : public Services
{
public:
	MOCK_CONST_METHOD0(artifacts, const ArtifactService *());
	MOCK_CONST_METHOD0(creatures, const CreatureService *());
	MOCK_CONST_METHOD0(factions, const FactionService *());
	MOCK_CONST_METHOD0(heroClasses, const HeroClassService *());
	MOCK_CONST_METHOD0(heroTypes, const HeroTypeService *());
#if SCRIPTING_ENABLED
	MOCK_CONST_METHOD0(scripts, const scripting::Service *());
#endif
	MOCK_CONST_METHOD0(spells, const spells::Service *());
	MOCK_CONST_METHOD0(skills, const SkillService * ());
	MOCK_CONST_METHOD0(battlefields, const BattleFieldService *());
	MOCK_CONST_METHOD0(obstacles, const ObstacleService *());
	MOCK_CONST_METHOD0(settings, const IGameSettings *());

	MOCK_METHOD3(updateEntity, void(Metatype, int32_t, const JsonNode &));

	MOCK_CONST_METHOD0(spellEffects, const spells::effects::Registry *());
	MOCK_METHOD0(spellEffects, spells::effects::Registry *());
};


