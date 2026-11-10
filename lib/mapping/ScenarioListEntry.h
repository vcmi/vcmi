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
#include "../modding/ModVerificationInfo.h"

class ITranslator;
class ResourcePath;
struct JsonValidator;
enum class EMapFormat : uint8_t;

/// Entry of the list of maps, saved games or campaigns, described by metadata instead of the map or campaign header
class DLL_LINKAGE ScenarioListEntry
{
public:
	enum class EType : uint8_t
	{
		MAP,
		SAVE,
		CAMPAIGN
	};

	/// Reason why the entry can not be selected with active mods and this version of VCMI
	enum class EProblem : uint8_t
	{
		NONE,
		MODS, ///< see mods
		MAP_FORMAT, ///< map format is not supported by active mods
		SAVE_TOO_OLD,
		SAVE_TOO_NEW,
		INVALID_FILE ///< see error
	};

	enum class EStatus : uint8_t
	{
		COMPATIBLE,
		MODS_INACTIVE, ///< can be selected after enabling or disabling installed mods
		INCOMPATIBLE
	};

	EType type = EType::MAP;
	std::string fileURI; ///< same as CMapInfo::fileURI
	std::string originalFileURI; ///< resource name with letter case kept
	std::time_t lastWrite = 0;
	JsonNode metadata; ///< format is described by config/schemas/scenarioMetadata.json, may lack "map" and "campaign" sections if the file can not be read
	EProblem problem = EProblem::NONE;
	ModListVerificationStatus mods; ///< mods that do not match active mods, by mod ID
	std::string error; ///< reason why the file can not be read, for INVALID_FILE

	/// Throws std::runtime_error if the map can not be loaded
	static ScenarioListEntry fromMap(const ResourcePath & file);

	/// Throws SaveTooOldException or SaveTooNewException if version of the save is not supported,
	/// std::runtime_error if the save is invalid
	static ScenarioListEntry fromSave(const ResourcePath & file, JsonValidator & validator);

	/// Reads only the campaign header; throws std::runtime_error if it can not be loaded
	static ScenarioListEntry fromCampaign(const ResourcePath & file);

	/// Name, author, contact, version, creation time and format are of the campaign for campaigns, of the map otherwise
	std::string getName(const ITranslator * translator) const;
	std::string getAuthor(const ITranslator * translator) const;
	std::string getAuthorContact(const ITranslator * translator) const;
	std::string getVersion(const ITranslator * translator) const;
	/// Returns 0 if creation time is unknown
	std::time_t getCreationTime() const;
	EMapFormat getMapFormat() const;
	/// Returns settings of the map format from the engine settings, null node for unknown formats
	const JsonNode & getMapFormatSettings() const;
	int getMapWidth() const;
	int getVictoryIcon() const;
	int getDefeatIcon() const;
	/// Returns number of players that can be controlled by human or computer
	int getPlayers() const;
	/// Returns number of players that can be controlled by human
	int getHumanPlayers() const;

	EStatus getStatus() const;

	bool isCampaignSave() const;
	bool isTutorialSave() const;
	int getHumanPlayersInSave() const;
	/// Returns map of the saved game, or campaign file for campaign saves; empty for random maps
	std::string getMapFileInSave() const;
	/// Returns current scenario of a campaign save
	int getCampaignScenarioInSave() const;
	int getCampaignScenarios() const;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & type;
		h & fileURI;
		h & originalFileURI;
		h & lastWrite;
		h & metadata;
		h & problem;
		h & mods;
		h & error;
	}

private:
	/// Section of metadata with name, author and version: "campaign" for campaigns, "map" otherwise
	const JsonNode & getDescription() const;
};
