/*
 * common.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class CBattleCallback;

template<typename Key, typename Val, typename Val2>
const Val getValOr(const std::map<Key, Val> &Map, const Key &key, const Val2 defaultValue)
{
	//returning references here won't work: defaultValue must be converted into Val, creating temporary
	auto i = Map.find(key);
	if(i != Map.end())
		return i->second;
	else
		return defaultValue;
}

void setCbc(std::shared_ptr<CBattleCallback> cb);
std::shared_ptr<CBattleCallback> getCbc();
