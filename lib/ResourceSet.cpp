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
#include "entities/ResourceTypeHandler.h"
#include "GameLibrary.h"
#include "modding/IdentifierResolver.h"

void ResourceSet::resolveFromJson(const JsonNode & node, IIdentifierResolver & resolver)
{
	for(auto & n : node.Struct())
		resolver.requestIdentifier(n.second.getModScope(), "resource", n.first, [n, this](int32_t identifier)
		{
			container[GameResID(identifier)] = static_cast<int>(n.second.Float());
		});
}

void ResourceSet::serializeJson(JsonSerializeFormat & handler, const std::string & fieldName)
{
	if(handler.saving && !nonZero())
		return;
	auto s = handler.enterStruct(fieldName);

	if(handler.saving)
	{
		for(auto & [resource, amount] : container)
			handler.serializeInt(resource.toResource()->getJsonKey(), amount, 0);
	}
	else
	{
		container.clear();
		for(const auto & resource : LIBRARY->resourceTypeHandler->getAllObjects())
		{
			TResource amount = 0;
			handler.serializeInt(resource.toResource()->getJsonKey(), amount, 0);
			if(amount)
				container[resource] = amount;
		}
	}
}

bool ResourceSet::nonZero() const
{
	return !nonZeroEntries().empty();
}

void ResourceSet::amin(const TResourceCap &val)
{
	assert(val >= 0);
	for(auto & entry : container)
		vstd::amin(entry.second, val);
}

void ResourceSet::removeNegative()
{
	for(auto & entry : container)
		vstd::amax(entry.second, 0);
}

void ResourceSet::applyHandicap(int percentage)
{
	for(auto & [resource, elem] : container)
	{
		int64_t newAmount = vstd::divideAndCeil(static_cast<int64_t>(elem) * percentage, 100);
		int64_t cap = GameConstants::PLAYER_RESOURCES_CAP;
		elem = std::min(cap, newAmount);
	}
}

bool ResourceSet::canAfford(const ResourceSet &price) const
{
	for(const auto & [resource, amount] : price)
		if(amount > (*this)[resource])
			return false;

	return true;
}

TResourceCap ResourceSet::marketValue() const
{
	TResourceCap total = 0;
	for(const auto & [resource, amount] : container)
		total += static_cast<TResourceCap>(resource.toResource()->getPrice()) * static_cast<TResourceCap>(amount);
	return total;
}

std::string ResourceSet::toString() const
{
	std::ostringstream out;
	out << "[";
	for(auto it = container.begin(); it != container.end(); ++it)
	{
		if(it != container.begin())
			out << ", ";
		out << it->first.getNum() << ": " << it->second;
	}
	out << "]";
	return out.str();
}
