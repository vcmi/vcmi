
/*
 * CZoneGraphGenerator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class CRmgTemplateZone;
class CRandomGenerator;
class CMapGenOptions;

class CZoneCell
{
public:
	explicit CZoneCell(const CRmgTemplateZone * zone);

private:
	//const CRmgTemplateZone * zone;

	//TODO additional data
};

class CZoneGraph
{
public:
	CZoneGraph();

private:
	//TODO zone graph storage
};

class CZoneGraphGenerator
{
public:
	CZoneGraphGenerator();

	std::unique_ptr<CZoneGraph> generate(const CMapGenOptions & options, CRandomGenerator * gen);

private:
	std::unique_ptr<CZoneGraph> graph;
	//CRandomGenerator * gen;
};
