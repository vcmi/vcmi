#include "StdInc.h"
#include "lobby_moc.h"
#include "ui_lobby_moc.h"
#include "lobbyroomrequest_moc.h"
#include "../mainwindow_moc.h"
#include "../../lib/CConfigHandler.h"

Lobby::Lobby(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Lobby)
{
	ui->setupUi(this);
	ui->buttonReady->setEnabled(false);

	connect(&socketLobby, SIGNAL(text(QString)), this, SLOT(chatMessage(QString)));
	connect(&socketLobby, SIGNAL(receive(QString)), this, SLOT(dispatchMessage(QString)));
	connect(&socketLobby, SIGNAL(disconnect()), this, SLOT(onDisconnected()));
	
	ui->hostEdit->setText(QString::fromStdString(settings["launcher"]["lobbyUrl"].String()));
	ui->portEdit->setText(QString::number(settings["launcher"]["lobbyPort"].Integer()));
	ui->userEdit->setText(QString::fromStdString(settings["launcher"]["lobbyUsername"].String()));
}

Lobby::~Lobby()
{
	delete ui;
}

void Lobby::serverCommand(const ServerCommand & command) try
{
	//initialize variables outside of switch block
	const QString statusPlaceholder("%1 %2\n");
	QString resText;
	const auto & args = command.arguments;
	int amount, tagPoint;
	QString joinStr;
	switch(command.command)
	{
	case SRVERROR:
		protocolAssert(args.size());
		chatMessage("System error:" + args[0]);
		break;

	case CREATED:
		protocolAssert(args.size());
		hostSession = args[0];
		session = args[0];
		chatMessage("System: new session started");
		ui->buttonReady->setEnabled(true);
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

			QTableWidgetItem * sessionProtectedItem = new QTableWidgetItem(args[tagPoint++]);
			ui->sessionsTable->setItem(i, 2, sessionProtectedItem);
		}
		break;

	case JOINED:
	case KICKED:
		protocolAssert(args.size() == 2);
		joinStr = (command.command == JOINED ? "System: %1 joined to the session %2" : "System: %1 left session %2");

		if(args[1] == username)
		{
			ui->chat->clear(); //cleanup the chat
			chatMessage(joinStr.arg("you", args[0]));
			session = args[0];
			ui->stackedWidget->setCurrentWidget(command.command == JOINED ? ui->roomPage : ui->sessionsPage);
			if(command.command == KICKED)
				ui->buttonReady->setEnabled(false);
		}
		else
		{
			chatMessage(joinStr.arg(args[1], args[0]));
		}
		break;

	case STATUS:
		protocolAssert(args.size() > 0);
		amount = args[0].toInt();
		protocolAssert(amount * 2 == (args.size() - 1));

		tagPoint = 1;
		ui->roomChat->clear();
		resText.clear();
		for(int i = 0; i < amount; ++i, tagPoint += 2)
		{
			resText += statusPlaceholder.arg(args[tagPoint], args[tagPoint + 1] == "True" ? "ready" : "");
		}
		ui->roomChat->setPlainText(resText);
		break;

	case START: {
		protocolAssert(args.size() == 1);
		//actually start game
		gameArgs << "--lobby";
		gameArgs << "--lobby-address" << ui->hostEdit->text();
		gameArgs << "--lobby-port" << ui->portEdit->text();
		gameArgs << "--uuid" << args[0];
		qobject_cast<MainWindow *>(qApp->activeWindow())->startGame(gameArgs);		
		break;
		}

	case HOST: {
		protocolAssert(args.size() == 2);
		gameArgs << "--lobby-host";
		gameArgs << "--lobby-uuid" << args[0];
		gameArgs << "--lobby-connections" << args[1];
		break;
		}

	case CHAT:
		protocolAssert(args.size() > 1);
		QString msg;
		for(int i = 1; i < args.size(); ++i)
			msg += args[i];
		chatMessage(QString("%1: %2").arg(args[0], msg));
		break;
	}
}
catch(const ProtocolError & e)
{
	chatMessage(QString("System error: %1").arg(e.what()));
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
	chatMessage(QString("System error: %1").arg(e.what()));
}

void Lobby::onDisconnected()
{
	ui->stackedWidget->setCurrentWidget(ui->sessionsPage);
	ui->connectButton->setChecked(false);
}

void Lobby::chatMessage(QString txt)
{
	QTextCursor curs(ui->chat->document());
	curs.movePosition(QTextCursor::End);
	curs.insertText(txt + "\n");
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
		username = ui->userEdit->text();

		Settings node = settings.write["launcher"];
		node["lobbyUrl"].String() = ui->hostEdit->text().toStdString();
		node["lobbyPort"].Integer() = ui->portEdit->text().toInt();
		node["lobbyUsername"].String() = username.toStdString();

		socketLobby.connectServer(ui->hostEdit->text(), ui->portEdit->text().toInt(), username);
	}
	else
	{
		socketLobby.disconnectServer();
	}
}

void Lobby::on_newButton_clicked()
{
	new LobbyRoomRequest(socketLobby, "", this);
}

void Lobby::on_joinButton_clicked()
{
	auto * item = ui->sessionsTable->item(ui->sessionsTable->currentRow(), 0);
	if(item)
		new LobbyRoomRequest(socketLobby, item->text(), this);
}


void Lobby::on_buttonLeave_clicked()
{
	socketLobby.requestLeaveSession(session);
}


void Lobby::on_buttonReady_clicked()
{
	socketLobby.requestReadySession(session);
}

