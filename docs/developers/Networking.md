# Networking

## The big picture

For implementation details see files located at `lib/network` directory.

VCMI uses connection using TCP to communicate with server, even in single-player games. However, even though TCP is stream-based protocol, VCMI uses atomic messages for communication. Each message is a serialized stream of bytes, preceded by 4-byte message size:
```
int32_t messageSize;
byte messagePayload[messageSize];
```

Networking can be used by:
- game client (vcmiclient / VCMI_Client.exe). Actual application that player interacts with directly using UI.
- match server (vcmiserver / VCMI_Server.exe / part of game client). This app controls game logic and coordinates multiplayer games.
- lobby server (vcmilobby). This app provides access to global lobby through which players can play game over Internet.

Following connections can be established during game lifetime:

- game client -> match server: This is main connection for use during gameplay, created once player requests to move from main menu to pregame / match lobby (e.g. after pressing New Game / Load Game)
- game client -> lobby server: This connection is used to access global lobby, for multiplayer over Internet. Created when player logs into a lobby (Multiplayer -> Connect to global service)
- match server -> lobby server: This connection is established when player creates new multiplayer room via lobby. It is used by lobby server to send commands to match server

## Gameplay communication

For gameplay, VCMI serializes data into a binary stream. See [Serialization](Serialization.md) for more information.

## Global lobby communication

For implementation details see:
- game client: `client/globalLobby/GlobalLobbyClient.h
- match server: `server/GlobalLobbyProcessor.h
- lobby server: `client/globalLobby/GlobalLobbyClient.h

In case of global lobby, message payload uses plaintext json format - utf-8 encoded string:
```
int32_t messageSize;
char jsonString[messageSize];
```

Every message must be a struct (json object) that contains "type" field. Unlike rest of VCMI codebase, this message is validated as strict json, without any extensions, such as comments.

### Communication flow

Notes:
- invalid message, such as corrupted json format or failure to validate message will result in no reply from server
- in addition to specified messages, match server will send `operationFailed` message on failure to apply player request

#### New Account Creation

- client -> lobby: `clientRegister`
- lobby -> client: `accountCreated`

#### Login 
- client -> lobby: `clientLogin`
- lobby -> client: `loginSuccess`
- lobby -> client: `chatHistory`
- lobby -> client: `activeAccounts`
- lobby -> client: `activeGameRooms`

#### Chat Message
- client -> lobby: `sendChatMessage`
- lobby -> every client: `chatMessage`

#### New Game Room
- client starts match server instance
- match -> lobby: `serverLogin`
- lobby -> match: `loginSuccess`
- match accepts connection from client
- client -> lobby: `activateGameRoom`
- lobby -> client: `joinRoomSuccess`
- lobby -> every client: `activeAccounts`
- lobby -> every client: `activeGameRooms`

#### Joining a game room
See [#Proxy mode](proxy-mode)

#### Leaving a game room
- client closes connection to match server
- match -> lobby: `leaveGameRoom`

#### Sending an invite:
- client -> lobby: `sendInvite`
- lobby -> target client: `inviteReceived`

Note: there is no dedicated procedure to accept an invite. Instead, invited player will use same flow as when joining public game room

#### Logout
- client closes connection
- lobby -> every client: `activeAccounts`

### Proxy mode

In order to connect players located behind NAT, VCMI lobby can operate in "proxy" mode. In this mode, connection will be act as proxy and will transmit gameplay data from client to a match server, without any data processing on lobby server.

Currently, process to establish connection using proxy mode is:
- Player attempt to join open game room using `joinGameRoom` message
- Lobby server validates requests and on success - notifies match server about new player in lobby using control connection
- Match server receives request, establishes new connection to game lobby, sends `serverProxyLogin` message to lobby server and immediately transfers this connection to VCMIServer class to use as connection for gameplay communication
- Lobby server accepts new connection and then sends reply to client using `joinRoomSuccess` message.
- Game client receives message and establishes own side of proxy connection - connects to lobby, sends `clientProxyLogin` message and transfers to ServerHandler class to use as connection for gameplay communication
- Lobby server accepts new connection and moves it into a proxy mode - all packages that will be received by one side of this connection will be re-sent to another side without any processing.

Once the game is over (or if one side disconnects) lobby server will close another side of the connection and erase proxy connection