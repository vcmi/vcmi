/*
 * modsettings.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "abstractsettings.h"

namespace Ui {
class ModSettings;
}

class ModSettings : public AbstractSettings
{
	Q_OBJECT

public:
	explicit ModSettings(QWidget *parent = nullptr);
	~ModSettings();

	void initialize(MapController & map) override;
	void update() override;

	void updateModWidgetBasedOnMods(const ModCompatibilityInfo & mods, bool leaveCheckedUnchanged = false);

private slots:
	void on_modResolution_map_clicked();

	void on_modResolution_full_clicked();

	void on_treeMods_itemChanged(QTreeWidgetItem *item, int column);

private:
	Ui::ModSettings *ui;
};
