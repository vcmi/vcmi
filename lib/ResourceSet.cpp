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
#include "ResourceSet.h"
#include "StringConstants.h"
#include "JsonNode.h"
#include "serializer/JsonSerializeFormat.h"
#include "VCMI_Lib.h"
#include "mapObjects/CObjectHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

Res::ResourceSet::ResourceSet()
{
	resize(GameConstants::RESOURCE_QUANTITY, 0);
}

Res::ResourceSet::ResourceSet(const JsonNode & node)
{
	reserve(GameConstants::RESOURCE_QUANTITY);
	for(std::string name : GameConstants::RESOURCE_NAMES)
		push_back((int)node[name].Float());
}

Res::ResourceSet::ResourceSet(TResource wood, TResource mercury, TResource ore, TResource sulfur, TResource crystal,
							TResource gems, TResource gold, TResource mithril)
{
	resize(GameConstants::RESOURCE_QUANTITY);
	auto d = data();
	d[Res::WOOD] = wood;
	d[Res::MERCURY] = mercury;
	d[Res::ORE] = ore;
	d[Res::SULFUR] = sulfur;
	d[Res::CRYSTAL] = crystal;
	d[Res::GEMS] = gems;
	d[Res::GOLD] = gold;
	d[Res::MITHRIL] = mithril;
}

void Res::ResourceSet::serializeJson(JsonSerializeFormat & handler, const std::string & fieldName)
{
	if(!handler.saving)
		resize(GameConstants::RESOURCE_QUANTITY, 0);
	if(handler.saving && !nonZero())
		return;
	auto s = handler.enterStruct(fieldName);

	//TODO: add proper support for mithril to map format
	for(int idx = 0; idx < GameConstants::RESOURCE_QUANTITY - 1; idx ++)
		handler.serializeInt(GameConstants::RESOURCE_NAMES[idx], this->operator[](idx), 0);
}

bool Res::ResourceSet::nonZero() const
{
	for(auto & elem : *this)
		if(elem)
			return true;

	return false;
}

void Res::ResourceSet::amax(const TResourceCap &val)
{
	for(auto & elem : *this)
		vstd::amax(elem, val);
}

void Res::ResourceSet::amin(const TResourceCap &val)
{
	for(auto & elem : *this)
		vstd::amin(elem, val);
}

void Res::ResourceSet::positive()
{
	for(auto & elem : *this)
		vstd::amax(elem, 0);
}

bool Res::ResourceSet::canBeAfforded(const ResourceSet &res) const
{
	return Res::canAfford(res, *this);
}

bool Res::ResourceSet::canAfford(const ResourceSet &price) const
{
	return Res::canAfford(*this, price);
}

bool Res::canAfford(const ResourceSet &res, const ResourceSet &price)
{
	assert(res.size() == price.size() && price.size() == GameConstants::RESOURCE_QUANTITY);
	for(int i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
		if(price[i] > res[i])
			return false;

	return true;
}

TResourceCap Res::ResourceSet::marketValue() const
{
	TResourceCap total = 0;
	for(int i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
		total += static_cast<TResourceCap>(VLC->objh->resVals[i]) * static_cast<TResourceCap>(operator[](i));
	return total;
}

std::string Res::ResourceSet::toString() const
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

bool Res::ResourceSet::nziterator::valid()
{
	return cur.resType < GameConstants::RESOURCE_QUANTITY && cur.resVal;
}

Res::ResourceSet::nziterator Res::ResourceSet::nziterator::operator++()
{
	advance();
	return *this;
}

Res::ResourceSet::nziterator Res::ResourceSet::nziterator::operator++(int)
{
	nziterator ret = *this;
	advance();
	return ret;
}

const Res::ResourceSet::nziterator::ResEntry& Res::ResourceSet::nziterator::operator*() const
{
	return cur;
}

const Res::ResourceSet::nziterator::ResEntry * Res::ResourceSet::nziterator::operator->() const
{
	return &cur;
}

void Res::ResourceSet::nziterator::advance()
{
	do
	{
		vstd::advance(cur.resType, +1);
	} while(cur.resType < GameConstants::RESOURCE_QUANTITY && !(cur.resVal=rs[cur.resType]));

	if(cur.resType >= GameConstants::RESOURCE_QUANTITY)
		cur.resVal = -1;
}

Res::ResourceSet::nziterator::nziterator(const ResourceSet &RS)
	: rs(RS)
{
	cur.resType = WOOD;
	cur.resVal = rs[WOOD];

	if(!valid())
		advance();
}

VCMI_LIB_NAMESPACE_END
