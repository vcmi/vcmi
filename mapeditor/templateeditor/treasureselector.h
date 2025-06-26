/*
 * treasureselector.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QWidget>

#include "../StdInc.h"
#include "../../lib/rmg/CRmgTemplate.h"

namespace Ui {
class TreasureSelector;
}

class TreasureSelector : public QDialog
{
	Q_OBJECT

public:
	explicit TreasureSelector(std::vector<CTreasureInfo> & treasures);

	static void showTreasureSelector(std::vector<CTreasureInfo> & treasures);

private slots:
	void on_buttonBoxResult_accepted();
    void on_buttonBoxResult_rejected();

private:
	Ui::TreasureSelector *ui;

	std::vector<CTreasureInfo> & treasures;
};
