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

Res::ResourceSet::ResourceSet()
{
	resize(GameConstants::RESOURCE_QUANTITY, 0);
}

Res::ResourceSet::ResourceSet(const JsonNode & node)
{
	reserve(GameConstants::RESOURCE_QUANTITY);
	for(std::string name : GameConstants::RESOURCE_NAMES)
		push_back(node[name].Float());
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
		::vstd::amax(elem, val);
}

void Res::ResourceSet::positive()
{
	for(auto & elem : *this)
		::vstd::amax(elem, 0);
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
