/*
 * GenerateMapDialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "GenerateMapDialog.h"
#include "ui_GenerateMapDialog.h"
#include "generatorprogress.h"
#include "mainwindow.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/rmg/CMapGenerator.h"
#include "../vcmiqt/launcherdirs.h"
#include <QMessageBox>

GenerateMapDialog::GenerateMapDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::GenerateMapDialog)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowModality(Qt::ApplicationModal);

	ui->widthSpinBox->setRange(36, 144);
	ui->widthSpinBox->setSingleStep(4);
	ui->widthSpinBox->setValue(72);

	ui->heightSpinBox->setRange(36, 144);
	ui->heightSpinBox->setSingleStep(4);
	ui->heightSpinBox->setValue(72);

	ui->playerCountSpinBox->setRange(1, 8);
	ui->playerCountSpinBox->setValue(2);

	ui->levelsSpinBox->setRange(1, 2);
	ui->levelsSpinBox->setValue(1);

	ui->densitySpinBox->setRange(1, 5);
	ui->densitySpinBox->setValue(3);

	ui->waterComboBox->addItem(tr("Random"), EWaterContent::RANDOM);
	ui->waterComboBox->addItem(tr("None"), EWaterContent::NONE);
	ui->waterComboBox->addItem(tr("Normal"), EWaterContent::NORMAL);
	ui->waterComboBox->addItem(tr("Islands"), EWaterContent::ISLANDS);
	ui->waterComboBox->setCurrentIndex(2);

	ui->monsterStrengthComboBox->addItem(tr("Random"), EMonsterStrength::RANDOM);
	ui->monsterStrengthComboBox->addItem(tr("Weak"), EMonsterStrength::GLOBAL_WEAK);
	ui->monsterStrengthComboBox->addItem(tr("Normal"), EMonsterStrength::GLOBAL_NORMAL);
	ui->monsterStrengthComboBox->addItem(tr("Strong"), EMonsterStrength::GLOBAL_STRONG);
	ui->monsterStrengthComboBox->setCurrentIndex(2);

	ui->replaceMapCheckBox->setChecked(true);

	loadUserSettings();
	show();
}

GenerateMapDialog::~GenerateMapDialog()
{
	delete ui;
}

void GenerateMapDialog::loadUserSettings()
{
	QSettings s = CLauncherDirs::getSettings(Ui::appName);
	ui->widthSpinBox->setValue(std::clamp(s.value(settingsWidth, 72).toInt(), 36, 144));
	ui->heightSpinBox->setValue(std::clamp(s.value(settingsHeight, 72).toInt(), 36, 144));
	ui->playerCountSpinBox->setValue(std::clamp(s.value(settingsPlayers, 2).toInt(), 1, 8));
	ui->levelsSpinBox->setValue(std::clamp(s.value(settingsLevels, 1).toInt(), 1, 2));
	ui->replaceMapCheckBox->setChecked(s.value(settingsReplaceMap, true).toBool());
	ui->densitySpinBox->setValue(std::clamp(s.value(settingsDensity, 3).toInt(), 1, 5));

	int waterIdx = ui->waterComboBox->findData(s.value(settingsWater, (int)EWaterContent::NORMAL).toInt());
	if(waterIdx >= 0) ui->waterComboBox->setCurrentIndex(waterIdx);

	int monsterIdx = ui->monsterStrengthComboBox->findData(s.value(settingsMonsterStrength, (int)EMonsterStrength::GLOBAL_NORMAL).toInt());
	if(monsterIdx >= 0) ui->monsterStrengthComboBox->setCurrentIndex(monsterIdx);
}

void GenerateMapDialog::saveUserSettings()
{
	QSettings s = CLauncherDirs::getSettings(Ui::appName);
	s.setValue(settingsWidth, ui->widthSpinBox->value());
	s.setValue(settingsHeight, ui->heightSpinBox->value());
	s.setValue(settingsPlayers, ui->playerCountSpinBox->value());
	s.setValue(settingsWater, ui->waterComboBox->currentData().toInt());
	s.setValue(settingsMonsterStrength, ui->monsterStrengthComboBox->currentData().toInt());
	s.setValue(settingsLevels, ui->levelsSpinBox->value());
	s.setValue(settingsReplaceMap, ui->replaceMapCheckBox->isChecked());
	s.setValue(settingsDensity, ui->densitySpinBox->value());
}

void GenerateMapDialog::on_generateButton_clicked()
{
	saveUserSettings();
	mapGenOptions.setWidth(ui->widthSpinBox->value());
	mapGenOptions.setHeight(ui->heightSpinBox->value());
	mapGenOptions.setHumanOrCpuPlayerCount(ui->playerCountSpinBox->value());
	mapGenOptions.setLevels(ui->levelsSpinBox->value());
	mapGenOptions.setWaterContent(static_cast<EWaterContent::EWaterContent>(ui->waterComboBox->currentData().toInt()));
	mapGenOptions.setMonsterStrength(static_cast<EMonsterStrength::EMonsterStrength>(ui->monsterStrengthComboBox->currentData().toInt()));
	mapGenOptions.setObjectDensity(ui->densitySpinBox->value());
	replaceCurrentMap = ui->replaceMapCheckBox->isChecked();

	if(mapGenOptions.getPossibleTemplates().empty())
	{
		QMessageBox::warning(this, tr("No template"),
			tr("No template available for the selected parameters. Please adjust your settings."));
		return;
	}

	hide();
	EditorMainWindow * mainWindow = static_cast<EditorMainWindow *>(parent());
	int seed = std::time(nullptr);

	try
	{
		CMapGenerator generator(mapGenOptions, mainWindow->controller.getCallback(), seed);
		auto progressBarWnd = new GeneratorProgress(generator, this);
		progressBarWnd->show();

		auto generationTask = std::async(std::launch::async, &CMapGenerator::generate, &generator);
		progressBarWnd->update();
		generatedMap = generationTask.get();

		generatedMap->mods = MapController::modAssessmentMap(*generatedMap);
		progressBarWnd->close();

		if(replaceCurrentMap)
		{
			mainWindow->controller.setMap(std::move(generatedMap));
			mainWindow->initializeMap(true);
		}
		else
		{
			auto * newWindow = new EditorMainWindow();
			newWindow->controller.setMap(std::move(generatedMap));
			newWindow->initializeMap(true);
			newWindow->show();
		}
		close();
	}
	catch(const std::exception & e)
	{
		QMessageBox::critical(this, tr("Map Generation Failed"),
			QString::fromStdString(std::string(e.what())));
		show();
	}
}

void GenerateMapDialog::on_cancelButton_clicked()
{
	reject();
}

std::unique_ptr<CMap> GenerateMapDialog::getGeneratedMap()
{
	return std::move(generatedMap);
}

