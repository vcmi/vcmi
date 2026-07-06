/*
 * GenerateMapDialog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QDialog>
#include <memory>
#include "../lib/rmg/CMapGenOptions.h"

VCMI_LIB_USING_NAMESPACE

class CMap;

namespace Ui
{
	class GenerateMapDialog;
}

class GenerateMapDialog : public QDialog
{
	Q_OBJECT

	const QString settingsWidth = "GenerateMapDialog/Width";
	const QString settingsHeight = "GenerateMapDialog/Height";
	const QString settingsPlayers = "GenerateMapDialog/Players";
	const QString settingsWater = "GenerateMapDialog/Water";
	const QString settingsMonsterStrength = "GenerateMapDialog/MonsterStrength";
	const QString settingsLevels = "GenerateMapDialog/Levels";
	const QString settingsReplaceMap = "GenerateMapDialog/ReplaceMap";
	const QString settingsDensity = "GenerateMapDialog/ObjectDensity";

public:
	explicit GenerateMapDialog(QWidget * parent = nullptr);
	~GenerateMapDialog();

	const CMapGenOptions & getMapGenOptions() const { return mapGenOptions; }
	bool shouldReplaceCurrentMap() const { return replaceCurrentMap; }
	std::unique_ptr<CMap> getGeneratedMap();

private slots:
	void on_generateButton_clicked();
	void on_cancelButton_clicked();

private:
	void loadUserSettings();
	void saveUserSettings();

	Ui::GenerateMapDialog * ui;
	CMapGenOptions mapGenOptions;
	std::unique_ptr<CMap> generatedMap;
	bool replaceCurrentMap = true;
};
