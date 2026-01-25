/*
 * Discord.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Discord.h"

#ifdef ENABLE_DISCORD
#include <discord-rpc.hpp>
#endif

#include "../lib/StartInfo.h"
#include "../lib/mapping/CMap.h"
#include "../lib/campaign/CampaignState.h"

Discord::Discord()
{
#ifdef ENABLE_DISCORD
	constexpr auto APPLICATION_ID = "1416538363254669455";

	discord::RPCManager::get().initialize();
	discord::RPCManager::get()
		.setClientID(APPLICATION_ID)
		.onReady([](discord::User const& user) {
			logGlobal->info("Discord: connected to user %s#%s - %s", user.username, user.discriminator, user.id);
		})
		.onDisconnected([](int errcode, std::string_view message) {
			logGlobal->warn("Discord: disconnected with error code %s - %s", errcode, message);
		})
		.onErrored([](int errcode, std::string_view message) {
			logGlobal->error("Discord: error with code %s - %s", errcode, message);
		});
	
	startTime = time(nullptr);
	setStatus("", "", {0, 0});
#endif
}

Discord::~Discord()
{
#ifdef ENABLE_DISCORD
	clearStatus();
	discord::RPCManager::get().shutdown();
#endif
}

void Discord::setStatus(std::string state, std::string details, std::tuple<int, int> partySize)
{
#ifdef ENABLE_DISCORD
	discord::RPCManager::get().getPresence()
		.setState(state)
		.setActivityType(discord::ActivityType::Game)
		.setStatusDisplayType(discord::StatusDisplayType::Name)
		.setDetails(details)
		.setStartTimestamp(startTime)
		.setLargeImageKey("canary-large")
		.setSmallImageKey("ptb-small")
		.setInstance(false)
		.setPartySize(std::get<0>(partySize))
		.setPartyMax(std::get<1>(partySize))
		.refresh();
#endif
}

void Discord::clearStatus()
{
#ifdef ENABLE_DISCORD
	discord::RPCManager::get().clearPresence();
#endif
}

void Discord::setPlayingStatus(std::shared_ptr<StartInfo> si, const CMap * map, int humanInterfacesCount)
{
#ifdef ENABLE_DISCORD
	bool isCampaign = si->campState != nullptr;
	bool isMulti = humanInterfacesCount > 1;
	int humanPlayersCount = 0;
	for(auto & player : map->players)
		if(player.canHumanPlay)
			humanPlayersCount++;
	std::string title = "Playing " + std::string(isCampaign ? "Campaign" : "Map") + (isMulti ? " (Multiplayer)" : " (Singleplayer)");
	std::string subTitle = std::string(isCampaign ? si->campState->getNameTranslated() + " - " : "") + map->name.toString();
	setStatus(subTitle, title, {humanInterfacesCount, isMulti ? humanPlayersCount : 0});
#endif
}
