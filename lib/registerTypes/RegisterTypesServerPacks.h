/*
 * RegisterTypesServerPacks.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../networkPacks/PacksForServer.h"

VCMI_LIB_NAMESPACE_BEGIN

class BinarySerializer;
class BinaryDeserializer;
class CTypeList;

template<typename Serializer>
void registerTypesServerPacks(Serializer &s)
{
	s.template registerType<CPack, CPackForServer>();
	s.template registerType<CPackForServer, EndTurn>();
	s.template registerType<CPackForServer, DismissHero>();
	s.template registerType<CPackForServer, MoveHero>();
	s.template registerType<CPackForServer, ArrangeStacks>();
	s.template registerType<CPackForServer, DisbandCreature>();
	s.template registerType<CPackForServer, BuildStructure>();
	s.template registerType<CPackForServer, RecruitCreatures>();
	s.template registerType<CPackForServer, UpgradeCreature>();
	s.template registerType<CPackForServer, GarrisonHeroSwap>();
	s.template registerType<CPackForServer, ExchangeArtifacts>();
	s.template registerType<CPackForServer, AssembleArtifacts>();
	s.template registerType<CPackForServer, BuyArtifact>();
	s.template registerType<CPackForServer, TradeOnMarketplace>();
	s.template registerType<CPackForServer, SetFormation>();
	s.template registerType<CPackForServer, HireHero>();
	s.template registerType<CPackForServer, BuildBoat>();
	s.template registerType<CPackForServer, QueryReply>();
	s.template registerType<CPackForServer, MakeAction>();
	s.template registerType<CPackForServer, DigWithHero>();
	s.template registerType<CPackForServer, CastAdvSpell>();
	s.template registerType<CPackForServer, CastleTeleportHero>();
	s.template registerType<CPackForServer, SaveGame>();
	s.template registerType<CPackForServer, PlayerMessage>();
	s.template registerType<CPackForServer, BulkSplitStack>();
	s.template registerType<CPackForServer, BulkMergeStacks>();
	s.template registerType<CPackForServer, BulkSmartSplitStack>();
	s.template registerType<CPackForServer, BulkMoveArmy>();
	s.template registerType<CPackForServer, BulkExchangeArtifacts>();
	s.template registerType < CPackForServer, ManageBackpackArtifacts>();
	s.template registerType<CPackForServer, EraseArtifactByClient>();
	s.template registerType<CPackForServer, GamePause>();
}

VCMI_LIB_NAMESPACE_END
