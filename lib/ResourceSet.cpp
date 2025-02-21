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
#include "constants/StringConstants.h"
#include "serializer/JsonSerializeFormat.h"
#include "mapObjects/CObjectHandler.h"
#include "GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

ResourceSet::ResourceSet() = default;

ResourceSet::ResourceSet(const JsonNode & node)
{
	for(auto i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
		container[i] = static_cast<int>(node[GameConstants::RESOURCE_NAMES[i]].Float());
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

void ResourceSet::applyHandicap(int percentage)
{
	for(auto & elem : *this)
	{
		int64_t newAmount = vstd::divideAndCeil(static_cast<int64_t>(elem) * percentage, 100);
		int64_t cap = GameConstants::PLAYER_RESOURCES_CAP;
		elem = std::min(cap, newAmount);
	}
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
		total += static_cast<TResourceCap>(LIBRARY->objh->resVals[i]) * static_cast<TResourceCap>(operator[](i));
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
	return cur.resType < GameResID::COUNT && cur.resVal;
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
	} while(cur.resType < GameResID::COUNT && !(cur.resVal=rs[cur.resType]));

	if(cur.resType >= GameResID::COUNT)
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
