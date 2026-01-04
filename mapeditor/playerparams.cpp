/*
 * playerparams.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "playerparams.h"
#include "ui_playerparams.h"
#include "mapsettings/abstractsettings.h"
#include "../lib/constants/StringConstants.h"
#include "../lib/entities/faction/CTownHandler.h"
#include "../lib/mapping/CMap.h"

PlayerParams::PlayerParams(MapController & ctrl, int playerId, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PlayerParams),
	controller(ctrl)
{
	ui->setupUi(this);
	
	//set colors and teams
	ui->teamId->addItem(tr("No team"), QVariant(TeamID::NO_TEAM));
	for(int i = 0, index = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		if(i == playerId || !controller.map()->players[i].canAnyonePlay())
		{
			MetaString str;
			str.appendName(PlayerColor(i));
			ui->playerColorCombo->addItem(QString::fromStdString(str.toString()), QVariant(i));
			if(i == playerId)
				ui->playerColorCombo->setCurrentIndex(index);
			++index;
		}
		
		//add teams
		ui->teamId->addItem(QString::number(i + 1), QVariant(i));
	}

	playerColor = playerId;
	assert(controller.map()->players.size() > playerColor);
	playerInfo = controller.map()->players[playerColor];
	ui->teamId->setCurrentIndex(playerInfo.team == TeamID::NO_TEAM ? 0 : playerInfo.team.getNum() + 1);
	
	//load factions
	for(auto idx : LIBRARY->townh->getAllowedFactions())
	{
		const auto & faction = LIBRARY->townh->objects.at(idx);
		auto * item = new QListWidgetItem(QString::fromStdString(faction->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(idx.getNum()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		ui->allowedFactions->addItem(item);
		if(playerInfo.allowedFactions.count(idx))
			item->setCheckState(Qt::Checked);
		else
			item->setCheckState(Qt::Unchecked);
	}
	QObject::connect(ui->allowedFactions, SIGNAL(itemChanged(QListWidgetItem*)),
					 this, SLOT(allowedFactionsCheck(QListWidgetItem*)));

	//load checks
	bool canHumanPlay = playerInfo.canHumanPlay; //need variable to restore after signal received
	playerInfo.canComputerPlay = true; //computer always can play
	ui->radioCpu->setChecked(true);
	if(canHumanPlay)
		ui->radioHuman->setChecked(true);
	if(playerInfo.isFactionRandom)
		ui->randomFaction->setChecked(true);
	if(playerInfo.generateHeroAtMainTown)
		ui->generateHero->setChecked(true);

	//load towns
	int foundMainTown = -1;
	for(int i = 0, townIndex = 0; i < controller.map()->objects.size(); ++i)
	{
		if(auto town = dynamic_cast<CGTownInstance*>(controller.map()->objects.at(i).get()))
		{
			auto * ctown = town->getTown();

			if(ctown && town->getOwner().getNum() == playerColor)
			{
				if(playerInfo.hasMainTown && playerInfo.posOfMainTown == town->pos)
					foundMainTown = townIndex;
				
				const auto name = AbstractSettings::getTownName(*controller.map(), i);
				ui->mainTown->addItem(tr(name.c_str()), QVariant::fromValue(i));
				++townIndex;
			}
		}
	}
	
	if(foundMainTown > -1)
	{
		ui->mainTown->setCurrentIndex(foundMainTown + 1);
	}
	else
	{
		ui->generateHero->setEnabled(false);
		playerInfo.hasMainTown = false;
		playerInfo.generateHeroAtMainTown = false;
		playerInfo.posOfMainTown = int3(-1, -1, -1);
	}

	ui->playerColor->setTitle(tr("Player ID: %1").arg(playerId));
	show();
}

PlayerParams::~PlayerParams()
{
	delete ui;
}

void PlayerParams::on_radioHuman_toggled(bool checked)
{
	if(checked)
		playerInfo.canHumanPlay = true;
}


void PlayerParams::on_radioCpu_toggled(bool checked)
{
	if(checked)
		playerInfo.canHumanPlay = false;
}


void PlayerParams::on_generateHero_stateChanged(int arg1)
{
	playerInfo.generateHeroAtMainTown = ui->generateHero->isChecked();
}


void PlayerParams::on_randomFaction_stateChanged(int arg1)
{
	playerInfo.isFactionRandom = ui->randomFaction->isChecked();
}


void PlayerParams::allowedFactionsCheck(QListWidgetItem * item)
{
	if(item->checkState() == Qt::Checked)
		playerInfo.allowedFactions.insert(FactionID(item->data(Qt::UserRole).toInt()));
	else
		playerInfo.allowedFactions.erase(FactionID(item->data(Qt::UserRole).toInt()));
}

void PlayerParams::on_mainTown_currentIndexChanged(int index)
{
	if(index == 0) //default
	{
		ui->generateHero->setEnabled(false);
		ui->generateHero->setChecked(false);
		playerInfo.hasMainTown = false;
		playerInfo.posOfMainTown = int3(-1, -1, -1);
	}
	else
	{
		ui->generateHero->setEnabled(true);
		auto town = controller.map()->objects.at(ui->mainTown->currentData().toInt());
		playerInfo.hasMainTown = true;
		playerInfo.posOfMainTown = town->pos;
	}
}


void PlayerParams::on_teamId_activated(int index)
{
	playerInfo.team.setNum(ui->teamId->currentData().toInt());
}


void PlayerParams::on_playerColorCombo_activated(int index)
{
	int data = ui->playerColorCombo->currentData().toInt();
	if(data != playerColor)
	{
		controller.map()->players[playerColor].canHumanPlay = false;
		controller.map()->players[playerColor].canComputerPlay = false;
		playerColor = data;
	}
}


void PlayerParams::on_townSelect_clicked()
{
	auto pred = [this](const CGObjectInstance * obj) -> bool
	{
		if(auto town = dynamic_cast<const CGTownInstance*>(obj))
			return town->getOwner().getNum() == playerColor;
		return false;
	};
	
	for(MapScene * level : controller.getScenes())
	{
		auto & l = level->objectPickerView;
		l.highlight(pred);
		l.update();
		QObject::connect(&l, &ObjectPickerLayer::selectionMade, this, &PlayerParams::onTownPicked);
	}
	
	controller.settingsDialog->hide();
}

void PlayerParams::onTownPicked(const CGObjectInstance * obj)
{
	controller.settingsDialog->show();
	
	for(MapScene * level : controller.getScenes())
	{
		auto & l = level->objectPickerView;
		l.clear();
		l.update();
		QObject::disconnect(&l, &ObjectPickerLayer::selectionMade, this, &PlayerParams::onTownPicked);
	}
	
	if(!obj) //discarded
		return;
	
	for(int i = 0; i < ui->mainTown->count(); ++i)
	{
		auto town = controller.map()->objects.at(ui->mainTown->itemData(i).toInt());
		if(town.get() == obj)
		{
			ui->mainTown->setCurrentIndex(i);
			break;
		}
	}
}

