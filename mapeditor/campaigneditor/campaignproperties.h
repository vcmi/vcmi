/*
 * campaignproperties.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QDialog>

#include "../../lib/campaign/CampaignState.h"

class CampaignState;
class QAbstractButton;

namespace Ui {
class CampaignProperties;
}

class CampaignProperties : public QDialog
{
	Q_OBJECT

public:
	explicit CampaignProperties(std::shared_ptr<CampaignState> campaignState);
	~CampaignProperties();

	static bool showCampaignProperties(std::shared_ptr<CampaignState> campaignState);
	
private slots:
	void on_buttonBox_clicked(QAbstractButton * button);
	void on_comboBoxRegionPreset_currentIndexChanged(int index);
	void on_pushButtonRegionAdd_clicked();
	void on_pushButtonRegionRemove_clicked();
	void on_checkBoxVideoRim_toggled(bool checked);

private:
	Ui::CampaignProperties *ui;

	std::shared_ptr<CampaignState> campaignState;
	CampaignRegions regions;

	void loadRegion();
	bool saveRegion();
};
