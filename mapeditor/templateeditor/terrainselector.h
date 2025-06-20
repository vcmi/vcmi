/*
 * terrainselector.h, part of VCMI engine
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
class TerrainSelector;
}

class TerrainSelector : public QDialog
{
	Q_OBJECT

public:
	explicit TerrainSelector(std::set<TerrainId> & terrains);

	static void showTerrainSelector(std::set<TerrainId> & terrains);

private slots:
	void on_buttonBoxResult_accepted();
    void on_buttonBoxResult_rejected();

private:
	Ui::TerrainSelector *ui;

	std::set<TerrainId> & terrainsSelected;
};
