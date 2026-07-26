/*
 * maplayerselectiondialog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QDialog>
#include <vector>
#include "../lib/constants/EntityIdentifiers.h"

class QComboBox;

namespace Ui
{
class MapLayerSelectionDialog;
}

class MapLayerSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit MapLayerSelectionDialog(int levelCount, const std::vector<MapLayerId> & currentLayers, QWidget *parent = nullptr);
	~MapLayerSelectionDialog();

	std::vector<MapLayerId> getSelectedLayers() const;

private:
	Ui::MapLayerSelectionDialog *ui;
	int levelCount;
	std::vector<QComboBox *> layerCombos;
};
