/*
 * CRmgTemplate.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"

class CRmgTemplateZone;

/// The CRmgTemplateZoneConnection describes the connection between two zones.
class DLL_LINKAGE CRmgTemplateZoneConnection
{
public:
	CRmgTemplateZoneConnection();

	CRmgTemplateZone * getZoneA() const;
	void setZoneA(CRmgTemplateZone * value);
	CRmgTemplateZone * getZoneB() const;
	void setZoneB(CRmgTemplateZone * value);
	int getGuardStrength() const; /// Default: 0
	void setGuardStrength(int value);

private:
	CRmgTemplateZone * zoneA, * zoneB;
	int guardStrength;
};

/// The CRmgTemplate describes a random map template.
class DLL_LINKAGE CRmgTemplate
{
public:
	class CSize
	{
	public:
		CSize();
		CSize(int width, int height, bool under);

		int getWidth() const; /// Default: CMapHeader::MAP_SIZE_MIDDLE
		void setWidth(int value);
		int getHeight() const; /// Default: CMapHeader::MAP_SIZE_MIDDLE
		void setHeight(int value);
		bool getUnder() const; /// Default: true
		void setUnder(bool value);
		bool operator<=(const CSize & value) const;
		bool operator>=(const CSize & value) const;

	private:
		int width, height;
		bool under;
	};

	class CPlayerCountRange
	{
	public:
		void addRange(int lower, int upper);
		void addNumber(int value);
		bool isInRange(int count) const;
		std::set<int> getNumbers() const;

	private:
		std::list<std::pair<int, int> > range;
	};

	CRmgTemplate();
	~CRmgTemplate();

	const std::string & getName() const;
	void setName(const std::string & value);
	const CSize & getMinSize() const;
	void setMinSize(const CSize & value);
	const CSize & getMaxSize() const;
	void setMaxSize(const CSize & value);
	const CPlayerCountRange & getPlayers() const;
	void setPlayers(const CPlayerCountRange & value);
	const CPlayerCountRange & getCpuPlayers() const;
	void setCpuPlayers(const CPlayerCountRange & value);
	const std::map<TRmgTemplateZoneId, CRmgTemplateZone *> & getZones() const;
	void setZones(const std::map<TRmgTemplateZoneId, CRmgTemplateZone *> & value);
	const std::list<CRmgTemplateZoneConnection> & getConnections() const;
	void setConnections(const std::list<CRmgTemplateZoneConnection> & value);

	void validate() const; /// Tests template on validity and throws exception on failure

private:
	std::string name;
	CSize minSize, maxSize;
	CPlayerCountRange players, cpuPlayers;
	std::map<TRmgTemplateZoneId, CRmgTemplateZone *> zones;
	std::list<CRmgTemplateZoneConnection> connections;
};
