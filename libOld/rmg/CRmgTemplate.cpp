
/*
 * CRmgTemplate.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CRmgTemplate.h"

#include "CRmgTemplateZone.h"
#include "../mapping/CMap.h"

CRmgTemplateZoneConnection::CRmgTemplateZoneConnection() : zoneA(nullptr), zoneB(nullptr), guardStrength(0)
{

}

CRmgTemplateZone * CRmgTemplateZoneConnection::getZoneA() const
{
	return zoneA;
}

void CRmgTemplateZoneConnection::setZoneA(CRmgTemplateZone * value)
{
	zoneA = value;
}

CRmgTemplateZone * CRmgTemplateZoneConnection::getZoneB() const
{
	return zoneB;
}

void CRmgTemplateZoneConnection::setZoneB(CRmgTemplateZone * value)
{
	zoneB = value;
}

int CRmgTemplateZoneConnection::getGuardStrength() const
{
	return guardStrength;
}

void CRmgTemplateZoneConnection::setGuardStrength(int value)
{
	if(value < 0) throw std::runtime_error("Negative value for guard strenth not allowed.");
	guardStrength = value;
}

CRmgTemplate::CSize::CSize() : width(CMapHeader::MAP_SIZE_MIDDLE), height(CMapHeader::MAP_SIZE_MIDDLE), under(true)
{

}

CRmgTemplate::CSize::CSize(int width, int height, bool under) : under(under)
{
	setWidth(width);
	setHeight(height);
}

int CRmgTemplate::CSize::getWidth() const
{
	return width;
}

void CRmgTemplate::CSize::setWidth(int value)
{
	if(value <= 0) throw std::runtime_error("Width > 0 failed.");
	width = value;
}

int CRmgTemplate::CSize::getHeight() const
{
	return height;
}

void CRmgTemplate::CSize::setHeight(int value)
{
	if(value <= 0) throw std::runtime_error("Height > 0 failed.");
	height = value;
}

bool CRmgTemplate::CSize::getUnder() const
{
	return under;
}

void CRmgTemplate::CSize::setUnder(bool value)
{
	under = value;
}

bool CRmgTemplate::CSize::operator<=(const CSize & value) const
{
	if(width < value.width && height < value.height)
	{
		return true;
	}
	else if(width == value.width && height == value.height)
	{
		return under ? value.under : true;
	}
	else
	{
		return false;
	}
}

bool CRmgTemplate::CSize::operator>=(const CSize & value) const
{
	if(width > value.width && height > value.height)
	{
		return true;
	}
	else if(width == value.width && height == value.height)
	{
		return under ? true : !value.under;
	}
	else
	{
		return false;
	}
}

CRmgTemplate::CRmgTemplate()
{

}

CRmgTemplate::~CRmgTemplate()
{
	for (auto & pair : zones) delete pair.second;
}

const std::string & CRmgTemplate::getName() const
{
	return name;
}

void CRmgTemplate::setName(const std::string & value)
{
	name = value;
}

const CRmgTemplate::CSize & CRmgTemplate::getMinSize() const
{
	return minSize;
}

void CRmgTemplate::setMinSize(const CSize & value)
{
	minSize = value;
}

const CRmgTemplate::CSize & CRmgTemplate::getMaxSize() const
{
	return maxSize;
}

void CRmgTemplate::setMaxSize(const CSize & value)
{
	maxSize = value;
}

const CRmgTemplate::CPlayerCountRange & CRmgTemplate::getPlayers() const
{
	return players;
}

void CRmgTemplate::setPlayers(const CPlayerCountRange & value)
{
	players = value;
}

const CRmgTemplate::CPlayerCountRange & CRmgTemplate::getCpuPlayers() const
{
	return cpuPlayers;
}

void CRmgTemplate::setCpuPlayers(const CPlayerCountRange & value)
{
	cpuPlayers = value;
}

const std::map<TRmgTemplateZoneId, CRmgTemplateZone *> & CRmgTemplate::getZones() const
{
	return zones;
}

void CRmgTemplate::setZones(const std::map<TRmgTemplateZoneId, CRmgTemplateZone *> & value)
{
	zones = value;
}

const std::list<CRmgTemplateZoneConnection> & CRmgTemplate::getConnections() const
{
	return connections;
}

void CRmgTemplate::setConnections(const std::list<CRmgTemplateZoneConnection> & value)
{
	connections = value;
}

void CRmgTemplate::validate() const
{
	//TODO add some validation checks, throw on failure
}

void CRmgTemplate::CPlayerCountRange::addRange(int lower, int upper)
{
	range.push_back(std::make_pair(lower, upper));
}

void CRmgTemplate::CPlayerCountRange::addNumber(int value)
{
	range.push_back(std::make_pair(value, value));
}

bool CRmgTemplate::CPlayerCountRange::isInRange(int count) const
{
	for(const auto & pair : range)
	{
		if(count >= pair.first && count <= pair.second) return true;
	}
	return false;
}

std::set<int> CRmgTemplate::CPlayerCountRange::getNumbers() const
{
	std::set<int> numbers;
	for(const auto & pair : range)
	{
		for(int i = pair.first; i <= pair.second; ++i) numbers.insert(i);
	}
	return numbers;
}
