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

	loadUserSettings();

	show();

	//setup initial parameters
	mapGenOptions.setWidth(ui->widthTxt->text().toInt());
	mapGenOptions.setHeight(ui->heightTxt->text().toInt());
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
	//TODO: teams when implemented

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
	//TODO: teams when implemented

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

	return std::move(map);
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
			QMessageBox::warning(this, "No template", "No template for parameters scecified. Random map cannot be generated.");
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
			QMessageBox::critical(this, "RMG failure", e.what());
		}
	}
	else
	{		
		auto f = std::async(std::launch::async, &::generateEmptyMap, std::ref(mapGenOptions));
		nmap = f.get();
	}
	

	static_cast<MainWindow*>(parent())->controller.setMap(std::move(nmap));
	static_cast<MainWindow*>(parent())->initializeMap(true);
	close();
}

void WindowNewMap::on_sizeCombo_activated(int index)
{
	std::map<int, std::pair<int, int>> sizes
	{
		{0, {36, 36}},
		{1, {72, 72}},
		{2, {108, 108}},
		{3, {144, 144}},
	};

	ui->widthTxt->setText(QString::number(sizes[index].first));
	ui->heightTxt->setText(QString::number(sizes[index].second));
}


void WindowNewMap::on_twoLevelCheck_stateChanged(int arg1)
{
	bool twoLevel = ui->twoLevelCheck->isChecked();
	mapGenOptions.setHasTwoLevels(twoLevel);
	updateTemplateList();
}


void WindowNewMap::on_humanCombo_activated(int index)
{
	int humans = players.at(index);
	if(humans > playerLimit)
	{
		humans = playerLimit;
		ui->humanCombo->setCurrentIndex(humans);
		return;
	}

	mapGenOptions.setPlayerCount(humans);

	int teams = mapGenOptions.getTeamCount();
	if(teams > humans - 1)
	{
		teams = humans - 1;
		//TBD
	}

	int cpu = mapGenOptions.getCompOnlyPlayerCount();
	if(cpu > playerLimit - humans)
	{
		cpu = playerLimit - humans;
		ui->cpuCombo->setCurrentIndex(cpu + 1);
	}

	int cpuTeams = mapGenOptions.getCompOnlyTeamCount(); //comp only players - 1
	if(cpuTeams > cpu - 1)
	{
		cpuTeams = cpu - 1;
		//TBD
	}

	//void setMapTemplate(const CRmgTemplate * value);
	updateTemplateList();
}


void WindowNewMap::on_cpuCombo_activated(int index)
{
	int humans = mapGenOptions.getPlayerCount();
	int cpu = cpuPlayers.at(index);
	if(cpu > playerLimit - humans)
	{
		cpu = playerLimit - humans;
		ui->cpuCombo->setCurrentIndex(cpu + 1);
		return;
	}

	mapGenOptions.setCompOnlyPlayerCount(cpu);
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

