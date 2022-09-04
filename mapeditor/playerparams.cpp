#include "StdInc.h"
#include "playerparams.h"
#include "ui_playerparams.h"

PlayerParams::PlayerParams(const CMapHeader & mapHeader, int playerId, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PlayerParams)
{
	ui->setupUi(this);

	playerColor = playerId;
	assert(mapHeader.players.size() > playerColor);
	playerInfo = mapHeader.players[playerColor];

	if(playerInfo.canComputerPlay)
		ui->radioCpu->setChecked(true);
	if(playerInfo.canHumanPlay)
		ui->radioHuman->setChecked(true);

	ui->playerColor->setTitle(QString("PlayerID: %1").arg(playerId));
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

