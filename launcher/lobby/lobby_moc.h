/*
 * lobby_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QWidget>
#include "lobby.h"

namespace Ui {
class Lobby;
}

class Lobby : public QWidget
{
	Q_OBJECT

	void changeEvent(QEvent *event) override;
public:
	explicit Lobby(QWidget *parent = nullptr);
	~Lobby();
	
signals:
	
	void enableMod(QString mod);
	void disableMod(QString mod);
	
public slots:
	void updateMods();

private slots:
	void on_messageEdit_returnPressed();

	void chatMessage(QString title, QString body, bool isSystem = false);
	void sysMessage(QString body);
	void dispatchMessage(QString);
	void serverCommand(const ServerCommand &);

	void on_connectButton_toggled(bool checked);

	void on_newButton_clicked();

	void on_joinButton_clicked();

	void on_buttonLeave_clicked();

	void on_buttonReady_clicked();

	void onDisconnected();

	void on_sessionsTable_itemSelectionChanged();

	void on_playersList_currentRowChanged(int currentRow);

	void on_kickButton_clicked();

	void on_buttonResolve_clicked();

	void on_optNewGame_toggled(bool checked);

	void on_optLoadGame_toggled(bool checked);

private:
	QString serverUrl;
	int serverPort{};
	bool isLoadGameMode = false;
	
	Ui::Lobby *ui;
	SocketLobby socketLobby;
	QString hostSession;
	QString session;
	QString username;
	QStringList gameArgs;
	QMap<QString, QString> hostModsMap;

	enum AuthStatus
	{
		AUTH_NONE, AUTH_OK, AUTH_ERROR
	};

	AuthStatus authentificationStatus = AUTH_NONE;

private:
	QMap<QString, QString> buildModsMap() const;
	bool isModAvailable(const QString & modName, const QString & modVersion) const;


	void protocolAssert(bool);
};
