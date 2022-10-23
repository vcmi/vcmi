#include "lobby_moc.h"
#include "ui_lobby_moc.h"
#include "../lib/GameConstants.h"
#include "../jsonutils.h"
#include "../../lib/CConfigHandler.h"
//#include "../../lib/VCMIDirs.h"

SocketLobby::SocketLobby(QObject *parent) :
	QObject(parent)
{
	socket = new QTcpSocket(this);
	connect(socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(socket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
}

void SocketLobby::connectServer(const QString & host, int port, const QString & usr)
{
	const int connectionTimeout = 1000;
	username = usr;

	emit text("Connecting to " + host + ":" + QString::number(port));

	socket->connectToHost(host, port);

	if(!socket->waitForDisconnected(connectionTimeout) && !isConnected)
	{
		emit text("Error: " + socket->errorString());
	}
}

void SocketLobby::disconnectServer()
{
	socket->disconnectFromHost();
}

void SocketLobby::requestNewSession(const QString & session, int totalPlayers, const QString & pswd)
{
	const QString sessionMessage = ProtocolStrings[CREATE].arg(session, pswd, QString::number(totalPlayers));
	send(sessionMessage);
}

void SocketLobby::requestJoinSession(const QString & session, const QString & pswd)
{
	const QString sessionMessage = ProtocolStrings[JOIN].arg(session, pswd);
	send(sessionMessage);
}

void SocketLobby::requestLeaveSession(const QString & session)
{
	const QString sessionMessage = ProtocolStrings[LEAVE].arg(session);
	send(sessionMessage);
}

void SocketLobby::requestReadySession(const QString & session)
{
	const QString sessionMessage = ProtocolStrings[READY].arg(session);
	send(sessionMessage);
}

void SocketLobby::send(const QString & msg)
{
	socket->write(qPrintable(msg));
}

void SocketLobby::connected()
{
	isConnected = true;
	emit text("Connected!");

	const QString greetingConst = ProtocolStrings[GREETING].arg(username, QString::fromStdString(GameConstants::VCMI_VERSION));
	send(greetingConst);
}

void SocketLobby::disconnected()
{
	isConnected = false;
	emit text("Disconnected!");
}

void SocketLobby::bytesWritten(qint64 bytes)
{
	qDebug() << "We wrote: " << bytes;
}

void SocketLobby::readyRead()
{
	qDebug() << "Reading...";
	emit receive(socket->readAll());
}


ServerCommand::ServerCommand(ProtocolConsts cmd, const QStringList & args):
	command(cmd),
	arguments(args)
{
}

Lobby::Lobby(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Lobby)
{
	ui->setupUi(this);

	connect(&socketLobby, SIGNAL(text(QString)), this, SLOT(chatMessage(QString)));
	connect(&socketLobby, SIGNAL(receive(QString)), this, SLOT(dispatchMessage(QString)));
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
	case ERROR:
		protocolAssert(args.size());
		chatMessage("System error:" + args[0]);
		break;

	case CREATED:
		protocolAssert(args.size());
		hostSession = args[0];
		session = args[0];
		chatMessage("System: new session started");
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

	case START:
		//actually start game
		break;

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
	bool ok;
	QString sessionName = QInputDialog::getText(this, tr("New session"), tr("Session name:"), QLineEdit::Normal, "", &ok);
	if(ok && !sessionName.isEmpty())
		socketLobby.requestNewSession(sessionName, 2, ui->passwordInput->text());
}

void Lobby::on_joinButton_clicked()
{
	auto * item = ui->sessionsTable->item(ui->sessionsTable->currentRow(), 0);
	if(item)
		socketLobby.requestJoinSession(item->text(), ui->passwordInput->text());
}


void Lobby::on_buttonLeave_clicked()
{
	socketLobby.requestLeaveSession(session);
}


void Lobby::on_buttonReady_clicked()
{
	socketLobby.requestReadySession(session);
}

