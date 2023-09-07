/*
 * lobby_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "main.h"
#include "lobby_moc.h"
#include "ui_lobby_moc.h"
#include "lobbyroomrequest_moc.h"
#include "../mainwindow_moc.h"
#include "../modManager/cmodlist.h"
#include "../../lib/CConfigHandler.h"

enum GameMode
{
	NEW_GAME = 0, LOAD_GAME = 1
};

enum ModResolutionRoles
{
	ModNameRole = Qt::UserRole + 1,
	ModEnableRole,
	ModResolvableRole
};

Lobby::Lobby(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Lobby)
{
	ui->setupUi(this);

	connect(&socketLobby, SIGNAL(text(QString)), ui->chatWidget, SLOT(sysMessage(QString)));
	connect(&socketLobby, SIGNAL(receive(QString)), this, SLOT(dispatchMessage(QString)));
	connect(&socketLobby, SIGNAL(disconnect()), this, SLOT(onDisconnected()));
	connect(ui->chatWidget, SIGNAL(messageSent(QString)), this, SLOT(onMessageSent(QString)));
	connect(ui->chatWidget, SIGNAL(channelSwitch(QString)), this, SLOT(onChannelSwitch(QString)));
	
	QString hostString("%1:%2");
	hostString = hostString.arg(QString::fromStdString(settings["launcher"]["lobbyUrl"].String()));
	hostString = hostString.arg(settings["launcher"]["lobbyPort"].Integer());
	
	ui->serverEdit->setText(hostString);
	ui->userEdit->setText(QString::fromStdString(settings["launcher"]["lobbyUsername"].String()));
	ui->kickButton->setVisible(false);
}

void Lobby::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
	}
	QWidget::changeEvent(event);
}

Lobby::~Lobby()
{
	delete ui;
}

QMap<QString, QString> Lobby::buildModsMap() const
{
	QMap<QString, QString> result;
	QObject * mainWindow = qApp->activeWindow();
	if(!mainWindow)
		mainWindow = parent();
	if(!mainWindow)
		return result; //probably something is really wrong here
	
	while(mainWindow->parent())
		mainWindow = mainWindow->parent();
	const auto & modlist = qobject_cast<MainWindow*>(mainWindow)->getModList();
	
	for(auto & modname : modlist.getModList())
	{
		auto mod = modlist.getMod(modname);
		if(mod.isEnabled())
		{
			result[modname] = mod.getValue("version").toString();
		}
	}
	return result;
}

bool Lobby::isModAvailable(const QString & modName, const QString & modVersion) const
{
	QObject * mainWindow = qApp->activeWindow();
	while(mainWindow->parent())
		mainWindow = mainWindow->parent();
	const auto & modlist = qobject_cast<MainWindow*>(mainWindow)->getModList();
	
	if(!modlist.hasMod(modName))
		return false;

	auto mod = modlist.getMod(modName);
	return (mod.isInstalled () || mod.isAvailable()) && (mod.getValue("version") == modVersion);
}

void Lobby::serverCommand(const ServerCommand & command) try
{
	//initialize variables outside of switch block
	const QString statusPlaceholder("%1 %2\n");
	const auto & args = command.arguments;
	int amount, tagPoint;
	QString joinStr;
	switch(command.command)
	{
	case SRVERROR:
		protocolAssert(args.size());
		ui->chatWidget->chatMessage("System error", args[0], true);
		if(authentificationStatus == AuthStatus::AUTH_NONE)
			authentificationStatus = AuthStatus::AUTH_ERROR;
		break;

	case CREATED:
		protocolAssert(args.size());
		hostSession = args[0];
		session = args[0];
		ui->chatWidget->setSession(session);
		break;

	case SESSIONS:
		protocolAssert(args.size());
		amount = args[0].toInt();
		protocolAssert(amount * 4 == (args.size() - 1));
		ui->sessionsTable->setRowCount(amount);

		tagPoint = 1;
		for(int i = 0; i < amount; ++i)
		{
			QTableWidgetItem * sessionNameItem = new QTableWidgetItem(args[tagPoint++]);
			ui->sessionsTable->setItem(i, 0, sessionNameItem);

			int playersJoined = args[tagPoint++].toInt();
			int playersTotal = args[tagPoint++].toInt();
			QTableWidgetItem * sessionPlayerItem = new QTableWidgetItem(QString("%1/%2").arg(playersJoined).arg(playersTotal));
			ui->sessionsTable->setItem(i, 1, sessionPlayerItem);

			QTableWidgetItem * sessionProtectedItem = new QTableWidgetItem();
			bool isPrivate = (args[tagPoint++] == "True");
			sessionProtectedItem->setData(Qt::UserRole, isPrivate);
			if(isPrivate)
				sessionProtectedItem->setIcon(QIcon("icons:room-private.png"));
			ui->sessionsTable->setItem(i, 2, sessionProtectedItem);
		}
		break;

	case JOINED:
	case KICKED:
		protocolAssert(args.size() == 2);
		session = "";
		ui->chatWidget->setSession(session);
		if(args[1] == username)
		{
			hostModsMap.clear();
			ui->buttonReady->setText("Ready");
			ui->optNewGame->setChecked(true);
			session = args[0];
			ui->chatWidget->setSession(session);
			bool isHost = command.command == JOINED && hostSession == session;
			ui->optNewGame->setEnabled(isHost);
			ui->optLoadGame->setEnabled(isHost);
			ui->stackedWidget->setCurrentWidget(command.command == JOINED ? ui->roomPage : ui->sessionsPage);
		}
		else
		{
			joinStr = (command.command == JOINED ? "%1 joined to the session %2" : "%1 left session %2");
			ui->chatWidget->sysMessage(joinStr.arg(args[1], args[0]));
		}
		break;

	case MODS: {
		protocolAssert(args.size() > 0);
		amount = args[0].toInt();
		protocolAssert(amount * 2 == (args.size() - 1));

		tagPoint = 1;
		for(int i = 0; i < amount; ++i, tagPoint += 2)
			hostModsMap[args[tagPoint]] = args[tagPoint + 1];
		
		updateMods();
		break;
		}
			
	case CLIENTMODS: {
		protocolAssert(args.size() >= 1);
		amount = args[1].toInt();
		protocolAssert(amount * 2 == (args.size() - 2));

		tagPoint = 2;
		break;
		}


	case STATUS:
		protocolAssert(args.size() > 0);
		amount = args[0].toInt();
		protocolAssert(amount * 2 == (args.size() - 1));

		tagPoint = 1;
		ui->playersList->clear();
		for(int i = 0; i < amount; ++i, tagPoint += 2)
		{
			if(args[tagPoint + 1] == "True")
				ui->playersList->addItem(new QListWidgetItem(QIcon("icons:mod-enabled.png"), args[tagPoint]));
			else
				ui->playersList->addItem(new QListWidgetItem(QIcon("icons:mod-disabled.png"), args[tagPoint]));
			
			if(args[tagPoint] == username)
			{
				if(args[tagPoint + 1] == "True")
					ui->buttonReady->setText("Not ready");
				else
					ui->buttonReady->setText("Ready");
			}
		}
		break;

	case START: {
		protocolAssert(args.size() == 1);
		//actually start game
		gameArgs << "--lobby";
		gameArgs << "--lobby-address" << serverUrl;
		gameArgs << "--lobby-port" << QString::number(serverPort);
		gameArgs << "--lobby-username" << username;
		gameArgs << "--lobby-gamemode" << QString::number(isLoadGameMode);
		gameArgs << "--uuid" << args[0];
		startGame(gameArgs);		
		break;
		}

	case HOST: {
		protocolAssert(args.size() == 2);
		gameArgs << "--lobby-host";
		gameArgs << "--lobby-uuid" << args[0];
		gameArgs << "--lobby-connections" << args[1];
		break;
		}

	case CHAT: {
		protocolAssert(args.size() > 1);
		QString msg;
		for(int i = 1; i < args.size(); ++i)
			msg += args[i];
		ui->chatWidget->chatMessage(args[0], msg);
		break;
		}
			
	case CHATCHANNEL: {
		protocolAssert(args.size() > 2);
		QString msg;
		for(int i = 2; i < args.size(); ++i)
			msg += args[i];
		ui->chatWidget->chatMessage(args[0], args[1], msg);
		break;
		}
			
	case CHANNEL: {
		protocolAssert(args.size() == 1);
		ui->chatWidget->setChannel(args[0]);
		break;
		}
			
	case HEALTH: {
		socketLobby.send(ProtocolStrings[ALIVE]);
		break;
	}
			
	case USERS: {
		protocolAssert(args.size() > 0);
		amount = args[0].toInt();
		
		protocolAssert(amount == (args.size() - 1));
		ui->chatWidget->clearUsers();
		for(int i = 0; i < amount; ++i)
		{
			ui->chatWidget->addUser(args[i + 1]);
		}
		break;
	}
			
	case GAMEMODE: {
		protocolAssert(args.size() == 1);
		isLoadGameMode = args[0].toInt();
		if(isLoadGameMode)
			ui->optLoadGame->setChecked(true);
		else
			ui->optNewGame->setChecked(true);
		break;
	}

	default:
			ui->chatWidget->sysMessage("Unknown server command");
	}

	if(authentificationStatus == AuthStatus::AUTH_ERROR)
	{
		socketLobby.disconnectServer();
	}
	else
	{
		authentificationStatus = AuthStatus::AUTH_OK;
		ui->newButton->setEnabled(true);
	}
}
catch(const ProtocolError & e)
{
	ui->chatWidget->chatMessage("System error", e.what(), true);
}

void Lobby::dispatchMessage(QString txt) try
{
	if(txt.isEmpty())
		return;

	QStringList parseTags = txt.split(":>>");
	protocolAssert(parseTags.size() > 1 && parseTags[0].isEmpty() && !parseTags[1].isEmpty());

	for(int c = 1; c < parseTags.size(); ++c)
	{
		QStringList parseArgs = parseTags[c].split(":");
		protocolAssert(parseArgs.size() > 1);

		auto ctype = ProtocolStrings.key(parseArgs[0]);
		parseArgs.pop_front();
		ServerCommand cmd(ctype, parseArgs);
		serverCommand(cmd);
	}
}
catch(const ProtocolError & e)
{
	ui->chatWidget->chatMessage("System error", e.what(), true);
}

void Lobby::onDisconnected()
{
	authentificationStatus = AuthStatus::AUTH_NONE;
	session = "";
	ui->chatWidget->setSession(session);
	ui->stackedWidget->setCurrentWidget(ui->sessionsPage);
	ui->connectButton->setChecked(false);
	ui->serverEdit->setEnabled(true);
	ui->userEdit->setEnabled(true);
	ui->newButton->setEnabled(false);
	ui->joinButton->setEnabled(false);
	ui->sessionsTable->setRowCount(0);
}

void Lobby::protocolAssert(bool expr)
{
	if(!expr)
		throw ProtocolError("Protocol error");
}

void Lobby::on_connectButton_toggled(bool checked)
{
	if(checked)
	{
		ui->connectButton->setText(tr("Disconnect"));
		authentificationStatus = AuthStatus::AUTH_NONE;
		username = ui->userEdit->text();
		ui->chatWidget->setUsername(username);
		const int connectionTimeout = settings["launcher"]["connectionTimeout"].Integer();
		
		auto serverStrings = ui->serverEdit->text().split(":");
		if(serverStrings.size() != 2)
		{
			QMessageBox::critical(this, "Connection error", "Server address must have the format URL:port");
			return;
		}
		
		serverUrl = serverStrings[0];
		serverPort = serverStrings[1].toInt();

		Settings node = settings.write["launcher"];
		node["lobbyUrl"].String() = serverUrl.toStdString();
		node["lobbyPort"].Integer() = serverPort;
		node["lobbyUsername"].String() = username.toStdString();
		
		ui->serverEdit->setEnabled(false);
		ui->userEdit->setEnabled(false);

		ui->chatWidget->sysMessage("Connecting to " + serverUrl + ":" + QString::number(serverPort));
		//show text immediately
		ui->chatWidget->repaint();
		qApp->processEvents();
		
		socketLobby.connectServer(serverUrl, serverPort, username, connectionTimeout);
	}
	else
	{
		ui->connectButton->setText(tr("Connect"));
		ui->serverEdit->setEnabled(true);
		ui->userEdit->setEnabled(true);
		ui->chatWidget->clearUsers();
		hostModsMap.clear();
		updateMods();
		socketLobby.disconnectServer();
	}
}

void Lobby::updateMods()
{
	ui->modsList->clear();
	if(hostModsMap.empty())
		return;
	
	auto createModListWidget = [](const QIcon & icon, const QString & label, const QString & name, bool enableFlag, bool resolveFlag)
	{
		auto * lw = new QListWidgetItem(icon, label);
		lw->setData(ModResolutionRoles::ModNameRole, name);
		lw->setData(ModResolutionRoles::ModEnableRole, enableFlag);
		lw->setData(ModResolutionRoles::ModResolvableRole, resolveFlag);
		return lw;
	};
	
	auto enabledMods = buildModsMap();
	for(const auto & mod : hostModsMap.keys())
	{
		auto & modValue = hostModsMap[mod];
		auto modName = QString("%1 (v%2)").arg(mod, modValue);
		if(enabledMods.contains(mod))
		{
			if(enabledMods[mod] == modValue)
				enabledMods.remove(mod); //mod fully matches, remove from list
			else
			{
				//mod version mismatch
				ui->modsList->addItem(createModListWidget(QIcon("icons:mod-update.png"), modName, mod, true, false));
			}
		}
		else if(isModAvailable(mod, modValue))
		{
			//mod is available and needs to be enabled
			ui->modsList->addItem(createModListWidget(QIcon("icons:mod-enabled.png"), modName, mod, true, true));
		}
		else
		{
			//mod is not available and needs to be installed
			ui->modsList->addItem(createModListWidget(QIcon("icons:mod-delete.png"), modName, mod, true, false));
		}
	}
	for(const auto & remainMod : enabledMods.keys())
	{
		auto modName = QString("%1 (v%2)").arg(remainMod, enabledMods[remainMod]);
		//mod needs to be disabled
		ui->modsList->addItem(createModListWidget(QIcon("icons:mod-disabled.png"), modName, remainMod, false, true));
	}
	if(!ui->modsList->count())
	{
		ui->buttonResolve->setEnabled(false);
		ui->modsList->addItem(tr("No issues detected"));
	}
	else
	{
		ui->buttonResolve->setEnabled(true);
	}
}

void Lobby::on_newButton_clicked()
{
	new LobbyRoomRequest(socketLobby, "", buildModsMap(), this);
}

void Lobby::on_joinButton_clicked()
{
	auto * item = ui->sessionsTable->item(ui->sessionsTable->currentRow(), 0);
	if(item)
	{
		auto isPrivate = ui->sessionsTable->item(ui->sessionsTable->currentRow(), 2)->data(Qt::UserRole).toBool();
		if(isPrivate)
			new LobbyRoomRequest(socketLobby, item->text(), buildModsMap(), this);
		else
			socketLobby.requestJoinSession(item->text(), "", buildModsMap());
	}
}

void Lobby::on_buttonLeave_clicked()
{
	socketLobby.requestLeaveSession(session);
}

void Lobby::on_buttonReady_clicked()
{
	if(ui->buttonReady->text() == "Ready")
		ui->buttonReady->setText("Not ready");
	else
		ui->buttonReady->setText("Ready");
	socketLobby.requestReadySession(session);
}

void Lobby::on_sessionsTable_itemSelectionChanged()
{
	auto selection = ui->sessionsTable->selectedItems();
	ui->joinButton->setEnabled(!selection.empty());
}

void Lobby::on_playersList_currentRowChanged(int currentRow)
{
	ui->kickButton->setVisible(ui->playersList->currentItem()
							   && currentRow > 0
							   && ui->playersList->currentItem()->text() != username);
}

void Lobby::on_kickButton_clicked()
{
	if(ui->playersList->currentItem() && ui->playersList->currentItem()->text() != username)
		socketLobby.send(ProtocolStrings[KICK].arg(ui->playersList->currentItem()->text()));
}


void Lobby::on_buttonResolve_clicked()
{
	QStringList toEnableList, toDisableList;
	for(auto * item : ui->modsList->selectedItems())
	{
		auto modName = item->data(ModResolutionRoles::ModNameRole);
		if(modName.isNull())
			continue;
		
		bool modToEnable = item->data(ModResolutionRoles::ModEnableRole).toBool();
		bool modToResolve = item->data(ModResolutionRoles::ModResolvableRole).toBool();
		
		if(!modToResolve)
			continue;
		
		if(modToEnable)
			toEnableList << modName.toString();
		else
			toDisableList << modName.toString();
	}
	
	//disabling first, then enabling
	for(auto & mod : toDisableList)
		emit disableMod(mod);
	for(auto & mod : toEnableList)
		emit enableMod(mod);
}

void Lobby::on_optNewGame_toggled(bool checked)
{
	if(checked)
	{
		if(isLoadGameMode)
			socketLobby.send(ProtocolStrings[HOSTMODE].arg(GameMode::NEW_GAME));
	}
}

void Lobby::on_optLoadGame_toggled(bool checked)
{
	if(checked)
	{
		if(!isLoadGameMode)
			socketLobby.send(ProtocolStrings[HOSTMODE].arg(GameMode::LOAD_GAME));
	}
}

void Lobby::onMessageSent(QString message)
{
	socketLobby.send(ProtocolStrings[MESSAGE].arg(message));
}

void Lobby::onChannelSwitch(QString channel)
{
	socketLobby.send(ProtocolStrings[SETCHANNEL].arg(channel));
}
