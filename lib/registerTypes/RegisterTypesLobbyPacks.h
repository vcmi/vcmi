/*
 * RegisterTypesLobbyPacks.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../networkPacks/PacksForLobby.h"
#include "../gameState/CGameState.h"
#include "../campaign/CampaignState.h"
#include "../mapping/CMapInfo.h"
#include "../rmg/CMapGenOptions.h"
#include "../gameState/TavernHeroesPool.h"
#include "../gameState/CGameStateCampaign.h"
#include "../mapping/CMap.h"
#include "../TerrainHandler.h"
#include "../RiverHandler.h"
#include "../RoadHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

template<typename Serializer>
void registerTypesLobbyPacks(Serializer &s)
{
	s.template registerType<CPack, CPackForLobby>();
	s.template registerType<CPackForLobby, CLobbyPackToPropagate>();
	s.template registerType<CPackForLobby, CLobbyPackToServer>();

	// Any client can sent
	s.template registerType<CLobbyPackToPropagate, LobbyClientConnected>();
	s.template registerType<CLobbyPackToPropagate, LobbyClientDisconnected>();
	s.template registerType<CLobbyPackToPropagate, LobbyChatMessage>();
	// Only host client send
	s.template registerType<CLobbyPackToPropagate, LobbyGuiAction>();
	s.template registerType<CLobbyPackToPropagate, LobbyLoadProgress>();
	s.template registerType<CLobbyPackToPropagate, LobbyRestartGame>();
	s.template registerType<CLobbyPackToPropagate, LobbyPrepareStartGame>();
	s.template registerType<CLobbyPackToPropagate, LobbyStartGame>();
	s.template registerType<CLobbyPackToPropagate, LobbyChangeHost>();
	// Only server send
	s.template registerType<CLobbyPackToPropagate, LobbyUpdateState>();
	s.template registerType<CLobbyPackToPropagate, LobbyShowMessage>();

	// For client with permissions
	s.template registerType<CLobbyPackToServer, LobbyChangePlayerOption>();
	// Only for host client
	s.template registerType<CLobbyPackToServer, LobbySetMap>();
	s.template registerType<CLobbyPackToServer, LobbySetCampaign>();
	s.template registerType<CLobbyPackToServer, LobbySetCampaignMap>();
	s.template registerType<CLobbyPackToServer, LobbySetCampaignBonus>();
	s.template registerType<CLobbyPackToServer, LobbySetPlayer>();
	s.template registerType<CLobbyPackToServer, LobbySetPlayerName>();
	s.template registerType<CLobbyPackToServer, LobbySetTurnTime>();
	s.template registerType<CLobbyPackToServer, LobbySetSimturns>();
	s.template registerType<CLobbyPackToServer, LobbySetDifficulty>();
	s.template registerType<CLobbyPackToServer, LobbyForceSetPlayer>();
	s.template registerType<CLobbyPackToServer, LobbySetExtraOptions>();
}

VCMI_LIB_NAMESPACE_END
