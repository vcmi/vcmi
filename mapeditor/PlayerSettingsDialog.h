/*
 * playersettings.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QDialog>
#include "playerparams.h"

namespace Ui {
class PlayerSettingsDialog;
}

class PlayerSettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PlayerSettingsDialog(MapController & controller, QWidget *parent = nullptr);
	~PlayerSettingsDialog();

private slots:

	void on_playersCount_currentIndexChanged(int index);

	void on_pushButton_clicked();

private:
	Ui::PlayerSettingsDialog *ui;

	std::vector<PlayerParams*> paramWidgets;
	
	MapController & controller;
};
