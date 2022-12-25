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

Lobby::Lobby(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Lobby)
{
	ui->setupUi(this);

	connect(&socketLobby, SIGNAL(text(QString)), this, SLOT(sysMessage(QString)));
	connect(&socketLobby, SIGNAL(receive(QString)), this, SLOT(dispatchMessage(QString)));
	connect(&socketLobby, SIGNAL(disconnect()), this, SLOT(onDisconnected()));
	
	QString hostString("%1:%2");
	hostString = hostString.arg(QString::fromStdString(settings["launcher"]["lobbyUrl"].String()));
	hostString = hostString.arg(settings["launcher"]["lobbyPort"].Integer());
	
	ui->serverEdit->setText(hostString);
	ui->userEdit->setText(QString::fromStdString(settings["launcher"]["lobbyUsername"].String()));
	ui->kickButton->setVisible(false);
}

void Lobby::changeEvent(QEvent *event)
{
	if ( event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
	}
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
		chatMessage("System error", args[0], true);
		if(authentificationStatus == AuthStatus::AUTH_NONE)
			authentificationStatus = AuthStatus::AUTH_ERROR;
		break;

	case CREATED:
		protocolAssert(args.size());
		hostSession = args[0];
		session = args[0];
		sysMessage("new session started");
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
		joinStr = (command.command == JOINED ? "%1 joined to the session %2" : "%1 left session %2");

		if(args[1] == username)
		{
			ui->buttonReady->setText("Ready");
			ui->chat->clear(); //cleanup the chat
			sysMessage(joinStr.arg("you", args[0]));
			session = args[0];
			ui->stackedWidget->setCurrentWidget(command.command == JOINED ? ui->roomPage : ui->sessionsPage);
		}
		else
		{
			sysMessage(joinStr.arg(args[1], args[0]));
		}
		break;

	case MODS: {
		protocolAssert(args.size() > 0);
		amount = args[0].toInt();
		protocolAssert(amount * 2 == (args.size() - 1));

		tagPoint = 1;
		ui->modsList->clear();
		auto enabledMods = buildModsMap();
		for(int i = 0; i < amount; ++i, tagPoint += 2)
		{
			if(enabledMods.contains(args[tagPoint]))
			{
				if(enabledMods[args[tagPoint]] == args[tagPoint + 1])
					enabledMods.remove(args[tagPoint]);
				else
					ui->modsList->addItem(new QListWidgetItem(QIcon("icons:mod-update.png"), QString("%1 (v%2)").arg(args[tagPoint], args[tagPoint + 1])));
			}
			else if(isModAvailable(args[tagPoint], args[tagPoint + 1]))
				ui->modsList->addItem(new QListWidgetItem(QIcon("icons:mod-enabled.png"), QString("%1 (v%2)").arg(args[tagPoint], args[tagPoint + 1])));
			else
				ui->modsList->addItem(new QListWidgetItem(QIcon("icons:mod-delete.png"), QString("%1 (v%2)").arg(args[tagPoint], args[tagPoint + 1])));
		}
		for(auto & remainMod : enabledMods.keys())
		{
			ui->modsList->addItem(new QListWidgetItem(QIcon("icons:mod-disabled.png"), QString("%1 (v%2)").arg(remainMod, enabledMods[remainMod])));
		}
		if(!ui->modsList->count())
			ui->modsList->addItem("No issues detected");
		break;
		}
			
	case CLIENTMODS: {
		protocolAssert(args.size() > 1);
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
				if(args[tagPoint + 1] == "True")
					ui->buttonReady->setText("Not ready");
				else
					ui->buttonReady->setText("Ready");
		}
		break;

	case START: {
		protocolAssert(args.size() == 1);
		//actually start game
		gameArgs << "--lobby";
		gameArgs << "--lobby-address" << serverUrl;
		gameArgs << "--lobby-port" << QString::number(serverPort);
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
		chatMessage(args[0], msg);
		break;
		}

	default:
		sysMessage("Unknown server command");
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
	chatMessage("System error", e.what(), true);
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
	chatMessage("System error", e.what(), true);
}

void Lobby::onDisconnected()
{
	authentificationStatus = AuthStatus::AUTH_NONE;
	ui->stackedWidget->setCurrentWidget(ui->sessionsPage);
	ui->connectButton->setChecked(false);
	ui->serverEdit->setEnabled(true);
	ui->userEdit->setEnabled(true);
	ui->newButton->setEnabled(false);
	ui->joinButton->setEnabled(false);
	ui->sessionsTable->clear();
}

void Lobby::chatMessage(QString title, QString body, bool isSystem)
{
	QTextCharFormat fmtBody, fmtTitle;
	fmtTitle.setFontWeight(QFont::Bold);
	if(isSystem)
		fmtBody.setFontWeight(QFont::DemiBold);
	
	QTextCursor curs(ui->chat->document());
	curs.movePosition(QTextCursor::End);
	curs.insertText(title + ": ", fmtTitle);
	curs.insertText(body + "\n", fmtBody);
	ui->chat->ensureCursorVisible();
}

void Lobby::sysMessage(QString body)
{
	chatMessage("System", body, true);
}

void Lobby::protocolAssert(bool expr)
{
	if(!expr)
		throw ProtocolError("Protocol error");
}

void Lobby::on_messageEdit_returnPressed()
{
	socketLobby.send(ProtocolStrings[MESSAGE].arg(ui->messageEdit->text()));
	ui->messageEdit->clear();
}

void Lobby::on_connectButton_toggled(bool checked)
{
	if(checked)
	{
		authentificationStatus = AuthStatus::AUTH_NONE;
		username = ui->userEdit->text();
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

		sysMessage("Connecting to " + serverUrl + ":" + QString::number(serverPort));
		//show text immediately
		ui->chat->repaint();
		qApp->processEvents();
		
		socketLobby.connectServer(serverUrl, serverPort, username, connectionTimeout);
	}
	else
	{
		ui->serverEdit->setEnabled(true);
		ui->userEdit->setEnabled(true);
		socketLobby.disconnectServer();
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

