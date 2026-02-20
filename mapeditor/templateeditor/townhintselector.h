/*
 * townhintselector.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QWidget>

#include "../../lib/rmg/CRmgTemplate.h"

namespace Ui {
class TownHintSelector;
}

class TownHintSelector : public QDialog
{
	Q_OBJECT

public:
	explicit TownHintSelector(std::vector<rmg::ZoneOptions::CTownHints> & townHints);

	static void showTownHintSelector(std::vector<rmg::ZoneOptions::CTownHints> & townHints);

private slots:
	void on_buttonBoxResult_accepted();
    void on_buttonBoxResult_rejected();

private:
	Ui::TownHintSelector *ui;

	std::vector<rmg::ZoneOptions::CTownHints> & townHints;
};
