/*
 * EditorFiles.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class CMap;
class CampaignState;
class CRmgTemplate;
class IGameInfoCallback;

/// Loading and saving of the files that are edited in the map editor. Every function throws on failure
namespace EditorFiles
{
	std::unique_ptr<CMap> openMap(const boost::filesystem::path & file, IGameInfoCallback * cb);
	std::shared_ptr<CampaignState> openCampaign(const boost::filesystem::path & file);
	std::map<std::string, std::shared_ptr<CRmgTemplate>> openTemplate(const boost::filesystem::path & file);

	void saveMap(const std::unique_ptr<CMap> & map, const boost::filesystem::path & file);
	void saveCampaign(const std::shared_ptr<CampaignState> & campaignState, const boost::filesystem::path & file);
	void saveTemplate(const std::map<std::string, std::shared_ptr<CRmgTemplate>> & templates, const boost::filesystem::path & file);
}
