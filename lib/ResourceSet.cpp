/*
 * ResourceSet.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GameConstants.h"
#include "ResourceSet.h"
#include "StringConstants.h"
#include "JsonNode.h"
#include "serializer/JsonSerializeFormat.h"
#include "VCMI_Lib.h"
#include "mapObjects/CObjectHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

ResourceSet::ResourceSet(const JsonNode & node)
{
	for(auto i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
		container[i] = static_cast<int>(node[GameConstants::RESOURCE_NAMES[i]].Float());
}

ResourceSet::ResourceSet(TResource wood, TResource mercury, TResource ore, TResource sulfur, TResource crystal,
							TResource gems, TResource gold, TResource mithril)
{
	container[GameResID(EGameResID::WOOD)] = wood;
	container[GameResID(EGameResID::MERCURY)] = mercury;
	container[GameResID(EGameResID::ORE)] = ore;
	container[GameResID(EGameResID::SULFUR)] = sulfur;
	container[GameResID(EGameResID::CRYSTAL)] = crystal;
	container[GameResID(EGameResID::GEMS)] = gems;
	container[GameResID(EGameResID::GOLD)] = gold;
	container[GameResID(EGameResID::MITHRIL)] = mithril;
}

void ResourceSet::serializeJson(JsonSerializeFormat & handler, const std::string & fieldName)
{
	if(handler.saving && !nonZero())
		return;
	auto s = handler.enterStruct(fieldName);

	//TODO: add proper support for mithril to map format
	for(int idx = 0; idx < GameConstants::RESOURCE_QUANTITY - 1; idx ++)
		handler.serializeInt(GameConstants::RESOURCE_NAMES[idx], this->operator[](idx), 0);
}

bool ResourceSet::nonZero() const
{
	for(const auto & elem : *this)
		if(elem)
			return true;

	return false;
}

void ResourceSet::amax(const TResourceCap &val)
{
	for(auto & elem : *this)
		vstd::amax(elem, val);
}

void ResourceSet::amin(const TResourceCap &val)
{
	for(auto & elem : *this)
		vstd::amin(elem, val);
}

void ResourceSet::positive()
{
	for(auto & elem : *this)
		vstd::amax(elem, 0);
}

static bool canAfford(const ResourceSet &res, const ResourceSet &price)
{
	assert(res.size() == price.size() && price.size() == GameConstants::RESOURCE_QUANTITY);
	for(int i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
		if(price[i] > res[i])
			return false;

	return true;
}

bool ResourceSet::canBeAfforded(const ResourceSet &res) const
{
	return VCMI_LIB_WRAP_NAMESPACE(canAfford(res, *this));
}

bool ResourceSet::canAfford(const ResourceSet &price) const
{
	return VCMI_LIB_WRAP_NAMESPACE(canAfford(*this, price));
}

TResourceCap ResourceSet::marketValue() const
{
	TResourceCap total = 0;
	for(int i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
		total += static_cast<TResourceCap>(VLC->objh->resVals[i]) * static_cast<TResourceCap>(operator[](i));
	return total;
}

std::string ResourceSet::toString() const
{
	std::ostringstream out;
	out << "[";
	for(auto it = begin(); it != end(); ++it)
	{
		out << *it;
		if(std::prev(end()) != it) out << ", ";
	}
	out << "]";
	return out.str();
}

bool ResourceSet::nziterator::valid() const
{
	return cur.resType < GameConstants::RESOURCE_QUANTITY && cur.resVal;
}

ResourceSet::nziterator ResourceSet::nziterator::operator++()
{
	advance();
	return *this;
}

ResourceSet::nziterator ResourceSet::nziterator::operator++(int)
{
	nziterator ret = *this;
	advance();
	return ret;
}

const ResourceSet::nziterator::ResEntry& ResourceSet::nziterator::operator*() const
{
	return cur;
}

const ResourceSet::nziterator::ResEntry * ResourceSet::nziterator::operator->() const
{
	return &cur;
}

void ResourceSet::nziterator::advance()
{
	do
	{
		++cur.resType;
	} while(cur.resType < GameConstants::RESOURCE_QUANTITY && !(cur.resVal=rs[cur.resType]));

	if(cur.resType >= GameConstants::RESOURCE_QUANTITY)
		cur.resVal = -1;
}

ResourceSet::nziterator::nziterator(const ResourceSet &RS)
	: rs(RS)
{
	cur.resType = EGameResID::WOOD;
	cur.resVal = rs[EGameResID::WOOD];

	if(!valid())
		advance();
}

VCMI_LIB_NAMESPACE_END
