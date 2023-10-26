/*
 * lobby.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QTcpSocket>
#include <QAbstractSocket>

const unsigned int ProtocolVersion = 5;
const std::string ProtocolEncoding = "utf8";

class ProtocolError: public std::runtime_error
{
public:
	ProtocolError(const char * w): std::runtime_error(w) {}
};

enum ProtocolConsts
{
	//client consts
	GREETING, USERNAME, MESSAGE, VERSION, CREATE, JOIN, LEAVE, KICK, READY, FORCESTART, HERE, ALIVE, HOSTMODE, SETCHANNEL,

	//server consts
	SESSIONS, CREATED, JOINED, KICKED, SRVERROR, CHAT, CHATCHANNEL, START, STATUS, HOST, MODS, CLIENTMODS, USERS, HEALTH, GAMEMODE, CHANNEL
};

const QMap<ProtocolConsts, QString> ProtocolStrings
{
	//=== client commands ===
	
	//handshaking with server
	//%1: first byte is protocol_version, then size of encoding string in bytes, then encoding string
	//%2: client name
	//%3: VCMI version
	{GREETING, "%1<GREETINGS>%2<VER>%3"},
	
	//[unsupported] autorization with username
	//%1: username
	{USERNAME, "<USER>%1"},
	
	//sending message to the chat
	//%1: message text
	{MESSAGE, "<MSG>%1"},
	
	//create new room
	//%1: room name
	//%2: password for the room
	//%3: max number of players
	//%4: mods used by host
	// each mod has a format modname&modversion, mods should be separated by ; symbol
	{CREATE, "<NEW>%1<PSWD>%2<COUNT>%3<MODS>%4"},
	
	//join to the room
	//%1: room name
	//%2: password for the room
	//%3: list of mods used by player
	// each mod has a format modname&modversion, mods should be separated by ; symbol
	{JOIN, "<JOIN>%1<PSWD>%2<MODS>%3"},
	
	//leave the room
	//%1: room name
	{LEAVE, "<LEAVE>%1"},
	
	//kick user from the current room
	//%1: player username
	{KICK, "<KICK>%1"},
	
	//signal that player is ready for game
	//%1: room name
	{READY, "<READY>%1"},
	
	//[unsupported] start session immediately
	//%1: room name
	{FORCESTART, "<FORCESTART>%1"},
	
	//request user list
	{HERE, "<HERE>"},
	
	//used as reponse to healcheck
	{ALIVE, "<ALIVE>"},
	
	//host sets game mode (new game or load game)
	//%1: game mode - 0 for new game, 1 for load game
	{HOSTMODE, "<HOSTMODE>%1"},
	
	//set new chat channel
	//%1: channel name
	{SETCHANNEL, "<CHANNEL>%1"},

	//=== server commands ===
	//server commands are started from :>>, arguments are enumerated by : symbol
	
	//new session was created
	//arg[0]: room name
	{CREATED, "CREATED"},
	
	//list of existing sessions
	//arg[0]: amount of sessions, following arguments depending on it
	//arg[x]: session name
	//arg[x+1]: amount of players in the session
	//arg[x+2]: total amount of players allowed
	//arg[x+3]: True if session is protected by password
	{SESSIONS, "SESSIONS"},
	
	//user has joined to the session
	//arg[0]: session name
	//arg[1]: username (who was joined)
	{JOINED, "JOIN"},
	
	//user has left the session
	//arg[0]: session name
	//arg[1]: username (who has left)
	{KICKED, "KICK"},
	
	//session has been started
	//arg[0]: session name
	//arg[1]: uuid to be used for connection
	{START, "START"},
	
	//host ownership for the game session
	//arg[0]: uuid to be used by vcmiserver
	//arg[1]: amount of players (clients) to be connected
	{HOST, "HOST"},
	
	//room status
	//arg[0]: amount of players, following arguments depending on it
	//arg[x]: player username
	//arg[x+1]: True if player is ready
	{STATUS, "STATUS"}, //joined_players:player_name:is_ready
	
	//server error
	//arg[0]: error message
	{SRVERROR, "ERROR"},
	
	//mods used in the session by host player
	//arg[0]: amount of mods, following arguments depending on it
	//arg[x]: mod name
	//arg[x+1]: mod version
	{MODS, "MODS"},
	
	//mods used by user
	//arg[0]: username
	//arg[1]: amount of mods, following arguments depending on it
	//arg[x]: mod name
	//arg[x+1]: mod version
	{CLIENTMODS, "MODSOTHER"},
	
	//received chat message
	//arg[0]: sender username
	//arg[1]: channel
	//arg[2]: message text
	{CHAT, "MSG"},
	
	//received chat message to specific channel
	//arg[0]: sender username
	//arg[1]: channel
	//arg[2]: message text
	{CHATCHANNEL, "MSGCH"},
	
	//list of users currently in lobby
	//arg[0]: amount of players, following arguments depend on it
	//arg[x]: username
	//arg[x+1]: room (empty if not in the room)
	{USERS, "USERS"},
	
	//healthcheck from server
	{HEALTH, "HEALTH"},
	
	//game mode (new game or load game) set by host
	//arg[0]: game mode
	{GAMEMODE, "GAMEMODE"},
	
	//chat channel changed
	//arg[0]: channel name
	{CHANNEL, "CHANNEL"},
};

class ServerCommand
{
public:
	ServerCommand(ProtocolConsts, const QStringList & arguments);

	const ProtocolConsts command;
	const QStringList arguments;
};

class SocketLobby : public QObject
{
	Q_OBJECT
public:
	explicit SocketLobby(QObject *parent = 0);
	void connectServer(const QString & host, int port, const QString & username, int timeout);
	void disconnectServer();
	void requestNewSession(const QString & session, int totalPlayers, const QString & pswd, const QMap<QString, QString> & mods);
	void requestJoinSession(const QString & session, const QString & pswd, const QMap<QString, QString> & mods);
	void requestLeaveSession(const QString & session);
	void requestReadySession(const QString & session);

	void send(const QString &);

signals:

	void text(QString);
	void receive(QString);
	void disconnect();

public slots:

	void connected();
	void disconnected();
	void bytesWritten(qint64 bytes);
	void readyRead();

private:
	QTcpSocket *socket;
	bool isConnected = false;
	QString username;
};

QString prepareModsClientString(const QMap<QString, QString> & mods);
