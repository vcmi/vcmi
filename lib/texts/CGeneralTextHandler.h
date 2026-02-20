/*
 * CGeneralTextHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "TextLocalizationContainer.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGeneralTextHandler;

/// Small wrapper that provides text access API compatible with old code
class DLL_LINKAGE LegacyTextContainer
{
	CGeneralTextHandler & owner;
	std::string basePath;

public:
	LegacyTextContainer(CGeneralTextHandler & owner, std::string basePath);
	std::string operator [](size_t index) const;
};

/// Small wrapper that provides help text access API compatible with old code
class DLL_LINKAGE LegacyHelpContainer
{
	CGeneralTextHandler & owner;
	std::string basePath;

public:
	LegacyHelpContainer(CGeneralTextHandler & owner, std::string basePath);
	std::pair<std::string, std::string> operator[](size_t index) const;
};

/// Handles all text-related data in game
class DLL_LINKAGE CGeneralTextHandler: public TextLocalizationContainer
{
	void readToVector(const std::string & sourceID, const std::string & sourceName);

public:
	LegacyTextContainer allTexts;

	LegacyTextContainer arraytxt;
	LegacyTextContainer primarySkillNames;
	LegacyTextContainer jktexts;
	LegacyTextContainer heroscrn;
	LegacyTextContainer overview;//text for Kingdom Overview window
	LegacyTextContainer capColors; //names of player colors with first letter capitalized ("Red",...)
	LegacyTextContainer turnDurations; //turn durations for pregame (1 Minute ... Unlimited)

	//towns
	LegacyTextContainer tcommands; //texts for town screen,
	LegacyTextContainer hcommands; // town hall screen
	LegacyTextContainer fcommands; // fort screen
	LegacyTextContainer tavernInfo;

	LegacyHelpContainer zelp;

	//objects
	LegacyTextContainer advobtxt;
	LegacyTextContainer restypes; //names of resources
	LegacyTextContainer seerEmpty;
	LegacyTextContainer seerNames;
	LegacyTextContainer tentColors;

	//sec skills
	LegacyTextContainer levels;

	std::vector<std::string> findStringsWithPrefix(const std::string & prefix);

	int32_t pluralText(int32_t textIndex, int32_t count) const;

	CGeneralTextHandler();
	CGeneralTextHandler(const CGeneralTextHandler&) = delete;
	CGeneralTextHandler operator=(const CGeneralTextHandler&) = delete;

	/// Attempts to detect encoding & language of H3 files
	static void detectInstallParameters();

	/// Returns name of language preferred by user
	static std::string getPreferredLanguage();

	/// Returns name of language of Heroes III text files
	static std::string getInstalledLanguage();

	/// Returns name of encoding of Heroes III text files
	static std::string getInstalledEncoding();
};

VCMI_LIB_NAMESPACE_END
