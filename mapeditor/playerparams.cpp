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
#include "../lib/CTownHandler.h"

PlayerParams::PlayerParams(MapController & ctrl, int playerId, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PlayerParams),
	controller(ctrl)
{
	ui->setupUi(this);

	playerColor = playerId;
	assert(controller.map()->players.size() > playerColor);
	playerInfo = controller.map()->players[playerColor];
	
	//load factions
	for(auto idx : VLC->townh->getAllowedFactions())
	{
		CFaction * faction = VLC->townh->objects.at(idx);
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
		if(auto town = dynamic_cast<CGTownInstance*>(controller.map()->objects[i].get()))
		{
			auto * ctown = town->town;
			if(!ctown)
			{
				ctown = VLC->townh->randomTown;
				town->town = ctown;
			}
			if(ctown && town->getOwner().getNum() == playerColor)
			{
				if(playerInfo.hasMainTown && playerInfo.posOfMainTown == town->pos)
					foundMainTown = townIndex;
				const auto name = ctown->faction ? town->getObjectName() : town->getNameTranslated() + ", (random)";
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


void PlayerParams::on_mainTown_activated(int index)
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

