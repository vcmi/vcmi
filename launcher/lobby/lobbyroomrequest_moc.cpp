/*
 * lobbyroomrequest_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "lobbyroomrequest_moc.h"
#include "ui_lobbyroomrequest_moc.h"

LobbyRoomRequest::LobbyRoomRequest(SocketLobby & socket, const QString & room, const QMap<QString, QString> & mods, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::LobbyRoomRequest),
	socketLobby(socket),
	mods(mods)
{
	ui->setupUi(this);
	ui->nameEdit->setText(room);
	if(!room.isEmpty())
	{
		ui->nameEdit->setReadOnly(true);
		ui->totalPlayers->setEnabled(false);
	}

	show();
}

void LobbyRoomRequest::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
	}
	QDialog::changeEvent(event);
}

LobbyRoomRequest::~LobbyRoomRequest()
{
	delete ui;
}

void LobbyRoomRequest::on_buttonBox_accepted()
{
	if(ui->nameEdit->isReadOnly())
	{
		socketLobby.requestJoinSession(ui->nameEdit->text(), ui->passwordEdit->text(), mods);
	}
	else
	{
		if(!ui->nameEdit->text().isEmpty())
		{
			int totalPlayers = ui->totalPlayers->currentIndex() + 2; //where 2 is a minimum amount of players
			socketLobby.requestNewSession(ui->nameEdit->text(), totalPlayers, ui->passwordEdit->text(), mods);
		}
	}
}

