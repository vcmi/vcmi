/*
 * MapFeaturesH3M.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MapFeaturesH3M.h"

#include "MapFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

MapFormatFeaturesH3M MapFormatFeaturesH3M::find(EMapFormat format, uint32_t hotaVersion)
{
	switch(format)
	{
		case EMapFormat::ROE:
			return getFeaturesROE();
		case EMapFormat::AB:
			return getFeaturesAB();
		case EMapFormat::SOD:
			return getFeaturesSOD();
		case EMapFormat::WOG:
			return getFeaturesWOG();
		case EMapFormat::HOTA:
			return getFeaturesHOTA(hotaVersion);
		default:
			throw std::runtime_error("Invalid map format!");
	}
}

MapFormatFeaturesH3M MapFormatFeaturesH3M::getFeaturesROE()
{
	MapFormatFeaturesH3M result;
	result.levelROE = true;

	result.factionsBytes = 1;
	result.heroesBytes = 16;
	result.artifactsBytes = 16;
	result.skillsBytes = 4;
	result.resourcesBytes = 4;
	result.spellsBytes = 9;
	result.buildingsBytes = 6;

	result.factionsCount = 8;
	result.heroesCount = 128;
	result.heroesPortraitsCount = 128;
	result.artifactsCount = 127;
	result.resourcesCount = 7;
	result.creaturesCount = 118;
	result.spellsCount = 70;
	result.skillsCount = 28;
	result.terrainsCount = 10;
	result.artifactSlotsCount = 18;
	result.buildingsCount = 41;

	result.heroIdentifierInvalid = 0xff;
	result.artifactIdentifierInvalid = 0xff;
	result.creatureIdentifierInvalid = 0xff;
	result.spellIdentifierInvalid = 0xff;

	return result;
}

MapFormatFeaturesH3M MapFormatFeaturesH3M::getFeaturesAB()
{
	MapFormatFeaturesH3M result = getFeaturesROE();
	result.levelAB = true;

	result.factionsBytes = 2; // + Conflux
	result.factionsCount = 9;

	result.creaturesCount = 145; // + Conflux and new neutrals

	result.heroesCount = 156; // + Conflux and campaign heroes
	result.heroesPortraitsCount = 163;
	result.heroesBytes = 20;

	result.artifactsCount = 129; // + Armaggedon Blade and Vial of Dragon Blood
	result.artifactsBytes = 17;

	result.artifactIdentifierInvalid = 0xffff; // Now uses 2 bytes / object
	result.creatureIdentifierInvalid = 0xffff; // Now uses 2 bytes / object

	return result;
}

MapFormatFeaturesH3M MapFormatFeaturesH3M::getFeaturesSOD()
{
	MapFormatFeaturesH3M result = getFeaturesAB();
	result.levelSOD = true;

	result.artifactsCount = 141; // + Combined artifacts
	result.artifactsBytes = 18;

	result.artifactSlotsCount = 19; // + MISC_5 slot

	return result;
}

MapFormatFeaturesH3M MapFormatFeaturesH3M::getFeaturesWOG()
{
	MapFormatFeaturesH3M result = getFeaturesSOD();
	result.levelWOG = true;

	return result;
}

MapFormatFeaturesH3M MapFormatFeaturesH3M::getFeaturesHOTA(uint32_t hotaVersion)
{
	// even if changes are minimal, we might not be able to parse map header in map selection screen
	// throw exception - to be catched by map selection screen & excluded as invalid
	if(hotaVersion > 3)
		throw std::runtime_error("Invalid map format!");

	MapFormatFeaturesH3M result = getFeaturesSOD();
	result.levelHOTA0 = true;
	result.levelHOTA1 = hotaVersion > 0;
	//result.levelHOTA2 = hotaVersion > 1; // HOTA2 seems to be identical to HOTA1 so far
	result.levelHOTA3 = hotaVersion > 2;

	result.artifactsBytes = 21;
	result.heroesBytes = 23;

	result.terrainsCount = 12; // +Highlands +Wasteland
	result.skillsCount = 29; // + Interference
	result.factionsCount = 10; // + Cove
	result.creaturesCount = 171; // + Cove + neutrals

	if(hotaVersion < 3)
	{
		result.artifactsCount = 163; // + HotA artifacts
		result.heroesCount = 176; // + Cove
		result.heroesPortraitsCount = 187; // + Cove
	}
	if(hotaVersion == 3)
	{
		result.artifactsCount = 165; // + HotA artifacts
		result.heroesCount = 177; // + Cove + Giselle
		result.heroesPortraitsCount = 188; // + Cove + Giselle
	}

	assert((result.heroesCount + 7) / 8 == result.heroesBytes);
	assert((result.artifactsCount + 7) / 8 == result.artifactsBytes);

	result.heroesCount = 179; // + Cove

	return result;
}

VCMI_LIB_NAMESPACE_END
