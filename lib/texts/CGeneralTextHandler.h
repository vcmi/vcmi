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
#include "../json/JsonNode.h"

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
	/// the static store is filled once during load; a duplicate afterwards is a content bug
	bool allowsStringOverride() const override { return false; }

	void readToVector(const std::string & sourceID, const std::string & sourceName);

	JsonNode roeMapping;

public:
	/// Returns true if the installed Heroes III data is RoE (Restoration of Erathia) or RoE Demo,
	/// i.e. TENTCOLR.TXT (a SoD-specific file) is absent from the game data.
	static bool isRoEData();
	LegacyTextContainer allTexts;
	LegacyHelpContainer zelp;

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
