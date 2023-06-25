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

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CampaignHandler
{
	static std::string readLocalizedString(CBinaryReader & reader, std::string filename, std::string modName, std::string encoding, std::string identifier);
	
	//parsers for VCMI campaigns (*.vcmp)
	static CampaignHeader readHeaderFromJson(JsonNode & reader, std::string filename, std::string modName, std::string encoding);
	static CampaignScenario readScenarioFromJson(JsonNode & reader);
	static CampaignTravel readScenarioTravelFromJson(JsonNode & reader);

	//parsers for original H3C campaigns
	static CampaignHeader readHeaderFromMemory(CBinaryReader & reader, std::string filename, std::string modName, std::string encoding);
	static CampaignScenario readScenarioFromMemory(CBinaryReader & reader, const CampaignHeader & header);
	static CampaignTravel readScenarioTravelFromMemory(CBinaryReader & reader, CampaignVersion version);
	/// returns h3c split in parts. 0 = h3c header, 1-end - maps (binary h3m)
	/// headerOnly - only header will be decompressed, returned vector wont have any maps
	static std::vector<std::vector<ui8>> getFile(std::unique_ptr<CInputStream> file, bool headerOnly);

	static std::string prologVideoName(ui8 index);
	static std::string prologMusicName(ui8 index);
	static std::string prologVoiceName(ui8 index);

public:
	static std::unique_ptr<CampaignHeader> getHeader( const std::string & name); //name - name of appropriate file

	static std::shared_ptr<CampaignState> getCampaign(const std::string & name); //name - name of appropriate file
};

VCMI_LIB_NAMESPACE_END
