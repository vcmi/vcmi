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

enum class EMapFormat : uint8_t;

struct DLL_LINKAGE MapFormatFeaturesH3M
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

	int factionsBytes = 0;
	int heroesBytes = 0;
	int artifactsBytes = 0;
	int resourcesBytes = 0;
	int skillsBytes = 0;
	int spellsBytes = 0;
	int buildingsBytes = 0;

	// total number of elements of appropriate type

	int factionsCount = 0;
	int heroesCount = 0;
	int heroesPortraitsCount = 0;
	int artifactsCount = 0;
	int resourcesCount = 0;
	int creaturesCount = 0;
	int spellsCount = 0;
	int skillsCount = 0;
	int terrainsCount = 0;
	int roadsCount = 0;
	int riversCount = 0;
	int artifactSlotsCount = 0;
	int buildingsCount = 0;

	// identifier that should be treated as "invalid", usually - '-1'

	int heroIdentifierInvalid = 0;
	int artifactIdentifierInvalid = 0;
	int creatureIdentifierInvalid = 0;
	int spellIdentifierInvalid = 0;

	// features from which map format are available

	bool levelROE = false;
	bool levelAB = false;
	bool levelSOD = false;
	bool levelCHR = false;
	bool levelWOG = false;
	// older, unsupported hota maps:
	// 1.0 -> uses SoD format
	// 1.1 -> uses SoD or WoG format
	// 1.2 -> uses custom 0x1e / 30 format
	bool levelHOTA0 = false; // 1.3.0
	bool levelHOTA1 = false; // 1.5.0
	bool levelHOTA2 = false; // 1.6.0
	bool levelHOTA3 = false; // 1.6.0
	// level 4 - not released publicly?
	bool levelHOTA5 = false; // 1.7.0
	bool levelHOTA6 = false; // 1.7.1
	bool levelHOTA7 = false; // 1.7.2
	bool levelHOTA8 = false; // 1.7.3
	bool levelHOTA9 = false; // 1.8.0
	bool levelHOTA10 = false; // 1.8.1
};
