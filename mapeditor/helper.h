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

class CMap;
class CampaignState;

namespace Helper
{
	std::unique_ptr<CMap> openMapInternal(const QString &);
	std::shared_ptr<CampaignState> openCampaignInternal(const QString &);
	void saveCampaign(std::shared_ptr<CampaignState> campaignState, const QString & filename);
}