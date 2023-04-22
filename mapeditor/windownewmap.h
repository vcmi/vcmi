/*
 * windownewmap.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QDialog>
#include "../lib/rmg/CMapGenOptions.h"

namespace Ui
{
	class WindowNewMap;
}

class WindowNewMap : public QDialog
{
	Q_OBJECT

	const QString newMapWidth = "NewMapWindow/Width";
	const QString newMapHeight = "NewMapWindow/Height";
	const QString newMapTwoLevel = "NewMapWindow/TwoLevel";
	const QString newMapGenerateRandom = "NewMapWindow/GenerateRandom";
	const QString newMapPlayers = "NewMapWindow/Players";		//map index
	const QString newMapCpuPlayers = "NewMapWindow/CpuPlayers"; //map index
	const QString newMapHumanTeams = "NewMapWindow/HumanTeams"; //map index
	const QString newMapCpuTeams = "NewMapWindow/CpuTeams";     //map index
	const QString newMapWaterContent = "NewMapWindow/WaterContent";
	const QString newMapMonsterStrength = "NewMapWindow/MonsterStrength";
	const QString newMapTemplate = "NewMapWindow/Template";
	
	const QString randomString = "Random";

	const std::map<int, int> players
	{
		{0, CMapGenOptions::RANDOM_SIZE},
		{1, 1},
		{2, 2},
		{3, 3},
		{4, 4},
		{5, 5},
		{6, 6},
		{7, 7},
		{8, 8}
	};

	const std::map<int, int> cpuPlayers
	{
		{0, CMapGenOptions::RANDOM_SIZE},
		{1, 0},
		{2, 1},
		{3, 2},
		{4, 3},
		{5, 4},
		{6, 5},
		{7, 6},
		{8, 7}
	};
	
	const std::map<int, std::pair<int, int>> mapSizes
	{
		{0, {36, 36}},
		{1, {72, 72}},
		{2, {108, 108}},
		{3, {144, 144}},
	};

public:
	explicit WindowNewMap(QWidget *parent = nullptr);
	~WindowNewMap();

private slots:
	void on_cancelButton_clicked();

	void on_okButton_clicked();

	void on_sizeCombo_activated(int index);

	void on_twoLevelCheck_stateChanged(int arg1);

	void on_humanCombo_activated(int index);

	void on_cpuCombo_activated(int index);

	void on_randomMapCheck_stateChanged(int arg1);

	void on_templateCombo_activated(int index);

	void on_widthTxt_textChanged(const QString &arg1);

	void on_heightTxt_textChanged(const QString &arg1);

	void on_checkSeed_toggled(bool checked);

	void on_humanTeamsCombo_activated(int index);

	void on_cpuTeamsCombo_activated(int index);

private:

	void updateTemplateList();

	void loadUserSettings();
	void saveUserSettings();

private:
	Ui::WindowNewMap *ui;

	CMapGenOptions mapGenOptions;
	bool randomMap = false;
};
