/*
 * helper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/filesystem/ResourcePath.h"

class CMap;
class CampaignState;
class CRmgTemplate;
class IGameInfoCallback;

namespace Helper
{
	std::unique_ptr<CMap> openMapInternal(const QString &, IGameInfoCallback *);
	std::shared_ptr<CampaignState> openCampaignInternal(const QString &);
	std::map<std::string, std::shared_ptr<CRmgTemplate>> openTemplateInternal(const QString &);
	void saveCampaign(std::shared_ptr<CampaignState> campaignState, const QString & filename);
	void saveTemplate(std::map<std::string, std::shared_ptr<CRmgTemplate>> tpl, const QString & filename);
}
