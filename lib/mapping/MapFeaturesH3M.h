/*
 * MapFeaturesH3M.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

enum class EMapFormat : uint8_t;

struct MapFormatFeaturesH3M
{
private:
	static MapFormatFeaturesH3M getFeaturesROE();
	static MapFormatFeaturesH3M getFeaturesAB();
	static MapFormatFeaturesH3M getFeaturesSOD();
	static MapFormatFeaturesH3M getFeaturesCHR();
	static MapFormatFeaturesH3M getFeaturesWOG();
	static MapFormatFeaturesH3M getFeaturesHOTA(uint32_t hotaVersion);

public:
	MapFormatFeaturesH3M() = default;

	static MapFormatFeaturesH3M find(EMapFormat format, uint32_t hotaVersion);

	// number of bytes in bitmask of appropriate type

	int factionsBytes;
	int heroesBytes;
	int artifactsBytes;
	int resourcesBytes;
	int skillsBytes;
	int spellsBytes;
	int buildingsBytes;

	// total number of elements of appropriate type

	int factionsCount;
	int heroesCount;
	int heroesPortraitsCount;
	int artifactsCount;
	int resourcesCount;
	int creaturesCount;
	int spellsCount;
	int skillsCount;
	int terrainsCount;
	int roadsCount;
	int riversCount;
	int artifactSlotsCount;
	int buildingsCount;

	// identifier that should be treated as "invalid", usually - '-1'

	int heroIdentifierInvalid;
	int artifactIdentifierInvalid;
	int creatureIdentifierInvalid;
	int spellIdentifierInvalid;

	// features from which map format are available

	bool levelROE = false;
	bool levelAB = false;
	bool levelSOD = false;
	bool levelCHR = false;
	bool levelWOG = false;
	bool levelHOTA0 = false;
	bool levelHOTA1 = false;
	bool levelHOTA2 = false;
	bool levelHOTA3 = false; // 1.6.0
	// level 4 - not released publicly?
	bool levelHOTA5 = false; // 1.7.0
	bool levelHOTA6 = false; // 1.7.1
	bool levelHOTA7 = false; // 1.7.2
	bool levelHOTA8 = false; // 1.7.3
};

VCMI_LIB_NAMESPACE_END
