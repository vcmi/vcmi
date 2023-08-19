/*
 * SThievesGuildInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"
#include "InfoAboutArmy.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE SThievesGuildInfo
{
	std::vector<PlayerColor> playerColors; //colors of players that are in-game

	std::vector< std::vector< PlayerColor > > numOfTowns, numOfHeroes, gold, woodOre, mercSulfCrystGems, obelisks, artifacts, army, income; // [place] -> [colours of players]

	std::map<PlayerColor, InfoAboutHero> colorToBestHero; //maps player's color to his best heros'

	std::map<PlayerColor, EAiTactic> personality; // color to personality // ai tactic
	std::map<PlayerColor, si32> bestCreature; // color to ID // id or -1 if not known

//	template <typename Handler> void serialize(Handler &h, const int version)
//	{
//		h & playerColors;
//		h & numOfTowns;
//		h & numOfHeroes;
//		h & gold;
//		h & woodOre;
//		h & mercSulfCrystGems;
//		h & obelisks;
//		h & artifacts;
//		h & army;
//		h & income;
//		h & colorToBestHero;
//		h & personality;
//		h & bestCreature;
//	}

};

VCMI_LIB_NAMESPACE_END
