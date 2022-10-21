#include "lobby_moc.h"
#include "ui_lobby_moc.h"
#include "../lib/GameConstants.h"

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

void SocketLobby::send(const QString & msg)
{
	socket->write(qPrintable(msg));
}

void SocketLobby::connected()
{
	isConnected = true;
	emit text("Connected!");

	const QString greetingConst = ProtocolStrings[GREETING].arg(username) + ProtocolStrings[VERSION].arg(QString::fromStdString(GameConstants::VCMI_VERSION));
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
	emit text(socket->readAll());
}




Lobby::Lobby(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Lobby)
{
	ui->setupUi(this);

	connect(&socketLobby, SIGNAL(text(QString)), this, SLOT(text(QString)));
}

Lobby::~Lobby()
{
	delete ui;
}

void Lobby::on_messageEdit_returnPressed()
{
	socketLobby.send(ProtocolStrings[MESSAGE].arg(ui->messageEdit->text()));
	ui->messageEdit->clear();
}

void Lobby::text(QString txt)
{
	QTextCursor curs(ui->chat->document());
	curs.movePosition(QTextCursor::End);
	curs.insertText(txt + "\n");
}

void Lobby::on_connectButton_toggled(bool checked)
{
	if(checked)
	{
		socketLobby.connectServer(ui->hostEdit->text(), ui->portEdit->text().toInt(), ui->userEdit->text());
	}
	else
	{
		socketLobby.disconnectServer();
	}
}

