/*
 * lobby.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "lobby.h"
#include "../lib/GameConstants.h"

SocketLobby::SocketLobby(QObject *parent) :
	QObject(parent), socket(new QTcpSocket(this))
{
	connect(socket, SIGNAL(connected()), this, SLOT(connected()));
	connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
	connect(socket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
}

void SocketLobby::connectServer(const QString & host, int port, const QString & usr, int timeout)
{
	username = usr;

	socket->connectToHost(host, port);

	if(!socket->waitForDisconnected(timeout) && !isConnected)
	{
		emit text("Error: " + socket->errorString());
		emit disconnect();
	}
}

void SocketLobby::disconnectServer()
{
	socket->disconnectFromHost();
}

void SocketLobby::requestNewSession(const QString & session, int totalPlayers, const QString & pswd, const QMap<QString, QString> & mods)
{
	const QString sessionMessage = ProtocolStrings[CREATE].arg(session, pswd, QString::number(totalPlayers), prepareModsClientString(mods));
	send(sessionMessage);
}

void SocketLobby::requestJoinSession(const QString & session, const QString & pswd, const QMap<QString, QString> & mods)
{
	const QString sessionMessage = ProtocolStrings[JOIN].arg(session, pswd, prepareModsClientString(mods));
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
	QByteArray str = msg.toUtf8();
	int sz = str.size();
	QByteArray pack((const char *)&sz, sizeof(sz));
	pack.append(str);
	socket->write(pack);
}

void SocketLobby::connected()
{
	isConnected = true;
	emit text("Connected!");

	QByteArray greetingBytes;
	greetingBytes.append(ProtocolVersion);
	greetingBytes.append(ProtocolEncoding.size());
	const QString greetingConst = QString(greetingBytes)
								+ ProtocolStrings[GREETING].arg(QString::fromStdString(ProtocolEncoding),
																username,
																QString::fromStdString(GameConstants::VCMI_VERSION));
	send(greetingConst);
}

void SocketLobby::disconnected()
{
	isConnected = false;
	emit disconnect();
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

QString prepareModsClientString(const QMap<QString, QString> & mods)
{
	QStringList result;
	for(auto & mod : mods.keys())
	{
		result << mod + "&" + mods[mod];
	}
	return result.join(";");
}
