/*
 * GameCb.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameCb.h"

#include <vcmi/Player.h>

#include "../LuaCallWrapper.h"

#include "../../../lib/mapObjects/CGHeroInstance.h"

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(GameCbProxy, "Game");

const std::vector<GameCbProxy::RegType> GameCbProxy::REGISTER = {};

const std::vector<GameCbProxy::CustomRegType> GameCbProxy::REGISTER_CUSTOM =
{
	{"getDate", LuaMethodWrapper<GameCb, int32_t(GameCb:: *)(Date::EDateType)const, &GameCb::getDate>::invoke, false},
	{"isAllowed", LuaMethodWrapper<GameCb, bool(GameCb:: *)(int32_t, int32_t)const, &GameCb::isAllowed>::invoke, false},
	{"getCurrentPlayer", LuaMethodWrapper<GameCb, PlayerColor(GameCb:: *)()const, &GameCb::getLocalPlayer>::invoke, false},
	{"getPlayer", LuaMethodWrapper<GameCb, const Player * (GameCb:: *)(PlayerColor)const, &GameCb::getPlayer>::invoke, false},

	{"getHero", LuaMethodWrapper<GameCb, const CGHeroInstance *(GameCb:: *)(ObjectInstanceID)const, &GameCb::getHero>::invoke, false},
	{"getHeroWithSubid", LuaMethodWrapper<GameCb, const CGHeroInstance *(GameCb:: *)(int)const, &GameCb::getHeroWithSubid>::invoke, false},

	{"getObj", LuaMethodWrapper<GameCb, const CGObjectInstance *(GameCb:: *)(ObjectInstanceID, bool)const, &GameCb::getObj>::invoke, false},
};

}
}
