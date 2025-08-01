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
#include "../lib/GameLibrary.h"
#include "../lib/mapping/CMapEditManager.h"
#include "../lib/mapping/MapFormat.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/serializer/JsonSerializer.h"
#include "../lib/serializer/JsonDeserializer.h"

#include "../vcmiqt/jsonutils.h"
#include "windownewmap.h"
#include "ui_windownewmap.h"
#include "mainwindow.h"
#include "generatorprogress.h"

WindowNewMap::WindowNewMap(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::WindowNewMap)
{
	ui->setupUi(this);

	setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

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

	on_sizeStandardRadio_toggled(true);
	on_checkSeed_toggled(false);

	bool useLoaded = loadUserSettings();
	if (!useLoaded)
	{
		for(auto * combo : {ui->humanCombo, ui->cpuCombo, ui->humanTeamsCombo, ui->cpuTeamsCombo})
			combo->setCurrentIndex(0);
	}

	show();

	if (!useLoaded)
	{
		//setup initial parameters
		int width = ui->widthTxt->text().toInt();
		int height = ui->heightTxt->text().toInt();
		mapGenOptions.setWidth(width ? width : 1);
		mapGenOptions.setHeight(height ? height : 1);
		bool twoLevel = ui->twoLevelCheck->isChecked();
		mapGenOptions.setHasTwoLevels(twoLevel);

		updateTemplateList();
	}
}

WindowNewMap::~WindowNewMap()
{
	delete ui;
}

bool WindowNewMap::loadUserSettings()
{
	bool ret = false;
	CRmgTemplate * templ = nullptr;

	QSettings s(Ui::teamName, Ui::appName);

	auto generateRandom = s.value(newMapGenerateRandom);
	if (generateRandom.isValid())
	{
		ui->randomMapCheck->setChecked(generateRandom.toBool());
	}

	auto settings = s.value(newMapWindow);

	if (settings.isValid())
	{
		auto node = JsonUtils::toJson(settings);
		JsonDeserializer handler(nullptr, node);
		handler.serializeStruct("lastSettings", mapGenOptions);
		templ = const_cast<CRmgTemplate*>(mapGenOptions.getMapTemplate()); // Remember for later

		ui->widthTxt->setValue(mapGenOptions.getWidth());
		ui->heightTxt->setValue(mapGenOptions.getHeight());
		for(const auto & sz : mapSizes)
		{
			if(sz.second.first == mapGenOptions.getWidth() &&
			sz.second.second == mapGenOptions.getHeight())
			{
				ui->sizeCombo->setCurrentIndex(sz.first);
				break;
			}
		}

		ui->twoLevelCheck->setChecked(mapGenOptions.getHasTwoLevels());

		ui->humanCombo->setCurrentIndex(mapGenOptions.getHumanOrCpuPlayerCount());
		ui->cpuCombo->setCurrentIndex(mapGenOptions.getCompOnlyPlayerCount());
		ui->humanTeamsCombo->setCurrentIndex(mapGenOptions.getTeamCount());
		ui->cpuTeamsCombo->setCurrentIndex(mapGenOptions.getCompOnlyTeamCount());

		switch (mapGenOptions.getWaterContent())
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

		switch (mapGenOptions.getMonsterStrength())
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

		ui->roadDirt->setChecked(mapGenOptions.isRoadEnabled(Road::DIRT_ROAD));
		ui->roadGravel->setChecked(mapGenOptions.isRoadEnabled(Road::GRAVEL_ROAD));
		ui->roadCobblestone->setChecked(mapGenOptions.isRoadEnabled(Road::COBBLESTONE_ROAD));

		ret = true;
	}

	updateTemplateList();
	mapGenOptions.setMapTemplate(templ); // Can be null

	if (templ)
	{
		std::string name = templ->getName();
		for (size_t i = 0; i < ui->templateCombo->count(); i++)
		{
			if (ui->templateCombo->itemText(i).toStdString() == name)
			{
				ui->templateCombo->setCurrentIndex(i);
				break;
			}
		}
		ret = true;
	}

	return ret;
}

void WindowNewMap::saveUserSettings()
{
	QSettings s(Ui::teamName, Ui::appName);

	JsonNode data;
	JsonSerializer ser(nullptr, data);

	ser.serializeStruct("lastSettings", mapGenOptions);

	auto variant = JsonUtils::toVariant(data);
	s.setValue(newMapWindow, variant);
	s.setValue(newMapGenerateRandom, ui->randomMapCheck->isChecked());
}

void WindowNewMap::on_cancelButton_clicked()
{
	saveUserSettings();
	close();
}

void generateRandomMap(CMapGenerator & gen, MainWindow * window)
{
	window->controller.setMap(gen.generate());
}

std::unique_ptr<CMap> generateEmptyMap(CMapGenOptions & options)
{
	auto map = std::make_unique<CMap>(nullptr);
	map->version = EMapFormat::VCMI;
	map->creationDateTime = std::time(nullptr);
	map->width = options.getWidth();
	map->height = options.getHeight();
	map->twoLevel = options.getHasTwoLevels();
	
	map->initTerrain();
	map->getEditManager()->clearTerrain(&CRandomGenerator::getDefault());

	return map;
}

std::pair<int, int> getSelectedMapSize(QComboBox* comboBox, const std::map<int, std::pair<int, int>>& mapSizes) {
	int selectedIndex = comboBox->currentIndex();

	auto it = mapSizes.find(selectedIndex);
	if (it != mapSizes.end()) {
		return it->second; // Return the width and height pair
	}

	return { 0, 0 };
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

	mapGenOptions.setRoadEnabled(Road::DIRT_ROAD, ui->roadDirt->isChecked());
	mapGenOptions.setRoadEnabled(Road::GRAVEL_ROAD, ui->roadGravel->isChecked());
	mapGenOptions.setRoadEnabled(Road::COBBLESTONE_ROAD, ui->roadCobblestone->isChecked());
	
	if(ui->sizeStandardRadio->isChecked())
	{
		auto size = getSelectedMapSize(ui->sizeCombo, mapSizes);
		mapGenOptions.setWidth(size.first);
		mapGenOptions.setHeight(size.second);
	}
	else
	{
		mapGenOptions.setWidth(ui->widthTxt->value());
		mapGenOptions.setHeight(ui->heightTxt->value());
	}
	
	saveUserSettings();

	std::unique_ptr<CMap> nmap;
	auto & mapController = static_cast<MainWindow *>(parent())->controller;

	if(ui->randomMapCheck->isChecked())
	{
		//verify map template
		if(mapGenOptions.getPossibleTemplates().empty())
		{
			QMessageBox::warning(this, tr("No template"), tr("No template for parameters specified. Random map cannot be generated."));
			return;
		}
		
		hide();

		int seed = std::time(nullptr);
		if(ui->checkSeed->isChecked() && ui->lineSeed->value() != 0)
			seed = ui->lineSeed->value();
			
		CMapGenerator generator(mapGenOptions, mapController.getCallback(), seed);
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
	
	nmap->mods = MapController::modAssessmentMap(*nmap);
	mapController.setMap(std::move(nmap));
	static_cast<MainWindow *>(parent())->initializeMap(true);
	close();
}

void WindowNewMap::on_sizeCombo_activated(int index)
{
	auto size = getSelectedMapSize(ui->sizeCombo, mapSizes);
	mapGenOptions.setWidth(size.first);
	mapGenOptions.setHeight(size.second);
	updateTemplateList();
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

	mapGenOptions.setHumanOrCpuPlayerCount(humans);

	updateTemplateList();
}


void WindowNewMap::on_cpuCombo_activated(int index)
{
	int humans = mapGenOptions.getHumanOrCpuPlayerCount();
	int cpu = ui->cpuCombo->currentData().toInt();

	// FIXME: Use mapGenOption method only to calculate actual number of players for current template
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

	mapGenOptions.setCompOnlyPlayerCount(cpu);

	updateTemplateList();
}


void WindowNewMap::on_randomMapCheck_stateChanged(int arg1)
{
	randomMap = ui->randomMapCheck->isChecked();
	ui->randomOptions->setEnabled(randomMap);
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


void WindowNewMap::on_widthTxt_valueChanged(int value)
{
	if(value > 1)
	{
		mapGenOptions.setWidth(value);
		updateTemplateList();
	}
}


void WindowNewMap::on_heightTxt_valueChanged(int value)
{
	if(value > 1)
	{
		mapGenOptions.setHeight(value);
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

	ui->templateCombo->addItem(tr("[default]"), 0);

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
	int humans = mapGenOptions.getHumanOrCpuPlayerCount();
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



void WindowNewMap::on_sizeStandardRadio_toggled(bool checked) 
{
	if (checked) {
		ui->sizeGroup1->setEnabled(true);
		ui->sizeGroup2->setEnabled(false);
	}
	updateTemplateList();
}


void WindowNewMap::on_sizeCustomRadio_toggled(bool checked) 
{
	if (checked) {
		ui->sizeGroup1->setEnabled(false);
		ui->sizeGroup2->setEnabled(true);
	}
	updateTemplateList();
}
