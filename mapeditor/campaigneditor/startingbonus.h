/*
 * startingbonus.h, part of VCMI engine
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

class CampaignBonus;
class CMap;

namespace Ui {
class StartingBonus;
}

class StartingBonus : public QDialog
{
	Q_OBJECT

public:
	explicit StartingBonus(PlayerColor color, std::shared_ptr<CMap> map, CampaignBonus bonus);
	~StartingBonus();

	static bool showStartingBonus(PlayerColor color, std::shared_ptr<CMap> map, CampaignBonus & bonus);
	static QString getBonusListTitle(CampaignBonus bonus, std::shared_ptr<CMap> map);

private slots:
	void on_radioButtonSpell_toggled();
	void on_radioButtonCreature_toggled();
	void on_radioButtonBuilding_toggled();
	void on_radioButtonArtifact_toggled();
	void on_radioButtonSpellScroll_toggled();
	void on_radioButtonPrimarySkill_toggled();
	void on_radioButtonSecondarySkill_toggled();
	void on_radioButtonResource_toggled();
	void on_buttonBox_clicked(QAbstractButton * button);

private:
	Ui::StartingBonus *ui;

	CampaignBonus bonus;
	std::shared_ptr<CMap> map;
	PlayerColor color;

	void initControls();
	void loadBonus();
	void saveBonus();
	void radioButtonToggled(int index);
};
