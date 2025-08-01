/*
 * factionselector.h, part of VCMI engine
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
#include "../../lib/constants/EntityIdentifiers.h"

namespace Ui {
class FactionSelector;
}

class FactionSelector : public QDialog
{
	Q_OBJECT

public:
	explicit FactionSelector(std::set<FactionID> & factions);

	static void showFactionSelector(std::set<FactionID> & factions);

private slots:
	void on_buttonBoxResult_accepted();
    void on_buttonBoxResult_rejected();

private:
	Ui::FactionSelector *ui;

	std::set<FactionID> & factionsSelected;
};
