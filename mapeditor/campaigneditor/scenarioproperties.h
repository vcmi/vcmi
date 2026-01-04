/*
 * scenarioproperties.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QDialog>

#include "lib/constants/EntityIdentifiers.h"
#include "lib/campaign/CampaignState.h"

class CMap;
class CampaignState;
class QAbstractButton;

namespace Ui {
class ScenarioProperties;
}

Q_DECLARE_METATYPE(CampaignBonus)

class ScenarioProperties : public QDialog
{
	Q_OBJECT

public:
	explicit ScenarioProperties(std::shared_ptr<CampaignState> campaignState, CampaignScenarioID scenario);
	~ScenarioProperties();

	static bool showScenarioProperties(std::shared_ptr<CampaignState> campaignState, CampaignScenarioID scenario);
	
private slots:
	void on_buttonBox_clicked(QAbstractButton * button);
	void on_pushButtonCreatureTypeAll_clicked();
	void on_pushButtonCreatureTypeNone_clicked();

	void on_pushButtonImport_clicked();
	void on_pushButtonExport_clicked();
	void on_pushButtonRemove_clicked();

	void on_radioButtonStartingOptionBonus_toggled();
	void on_radioButtonStartingOptionHeroCrossover_toggled();
	void on_radioButtonStartingOptionStartingHero_toggled();

	void on_pushButtonStartingAdd_clicked();
	void on_pushButtonStartingEdit_clicked();
	void on_pushButtonStartingRemove_clicked();

private:
	Ui::ScenarioProperties *ui;

	std::shared_ptr<CMap> map;
	std::shared_ptr<CampaignState> campaignState;
	CampaignScenarioID scenario;
	
	std::map<int, std::string> heroSelection;

	void reloadMapRelatedUi();
	void reloadEnableState();
};
