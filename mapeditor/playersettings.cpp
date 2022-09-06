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
	for(auto & p : controller.map()->players)
	{
		if(p.canAnyonePlay())
		{
			paramWidgets.push_back(new PlayerParams(controller, players));
			ui->playersLayout->addWidget(paramWidgets.back());
			++players;
		}
	}

	if(players < 2)
		ui->playersCount->setCurrentText("");
	else
		ui->playersCount->setCurrentIndex(players - 2);
}

PlayerSettings::~PlayerSettings()
{
	delete ui;
}

void PlayerSettings::on_playersCount_currentIndexChanged(int index)
{
	assert(index + 2 <= controller.map()->players.size());

	for(int i = 0; i < index + 2; ++i)
	{
		if(i < paramWidgets.size())
			continue;

		auto & p = controller.map()->players[i];
		p.canComputerPlay = true;
		paramWidgets.push_back(new PlayerParams(controller, i));
		ui->playersLayout->addWidget(paramWidgets.back());
	}

	assert(!paramWidgets.empty());
	for(int i = paramWidgets.size() - 1; i >= index + 2; --i)
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

