/*
 * windownewmap.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "../lib/mapping/CMap.h"
#include "../lib/rmg/CRmgTemplateStorage.h"
#include "../lib/rmg/CRmgTemplate.h"
#include "../lib/rmg/CMapGenerator.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/mapping/MapFormat.h"
#include "../lib/CGeneralTextHandler.h"

#include "windownewmap.h"
#include "ui_windownewmap.h"
#include "mainwindow.h"
#include "generatorprogress.h"

WindowNewMap::WindowNewMap(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::WindowNewMap)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	setWindowModality(Qt::ApplicationModal);
	
	for(auto * combo : {ui->humanCombo, ui->cpuCombo, ui->humanTeamsCombo, ui->cpuTeamsCombo})
		combo->clear();
	
	//prepare human players combo box
	for(int i = 0; i <= PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		ui->humanCombo->addItem(!i ? randomString : QString::number(players.at(i)));
		ui->humanCombo->setItemData(i, QVariant(players.at(i)));
		
		ui->cpuCombo->addItem(!i ? randomString : QString::number(cpuPlayers.at(i)));
		ui->cpuCombo->setItemData(i, QVariant(cpuPlayers.at(i)));
		
		ui->humanTeamsCombo->addItem(!i ? randomString : QString::number(cpuPlayers.at(i)));
		ui->humanTeamsCombo->setItemData(i, QVariant(cpuPlayers.at(i)));
		
		ui->cpuTeamsCombo->addItem(!i ? randomString : QString::number(cpuPlayers.at(i)));
		ui->cpuTeamsCombo->setItemData(i, QVariant(cpuPlayers.at(i)));
	}
	
	for(auto * combo : {ui->humanCombo, ui->cpuCombo, ui->humanTeamsCombo, ui->cpuTeamsCombo})
		combo->setCurrentIndex(0);

	loadUserSettings();

	show();

	//setup initial parameters
	int width = ui->widthTxt->text().toInt();
	int height = ui->heightTxt->text().toInt();
	mapGenOptions.setWidth(width ? width : 1);
	mapGenOptions.setHeight(height ? height : 1);
	bool twoLevel = ui->twoLevelCheck->isChecked();
	mapGenOptions.setHasTwoLevels(twoLevel);
	updateTemplateList();
}

WindowNewMap::~WindowNewMap()
{
	saveUserSettings();
	delete ui;
}

void WindowNewMap::loadUserSettings()
{
	//load window settings
	QSettings s(Ui::teamName, Ui::appName);

	auto width = s.value(newMapWidth);
	if (width.isValid())
	{
		ui->widthTxt->setText(width.toString());
	}
	auto height = s.value(newMapHeight);
	if (height.isValid())
	{
		ui->heightTxt->setText(height.toString());
	}
	auto twoLevel = s.value(newMapTwoLevel);
	if (twoLevel.isValid())
	{
		ui->twoLevelCheck->setChecked(twoLevel.toBool());
	}
	auto generateRandom = s.value(newMapGenerateRandom);
	if (generateRandom.isValid())
	{
		ui->randomMapCheck->setChecked(generateRandom.toBool());
	}
	auto players = s.value(newMapPlayers);
	if (players.isValid())
	{
		ui->humanCombo->setCurrentIndex(players.toInt());
	}
	auto cpuPlayers = s.value(newMapCpuPlayers);
	if (cpuPlayers.isValid())
	{
		ui->cpuCombo->setCurrentIndex(cpuPlayers.toInt());
	}
	auto teams = s.value(newMapHumanTeams);
	if(teams.isValid())
	{
		ui->humanTeamsCombo->setCurrentIndex(teams.toInt());
	}
	auto cputeams = s.value(newMapCpuTeams);
	if(cputeams.isValid())
	{
		ui->cpuTeamsCombo->setCurrentIndex(cputeams.toInt());
	}
	
	auto waterContent = s.value(newMapWaterContent);
	if (waterContent.isValid())
	{
		switch (waterContent.toInt())
		{
			case EWaterContent::RANDOM:
				ui->waterOpt1->setChecked(true); break;
			case EWaterContent::NONE:
				ui->waterOpt2->setChecked(true); break;
			case EWaterContent::NORMAL:
				ui->waterOpt3->setChecked(true); break;
			case EWaterContent::ISLANDS:
				ui->waterOpt4->setChecked(true); break;
		}

	}
	auto monsterStrength = s.value(newMapMonsterStrength);
	if (monsterStrength.isValid())
	{
		switch (monsterStrength.toInt())
		{
			case EMonsterStrength::RANDOM:
				ui->monsterOpt1->setChecked(true); break;
			case EMonsterStrength::GLOBAL_WEAK:
				ui->monsterOpt2->setChecked(true); break;
			case EMonsterStrength::GLOBAL_NORMAL:
				ui->monsterOpt3->setChecked(true); break;
			case EMonsterStrength::GLOBAL_STRONG:
				ui->monsterOpt4->setChecked(true); break;
		}
	}

	auto templateName = s.value(newMapTemplate);
	if (templateName.isValid())
	{
		updateTemplateList();
		
		auto* templ = VLC->tplh->getTemplate(templateName.toString().toStdString());
		if (templ)
		{
			ui->templateCombo->setCurrentText(templateName.toString());
			//TODO: validate inside this method
			mapGenOptions.setMapTemplate(templ);
		}
		else
		{
			//Display problem on status bar
		}
	}
}

void WindowNewMap::saveUserSettings()
{
	QSettings s(Ui::teamName, Ui::appName);
	s.setValue(newMapWidth, ui->widthTxt->text().toInt());
	s.setValue(newMapHeight, ui->heightTxt->text().toInt());
	s.setValue(newMapTwoLevel, ui->twoLevelCheck->isChecked());
	s.setValue(newMapGenerateRandom, ui->randomMapCheck->isChecked());

	s.setValue(newMapPlayers,ui->humanCombo->currentIndex());
	s.setValue(newMapCpuPlayers,ui->cpuCombo->currentIndex());
	s.setValue(newMapHumanTeams, ui->humanTeamsCombo->currentIndex());
	s.setValue(newMapCpuTeams, ui->cpuTeamsCombo->currentIndex());

	EWaterContent::EWaterContent water = EWaterContent::RANDOM;
	if(ui->waterOpt1->isChecked())
		water = EWaterContent::RANDOM;
	else if(ui->waterOpt2->isChecked())
		water = EWaterContent::NONE;
	else if(ui->waterOpt3->isChecked())
		water = EWaterContent::NORMAL;
	else if(ui->waterOpt4->isChecked())
		water = EWaterContent::ISLANDS;
	s.setValue(newMapWaterContent, static_cast<int>(water));

	EMonsterStrength::EMonsterStrength monster = EMonsterStrength::RANDOM;
	if(ui->monsterOpt1->isChecked())
		monster = EMonsterStrength::RANDOM;
	else if(ui->monsterOpt2->isChecked())
		monster = EMonsterStrength::GLOBAL_WEAK;
	else if(ui->monsterOpt3->isChecked())
		monster = EMonsterStrength::GLOBAL_NORMAL;
	else if(ui->monsterOpt4->isChecked())
		monster = EMonsterStrength::GLOBAL_STRONG;
	s.setValue(newMapMonsterStrength, static_cast<int>(monster));

	auto templateName = ui->templateCombo->currentText();
	if (templateName.size())
	{
		s.setValue(newMapTemplate, templateName);
	}
}

void WindowNewMap::on_cancelButton_clicked()
{
	close();
}

void generateRandomMap(CMapGenerator & gen, MainWindow * window)
{
	window->controller.setMap(gen.generate());
}

std::unique_ptr<CMap> generateEmptyMap(CMapGenOptions & options)
{
	std::unique_ptr<CMap> map(new CMap);
	map->version = EMapFormat::VCMI;
	map->width = options.getWidth();
	map->height = options.getHeight();
	map->twoLevel = options.getHasTwoLevels();
	
	map->initTerrain();
	map->getEditManager()->clearTerrain(&CRandomGenerator::getDefault());

	return map;
}

void WindowNewMap::on_okButton_clicked()
{
	EWaterContent::EWaterContent water = EWaterContent::RANDOM;
	EMonsterStrength::EMonsterStrength monster = EMonsterStrength::RANDOM;
	if(ui->waterOpt1->isChecked())
		water = EWaterContent::RANDOM;
	if(ui->waterOpt2->isChecked())
		water = EWaterContent::NONE;
	if(ui->waterOpt3->isChecked())
		water = EWaterContent::NORMAL;
	if(ui->waterOpt4->isChecked())
		water = EWaterContent::ISLANDS;
	if(ui->monsterOpt1->isChecked())
		monster = EMonsterStrength::RANDOM;
	if(ui->monsterOpt2->isChecked())
		monster = EMonsterStrength::GLOBAL_WEAK;
	if(ui->monsterOpt3->isChecked())
		monster = EMonsterStrength::GLOBAL_NORMAL;
	if(ui->monsterOpt4->isChecked())
		monster = EMonsterStrength::GLOBAL_STRONG;

	mapGenOptions.setWaterContent(water);
	mapGenOptions.setMonsterStrength(monster);
	
	std::unique_ptr<CMap> nmap;
	if(ui->randomMapCheck->isChecked())
	{
		//verify map template
		if(mapGenOptions.getPossibleTemplates().empty())
		{
			QMessageBox::warning(this, tr("No template"), tr("No template for parameters scecified. Random map cannot be generated."));
			return;
		}
		
		int seed = std::time(nullptr);
		if(ui->checkSeed->isChecked() && !ui->lineSeed->text().isEmpty())
			seed = ui->lineSeed->text().toInt();
			
		CMapGenerator generator(mapGenOptions, seed);
		auto progressBarWnd = new GeneratorProgress(generator, this);
		progressBarWnd->show();
	
		try
		{
			auto f = std::async(std::launch::async, &CMapGenerator::generate, &generator);
			progressBarWnd->update();
			nmap = f.get();
		}
		catch(const std::exception & e)
		{
			QMessageBox::critical(this, tr("RMG failure"), e.what());
		}
	}
	else
	{		
		auto f = std::async(std::launch::async, &::generateEmptyMap, std::ref(mapGenOptions));
		nmap = f.get();
	}
	
	nmap->mods = MapController::modAssessmentAll();
	static_cast<MainWindow*>(parent())->controller.setMap(std::move(nmap));
	static_cast<MainWindow*>(parent())->initializeMap(true);
	close();
}

void WindowNewMap::on_sizeCombo_activated(int index)
{
	ui->widthTxt->setText(QString::number(mapSizes.at(index).first));
	ui->heightTxt->setText(QString::number(mapSizes.at(index).second));
}


void WindowNewMap::on_twoLevelCheck_stateChanged(int arg1)
{
	bool twoLevel = ui->twoLevelCheck->isChecked();
	mapGenOptions.setHasTwoLevels(twoLevel);
	updateTemplateList();
}


void WindowNewMap::on_humanCombo_activated(int index)
{
	int humans = ui->humanCombo->currentData().toInt();
	if(humans > PlayerColor::PLAYER_LIMIT_I)
	{
		humans = PlayerColor::PLAYER_LIMIT_I;
		ui->humanCombo->setCurrentIndex(humans);
	}

	mapGenOptions.setPlayerCount(humans);

	int teams = mapGenOptions.getTeamCount();
	if(teams > humans - 1)
	{
		teams = humans > 0 ? humans - 1 : CMapGenOptions::RANDOM_SIZE;
		ui->humanTeamsCombo->setCurrentIndex(teams + 1); //skip one element because first is random
	}

	int cpu = mapGenOptions.getCompOnlyPlayerCount();
	if(cpu > PlayerColor::PLAYER_LIMIT_I - humans)
	{
		cpu = PlayerColor::PLAYER_LIMIT_I - humans;
		ui->cpuCombo->setCurrentIndex(cpu + 1); //skip one element because first is random
	}

	int cpuTeams = mapGenOptions.getCompOnlyTeamCount(); //comp only players - 1
	if(cpuTeams > cpu - 1)
	{
		cpuTeams = cpu > 0 ? cpu - 1 : CMapGenOptions::RANDOM_SIZE;
		ui->cpuTeamsCombo->setCurrentIndex(cpuTeams + 1); //skip one element because first is random
	}

	updateTemplateList();
}


void WindowNewMap::on_cpuCombo_activated(int index)
{
	int humans = mapGenOptions.getPlayerCount();
	int cpu = ui->cpuCombo->currentData().toInt();
	if(cpu > PlayerColor::PLAYER_LIMIT_I - humans)
	{
		cpu = PlayerColor::PLAYER_LIMIT_I - humans;
		ui->cpuCombo->setCurrentIndex(cpu + 1); //skip one element because first is random
	}
	
	mapGenOptions.setCompOnlyPlayerCount(cpu);
	
	int cpuTeams = mapGenOptions.getCompOnlyTeamCount(); //comp only players - 1
	if(cpuTeams > cpu - 1)
	{
		cpuTeams = cpu > 0 ? cpu - 1 : CMapGenOptions::RANDOM_SIZE;
		ui->cpuTeamsCombo->setCurrentIndex(cpuTeams + 1); //skip one element because first is random
	}

	updateTemplateList();
}


void WindowNewMap::on_randomMapCheck_stateChanged(int arg1)
{
	randomMap = ui->randomMapCheck->isChecked();
	ui->templateCombo->setEnabled(randomMap);
	updateTemplateList();
}


void WindowNewMap::on_templateCombo_activated(int index)
{
	if(index == 0)
	{
		mapGenOptions.setMapTemplate(nullptr);
		return;
	}
	
	auto * templ = data_cast<const CRmgTemplate>(ui->templateCombo->currentData().toLongLong());
	mapGenOptions.setMapTemplate(templ);
}


void WindowNewMap::on_widthTxt_textChanged(const QString &arg1)
{
	int sz = arg1.toInt();
	if(sz > 1)
	{
		mapGenOptions.setWidth(arg1.toInt());
		updateTemplateList();
	}
}


void WindowNewMap::on_heightTxt_textChanged(const QString &arg1)
{
	int sz = arg1.toInt();
	if(sz > 1)
	{
		mapGenOptions.setHeight(arg1.toInt());
		updateTemplateList();
	}
}

void WindowNewMap::updateTemplateList()
{
	ui->templateCombo->clear();
	ui->templateCombo->setCurrentIndex(-1);

	if(!randomMap)
		return;

	mapGenOptions.setMapTemplate(nullptr);
	auto templates = mapGenOptions.getPossibleTemplates();
	if(templates.empty())
		return;

	ui->templateCombo->addItem("[default]", 0);

	for(auto * templ : templates)
	{
		ui->templateCombo->addItem(QString::fromStdString(templ->getName()), data_cast(templ));
	}

	ui->templateCombo->setCurrentIndex(0);
}

void WindowNewMap::on_checkSeed_toggled(bool checked)
{
	ui->lineSeed->setEnabled(checked);
}


void WindowNewMap::on_humanTeamsCombo_activated(int index)
{
	int humans = mapGenOptions.getPlayerCount();
	int teams = ui->humanTeamsCombo->currentData().toInt();
	if(teams >= humans)
	{
		teams = humans > 0 ? humans - 1 : CMapGenOptions::RANDOM_SIZE;
		ui->humanTeamsCombo->setCurrentIndex(teams + 1); //skip one element because first is random
	}

	mapGenOptions.setTeamCount(teams);
	updateTemplateList();
}


void WindowNewMap::on_cpuTeamsCombo_activated(int index)
{
	int cpu = mapGenOptions.getCompOnlyPlayerCount();
	int cpuTeams = ui->cpuTeamsCombo->currentData().toInt();
	if(cpuTeams >= cpu)
	{
		cpuTeams = cpu > 0 ? cpu - 1 : CMapGenOptions::RANDOM_SIZE;
		ui->cpuTeamsCombo->setCurrentIndex(cpuTeams + 1); //skip one element because first is random
	}

	mapGenOptions.setCompOnlyTeamCount(cpuTeams);
	updateTemplateList();
}

