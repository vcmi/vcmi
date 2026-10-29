/*
 * ScenarioListEntry.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../json/JsonNode.h"

class ITranslator;
class ResourcePath;
struct JsonValidator;
enum class EMapFormat : uint8_t;

/// Entry of the list of maps or saved games, described by metadata instead of the map header
class DLL_LINKAGE ScenarioListEntry
{
public:
	enum class EType : uint8_t
	{
		MAP,
		SAVE
	};

	EType type = EType::MAP;
	std::string fileURI; ///< same as CMapInfo::fileURI
	std::string originalFileURI; ///< resource name with letter case kept
	std::time_t lastWrite = 0;
	JsonNode metadata; ///< format is described by config/schemas/scenarioMetadata.json

	/// Throws std::runtime_error if the map can not be loaded
	static ScenarioListEntry fromMap(const ResourcePath & file);

	/// Throws SaveTooOldException or SaveTooNewException if version of the save is not supported,
	/// std::runtime_error if the save is invalid
	static ScenarioListEntry fromSave(const ResourcePath & file, JsonValidator & validator);

	std::string getMapName(const ITranslator * translator) const;
	std::string getMapAuthor(const ITranslator * translator) const;
	std::string getMapAuthorContact(const ITranslator * translator) const;
	std::string getMapVersion(const ITranslator * translator) const;
	/// Returns 0 if creation time is unknown
	std::time_t getMapCreationTime() const;
	EMapFormat getMapFormat() const;
	int getMapWidth() const;
	int getVictoryIcon() const;
	int getDefeatIcon() const;
	/// Returns number of players that can be controlled by human or computer
	int getPlayers() const;
	/// Returns number of players that can be controlled by human
	int getHumanPlayers() const;

	bool isCampaignSave() const;
	bool isTutorialSave() const;
	int getHumanPlayersInSave() const;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & type;
		h & fileURI;
		h & originalFileURI;
		h & lastWrite;
		h & metadata;
	}
};
