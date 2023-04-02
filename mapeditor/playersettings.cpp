/*
 * playersettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "playersettings.h"
#include "ui_playersettings.h"
#include "playerparams.h"
#include "mainwindow.h"

PlayerSettings::PlayerSettings(MapController & ctrl, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PlayerSettings),
	controller(ctrl)
{
	ui->setupUi(this);
	show();

	int players = 0;
	const int minAllowedPlayers = 1;
	for(auto & p : controller.map()->players)
	{
		if(p.canAnyonePlay())
		{
			paramWidgets.push_back(new PlayerParams(controller, players));
			ui->playersLayout->addWidget(paramWidgets.back());
			++players;
		}
	}

	if(players < minAllowedPlayers)
		ui->playersCount->setCurrentText("");
	else
		ui->playersCount->setCurrentIndex(players - minAllowedPlayers);
	
	setAttribute(Qt::WA_DeleteOnClose);
}

PlayerSettings::~PlayerSettings()
{
	delete ui;
}

void PlayerSettings::on_playersCount_currentIndexChanged(int index)
{
	const auto selectedPlayerCount = index + 1;
	assert(selectedPlayerCount <= controller.map()->players.size());

	for(int i = 0; i < selectedPlayerCount; ++i)
	{
		if(i < paramWidgets.size())
			continue;

		auto & p = controller.map()->players[i];
		p.canComputerPlay = true;
		paramWidgets.push_back(new PlayerParams(controller, i));
		ui->playersLayout->addWidget(paramWidgets.back());
	}

	assert(!paramWidgets.empty());
	for(int i = paramWidgets.size() - 1; i >= selectedPlayerCount; --i)
	{
		auto & p = controller.map()->players[i];
		p.canComputerPlay = false;
		p.canHumanPlay = false;
		ui->playersLayout->removeWidget(paramWidgets[i]);
		delete paramWidgets[i];
		paramWidgets.pop_back();
	}
}


void PlayerSettings::on_pushButton_clicked()
{
	for(auto * w : paramWidgets)
	{
		controller.map()->players[w->playerColor] = w->playerInfo;
	}

	controller.commitChangeWithoutRedraw();
	close();
}

