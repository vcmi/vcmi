/*
 * generalsettings.h, part of VCMI engine
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
class GeneralSettings;
}

class GeneralSettings : public AbstractSettings
{
	Q_OBJECT

public:
	explicit GeneralSettings(QWidget *parent = nullptr);
	~GeneralSettings();

	void initialize(MapController & map) override;
	void update() override;

private slots:
	void on_heroLevelLimitCheck_toggled(bool checked);

private:
	Ui::GeneralSettings *ui;
};
