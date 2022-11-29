/*
 * lobbyroomrequest_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#ifndef LOBBYROOMREQUEST_MOC_H
#define LOBBYROOMREQUEST_MOC_H

#include <QDialog>
#include "lobby.h"

namespace Ui {
class LobbyRoomRequest;
}

class LobbyRoomRequest : public QDialog
{
	Q_OBJECT

public:
	explicit LobbyRoomRequest(SocketLobby & socket, const QString & room, const QMap<QString, QString> & mods, QWidget *parent = nullptr);
	~LobbyRoomRequest();

private slots:
	void on_buttonBox_accepted();

private:
	Ui::LobbyRoomRequest *ui;
	SocketLobby & socketLobby;
	QMap<QString, QString> mods;
};

#endif // LOBBYROOMREQUEST_MOC_H
