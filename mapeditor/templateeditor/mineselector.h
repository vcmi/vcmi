/*
 * mineselector.h, part of VCMI engine
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
#include "../../lib/ResourceSet.h"

namespace Ui {
class MineSelector;
}

class MineSelector : public QDialog
{
	Q_OBJECT

public:
	explicit MineSelector(std::map<GameResID, ui16> & mines);

	static void showMineSelector(std::map<GameResID, ui16> & mines);

private slots:
	void on_buttonBoxResult_accepted();
    void on_buttonBoxResult_rejected();

private:
	Ui::MineSelector *ui;

	std::map<GameResID, ui16> & minesSelected;
};
