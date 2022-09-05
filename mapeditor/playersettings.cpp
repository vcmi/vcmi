#include "StdInc.h"
#include "playersettings.h"
#include "ui_playersettings.h"
#include "playerparams.h"
#include "mainwindow.h"

PlayerSettings::PlayerSettings(CMapHeader & mapHeader, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PlayerSettings),
	header(mapHeader)
{
	ui->setupUi(this);
	show();

	int players = 0;
	for(auto & p : header.players)
	{
		if(p.canAnyonePlay())
		{
			paramWidgets.push_back(new PlayerParams(header, players));
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
	assert(index + 2 <= header.players.size());

	for(int i = 0; i < index + 2; ++i)
	{
		if(i < paramWidgets.size())
			continue;

		auto & p = header.players[i];
		p.canComputerPlay = true;
		paramWidgets.push_back(new PlayerParams(header, i));
		ui->playersLayout->addWidget(paramWidgets.back());
	}

	assert(!paramWidgets.empty());
	for(int i = paramWidgets.size() - 1; i >= index + 2; --i)
	{
		auto & p = header.players[i];
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
		header.players[w->playerColor] = w->playerInfo;
	}

	static_cast<MainWindow*>(parent())->controller.commitChangeWithoutRedraw();
	close();
}

