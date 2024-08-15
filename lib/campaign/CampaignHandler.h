/*
 * CampaignHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CampaignState.h" // Convenience include - not required for build, but required for any user of CampaignHandler
#include "../filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CampaignHandler
{
	static std::string readLocalizedString(CampaignHeader & target, CBinaryReader & reader, std::string filename, std::string modName, std::string encoding, std::string identifier);
	static std::string readLocalizedString(CampaignHeader & target, std::string text, std::string filename, std::string modName, std::string identifier);

	static void readCampaign(Campaign * target, const std::vector<ui8> & stream, std::string filename, std::string modName, std::string encoding);

	//parsers for VCMI campaigns (*.vcmp)
	static void readHeaderFromJson(CampaignHeader & target, JsonNode & reader, std::string filename, std::string modName, std::string encoding);
	static CampaignScenario readScenarioFromJson(JsonNode & reader);
	static CampaignTravel readScenarioTravelFromJson(JsonNode & reader);

	//parsers for original H3C campaigns
	static void readHeaderFromMemory(CampaignHeader & target, CBinaryReader & reader, std::string filename, std::string modName, std::string encoding);
	static CampaignScenario readScenarioFromMemory(CBinaryReader & reader, CampaignHeader & header);
	static CampaignTravel readScenarioTravelFromMemory(CBinaryReader & reader, CampaignVersion version);
	/// returns h3c split in parts. 0 = h3c header, 1-end - maps (binary h3m)
	/// headerOnly - only header will be decompressed, returned vector wont have any maps
	static std::vector<std::vector<ui8>> getFile(std::unique_ptr<CInputStream> file, const std::string & filename, bool headerOnly);

	static VideoPath prologVideoName(ui8 index);
	static AudioPath prologMusicName(ui8 index);
	static AudioPath prologVoiceName(ui8 index);

	static constexpr auto VCMP_HEADER_FILE_NAME = "header.json";
public:
	static std::unique_ptr<Campaign> getHeader( const std::string & name); //name - name of appropriate file

	static std::shared_ptr<CampaignState> getCampaign(const std::string & name); //name - name of appropriate file
};

VCMI_LIB_NAMESPACE_END
