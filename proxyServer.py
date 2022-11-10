import socket
import re
import uuid
import struct
import logging
from threading import Thread

PROTOCOL_VERSION_MIN = 1
PROTOCOL_VERSION_MAX = 1

# server's IP address
SERVER_HOST = "0.0.0.0"
SERVER_PORT = 5002 # port we want to use

#logging
logHandlerHighlevel = logging.FileHandler('proxyServer.log')
logHandlerHighlevel.setLevel(logging.WARNING)

logHandlerLowlevel = logging.FileHandler('proxyServer_debug.log')
logHandlerLowlevel.setLevel(logging.DEBUG)

logging.basicConfig(handlers=[logHandlerHighlevel, logHandlerLowlevel], level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S')

def receive_packed(sock):
    # Read message length and unpack it into an integer
    raw_msglen = recvall(sock, 4)
    if not raw_msglen:
        return None
    msglen = struct.unpack('<I', raw_msglen)[0]
    # Read the message data
    return recvall(sock, msglen)

def recvall(sock, n):
    # Helper function to recv n bytes or return None if EOF is hit
    data = bytearray()
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return None
        data.extend(packet)
    return data


class GameConnection:
    server: socket # socket to vcmiserver
    client: socket # socket to vcmiclient
    serverInit = False # if vcmiserver already connected
    clientInit = False # if vcmiclient already connected

    def __init__(self) -> None:
        self.server = None
        self.client = None
        pass


class Room:
    total = 1 # total amount of players
    joined = 0 # amount of players joined to the session
    password = "" # password to connect
    protected = False # if True, password is required to join to the session
    name: str # name of session
    host: socket # player socket who created the room
    players = [] # list of sockets of players, joined to the session
    started = False

    def __init__(self, host: socket, name: str) -> None:
        self.name = name
        self.host = host
        self.players = [host]
        self.joined += 1

    def isJoined(self, player: socket) -> bool:
        return player in self.players

    def join(self, player: socket):
        if not self.isJoined(player) and self.joined < self.total:
            self.players.append(player)
            self.joined += 1

    def leave(self, player: socket):
        if not self.isJoined(player) or player == self.host:
            return

        self.players.remove(player)
        self.joined -= 1


class Session:
    name: str # name of session
    host_uuid: str # uuid of vcmiserver for hosting player
    clients_uuid: list # list od vcmiclients uuid
    players: list # list of sockets of players, joined to the session
    connections: list # list of GameConnections for vcmiclient/vcmiserver (game mode)

    def __init__(self) -> None:
        self.name = ""
        self.host_uuid = ""
        self.clients_uuid = []
        self.connections = []
        pass

    def addConnection(self, conn: socket, isServer: bool):
        #find uninitialized server connection
        for gc in self.connections:
            if isServer and not gc.serverInit:
                gc.server = conn
                gc.serverInit = True
            if not isServer and not gc.clientInit:
                gc.client = conn
                gc.clientInit = True
            
        #no existing connection - create the new one
        gc = GameConnection()
        if isServer:
            gc.server = conn
            gc.serverInit = True
        else:
            gc.client = conn
            gc.clientInit = True
        self.connections.append(gc)

    def removeConnection(self, conn: socket):
        newConnections = []
        for c in self.connections:
            if c.server == conn:
                c.server = None
                c.serverInit = False
            if c.client == conn:
                c.client = None
                c.clientInit = False
            if c.server != None or c.client != None:
                newConnections.append(c)
        self.connections = newConnections

    def validPipe(self, conn) -> bool:
        for gc in self.connections:
            if gc.server == conn or gc.client == conn:
                return gc.serverInit and gc.clientInit
        return False

    def getPipe(self, conn) -> socket:
        for gc in self.connections:
            if gc.server == conn:
                return gc.client
            if gc.client == conn:
                return gc.server


class Client:
    auth: bool

    def __init__(self) -> None:
        self.auth = False


class ClientLobby(Client):
    joined: bool
    username: str
    room: Room
    protocolVersion: int
    encoding: str
    ready: bool
    vcmiversion: str #TODO: check version compatibility

    def __init__(self) -> None:
        super().__init__()
        self.room = None
        self.joined = False
        self.username = ""
        self.protocolVersion = 0
        self.encoding = 'utf8'
        self.ready = False
        self.vcmiversion = ""


class ClientPipe(Client):
    apptype: str #client/server
    prevmessages: list
    session: Session
    uuid: str

    def __init__(self) -> None:
        super().__init__()
        self.prevmessages = []
        self.session = None
        self.apptype = ""
        self.uuid = ""


class Sender:
    address: str #full client address
    client: Client

    def __init__(self) -> None:
        self.client = None
        pass

    def isLobby(self) -> bool:
        return isinstance(self.client, ClientLobby)

    def isPipe(self) -> bool:
        return isinstance(self.client, ClientPipe)


# create a TCP socket
s = socket.socket()
# make the port as reusable port
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
# bind the socket to the address we specified
s.bind((SERVER_HOST, SERVER_PORT))
# listen for upcoming connections
s.listen(10)
logging.critical(f"[*] Listening as {SERVER_HOST}:{SERVER_PORT}")

# active rooms
rooms = {}

# list of active sessions
sessions = []

# initialize list/set of all connected client's sockets
client_sockets = {}


def handleDisconnection(client: socket):
    logging.warning(f"[!] Disconnecting client {client}")
    
    if not client in client_sockets:
        return

    sender = client_sockets[client]
    if sender.isLobby() and sender.client.joined:
        if not sender.client.room.started:
            if sender.client.room.host == client:
                #destroy the session, sending messages inside the function
                deleteRoom(sender.client.room)
            else:
                sender.client.room.leave(client)
                sender.client.joined = False
                message = f":>>KICK:{sender.client.room.name}:{sender.client.username}"
                broadcast(sender.client.room.players, message.encode())
            updateStatus(sender.client.room)
            updateRooms()

    if sender.isPipe() and sender.client.auth:
        if sender.client.session in sessions:
            sender.client.session.removeConnection(client)
            if not len(sender.client.session.connections):
                logging.warning(f"[*] Destroying session {sender.client.session.name}")
                sessions.remove(sender.client.session)

    client.close()
    client_sockets.pop(client)


def send(client: socket, message: str):
    if client in client_sockets.keys():
        sender = client_sockets[client]
        client.send(message.encode(errors='replace'))


def broadcast(clients: list, message: str):
    for c in clients:
        if client_sockets[c].isLobby() and client_sockets[c].client.auth:
            send(c, message)


def sendRooms(client: socket):
    msg2 = ""
    counter = 0
    for s in rooms.values():
        if not s.started:
            msg2 += f":{s.name}:{s.joined}:{s.total}:{s.protected}"
            counter += 1
    msg = f":>>SESSIONS:{counter}{msg2}"

    send(client, msg)


def updateRooms():
    for s in client_sockets.keys():
        sendRooms(s)


def deleteRoom(room: Room):
    msg = f":>>KICK:{room.name}"
    for player in room.players:
        client_sockets[player].client.joined = False
        msg2 = msg + f":{client_sockets[player].client.username}"
        send(player, msg2)
    
    logging.warning(f"[*] Destroying room {room.name}")
    rooms.pop(room.name)


def updateStatus(room: Room):
    msg = f":>>STATUS:{room.joined}"
    for player in room.players:
        msg += f":{client_sockets[player].client.username}:{client_sockets[player].client.ready}"
    broadcast(room.players, msg)


def startRoom(room: Room):
    room.started = True
    session = Session()
    session.name = room.name
    sessions.append(session)
    logging.warning(f"[*] Starting session {session.name}")
    session.host_uuid = str(uuid.uuid4())
    hostMessage = f":>>HOST:{session.host_uuid}:{room.joined - 1}" #one client will be connected locally
    #host message must be before start message
    send(room.host, hostMessage)

    for player in room.players:
        _uuid = str(uuid.uuid4())
        session.clients_uuid.append(_uuid)
        msg = f":>>START:{_uuid}"
        send(player, msg)
        #remove this connection
        player.close
        client_sockets.pop(player)

    #this room shall not exist anymore
    logging.info(f"[*] Destroying room {room.name}")
    rooms.pop(room.name)


def dispatch(cs: socket, sender: Sender, arr: bytes):
    
    if arr == None or len(arr) == 0:
        return

    #check for game mode connection
    msg = str(arr)
    if msg.find("Aiya!") != -1:
        sender.client = ClientPipe()
        logging.debug("  vcmi recognized")

    if sender.isPipe():
        if sender.client.auth: #if already playing - sending raw bytes as is
            sender.client.prevmessages.append(arr)
        else:
            sender.client.prevmessages.append(struct.pack('<I', len(arr)) + arr) #pack message
            logging.debug("  packing message")
            #search fo application type in the message
            match = re.search(r"\((\w+)\)", msg)
            _appType = ''
            if match != None:
                _appType = match.group(1)
                sender.client.apptype = _appType
            
            #extract uuid from message
            _uuid = arr.decode()
            logging.debug(f"  decoding {_uuid}")
            if not _uuid == '' and not sender.client.apptype == '':
                #search for uuid
                for session in sessions:
                    #verify uuid of connected application
                    if _uuid.find(session.host_uuid) != -1 and sender.client.apptype == "server":
                        logging.debug(f"  apptype {sender.client.apptype} uuid {_uuid}")
                        session.addConnection(cs, True)
                        sender.client.session = session
                        sender.client.auth = True
                        #read boolean flag for the endian
                        # this is workaround to send only one remaining byte
                        # WARNING: reversed byte order is not supported
                        sender.client.prevmessages.append(cs.recv(1))
                        logging.debug(f"  binding server connection to session {session.name}")
                        break

                    if sender.client.apptype == "client":
                        for p in session.clients_uuid:
                            if _uuid.find(p) != -1:
                                logging.debug(f"  apptype {sender.client.apptype} uuid {_uuid}")
                                #client connection
                                session.addConnection(cs, False)
                                sender.client.session = session
                                sender.client.auth = True
                                #read boolean flag for the endian
                                # this is workaround to send only one remaining byte
                                # WARNING: reversed byte order is not supported
                                sender.client.prevmessages.append(cs.recv(1))
                                logging.debug(f"  binding client connection to session {session.name}")
                                break

    #game mode
    if sender.isPipe() and sender.client.auth and sender.client.session.validPipe(cs):
        #send messages from queue
        opposite = sender.client.session.getPipe(cs)
        for x in client_sockets[opposite].client.prevmessages:
            cs.sendall(x)
        client_sockets[opposite].client.prevmessages.clear()

        for x in sender.client.prevmessages:
            opposite.sendall(x)

        sender.client.prevmessages.clear()
        return

    #we are in pipe mode but game still not started - waiting other clients to connect
    if sender.isPipe():
        logging.debug(f"  waiting other clients")
        return
    
    #intialize lobby mode
    if not sender.isLobby():
        if len(arr) < 2: 
            logging.critical("[!] Error: unknown client tries to connect")
            #TODO: block address? close the socket?
            return

        sender.client = ClientLobby()

        # first byte is protocol version
        sender.client.protocolVersion = arr[0]
        if arr[0] < PROTOCOL_VERSION_MIN or arr[0] > PROTOCOL_VERSION_MAX:
            logging.critical(f"[!] Error: client has incompatbile protocol version {arr[0]}")
            send(cs, ":>>ERROR:Cannot connect to remote server due to protocol incompatibility")
            return

        # second byte is an encoding str size
        if arr[1] == 0:
            sender.client.encoding = "utf8"
        else:
            if len(arr) < arr[1] + 2:
                send(cs, ":>>ERROR:Protocol error")
                return
            # read encoding string
            sender.client.encoding = arr[2:(arr[1] + 2)].decode(errors='ignore')
            arr = arr[(arr[1] + 2):]
            msg = str(arr)

    msg = arr.decode(encoding=sender.client.encoding, errors='replace')
    _open = msg.partition('<')
    _close = _open[2].partition('>')
    if _open[0] != '' or _open[1] == '' or _open[2] == '' or _close[0] == '' or _close[1] == '':
        logging.error(f"[!] Incorrect message from {sender.address}: {msg}")
        return

    _nextTag = _close[2].partition('<')
    tag = _close[0]
    tag_value = _nextTag[0]

    #greetings to the server
    if tag == "GREETINGS":
        if sender.client.auth:
            logging.error(f"[!] Greetings from authorized user {sender.client.username} {sender.address}")
            send(cs, ":>>ERROR:User already authorized")
            return

        if len(tag_value) < 3:
            send(cs, f":>>ERROR:Too short username {tag_value}")
            return

        for user in client_sockets.values():
            if user.isLobby() and user.client.username == tag_value:
                send(cs, f":>>ERROR:Can't connect with the name {tag_value}. This login is already occpupied")
                return
        
        logging.info(f"[*] {sender.address} autorized as {tag_value}")
        sender.client.username = tag_value
        sender.client.auth = True
        sendRooms(cs)

    #VCMI version received
    if tag == "VER" and sender.client.auth:
        logging.info(f"[*] User {sender.client.username} has version {tag_value}")
        sender.client.vcmiversion = tag_value

    #message received
    if tag == "MSG" and sender.client.auth:
        message = f":>>MSG:{sender.client.username}:{tag_value}"
        if sender.client.joined:
            broadcast(sender.client.room.players, message) #send message only to players in the room
        else:
            targetClients = [i for i in client_sockets.keys() if not client_sockets[i].client.joined]
            broadcast(targetClients, message)

    #new room
    if tag == "NEW" and sender.client.auth and not sender.client.joined:
        if tag_value in rooms:
            #refuse creating game
            message = f":>>ERROR:Cannot create session with name {tag_value}, session with this name already exists"
            send(cs, message)
            return

        if tag_value == "" or tag_value.startswith(" ") or len(tag_value) < 3:
            #refuse creating game
            message = f":>>ERROR:Cannot create session with invalid name {tag_value}"
            send(cs, message)
            return
        
        rooms[tag_value] = Room(cs, tag_value)
        sender.client.joined = True
        sender.client.ready = False
        sender.client.room = rooms[tag_value]

    #set password for the session
    if tag == "PSWD" and sender.client.auth and sender.client.joined and sender.client.room.host == cs:
        sender.client.room.password = tag_value
        sender.client.room.protected = bool(tag_value != "")

    #set amount of players to the new room
    if tag == "COUNT" and sender.client.auth and sender.client.joined and sender.client.room.host == cs:
        if sender.client.room.total != 1:
            #refuse changing amount of players
            message = f":>>ERROR:Changing amount of players is not possible for existing session"
            send(cs, message)
            return

        if int(tag_value) < 2 or int(tag_value) > 8:
            #refuse and cleanup room
            deleteRoom(sender.client.room)
            message = f":>>ERROR:Cannot create room with invalid amount of players"
            send(cs, message)
            return

        sender.client.room.total = int(tag_value)
        message = f":>>CREATED:{sender.client.room.name}"
        send(cs, message)
        #now room is ready to be broadcasted
        message = f":>>JOIN:{sender.client.room.name}:{sender.client.username}"
        send(cs, message)
        updateStatus(sender.client.room)
        updateRooms()

    #join session
    if tag == "JOIN" and sender.client.auth and not sender.client.joined:
        if tag_value not in rooms:
            message = f":>>ERROR:Room with name {tag_value} doesn't exist"
            send(cs, message)
            return
        
        if rooms[tag_value].joined >= rooms[tag_value].total:
            message = f":>>ERROR:Room {tag_value} is full"
            send(cs, message)
            return

        if rooms[tag_value].started:
            message = f":>>ERROR:Session {tag_value} is started"
            send(cs, message)
            return

        sender.client.joined = True
        sender.client.ready = False
        sender.client.room = rooms[tag_value]

    if tag == "PSWD" and sender.client.auth and sender.client.joined and sender.client.room.host != cs:
        if not sender.client.room.protected or sender.client.room.password == tag_value:
            sender.client.room.join(cs)
            message = f":>>JOIN:{sender.client.room.name}:{sender.client.username}"
            broadcast(sender.client.room.players, message)
            updateStatus(sender.client.room)
            updateRooms()

        else:
            sender.client.joined = False
            message = f":>>ERROR:Incorrect password"
            send(cs, message)
            return

    #leaving session
    if tag == "LEAVE" and sender.client.auth and sender.client.joined and sender.client.room.name == tag_value:
        if sender.client.room.host == cs:
            #destroy the session, sending messages inside the function
            deleteRoom(sender.client.room)
        else:
            message = f":>>KICK:{sender.client.room.name}:{sender.client.username}"
            broadcast(sender.client.room.players, message)
            sender.client.room.leave(cs)
            sender.client.joined = False
            updateStatus(sender.client.room)
        updateRooms() 

    if tag == "READY" and sender.client.auth and sender.client.joined and sender.client.room.name == tag_value:
        if sender.client.room.joined > 0 and sender.client.room.host == cs:
            startRoom(sender.client.room)
            updateRooms()

    dispatch(cs, sender, (_nextTag[1] + _nextTag[2]).encode())


def listen_for_client(cs):
    """
    This function keep listening for a message from `cs` socket
    Whenever a message is received, broadcast it to all other connected clients
    """
    while True:
        try:
            # keep listening for a message from `cs` socket
            if client_sockets[cs].isPipe() and client_sockets[cs].client.auth:
                msg = cs.recv(4096)
            else:
                msg = receive_packed(cs)

            if msg == None or msg == b'':
                handleDisconnection(cs)
                return

            dispatch(cs, client_sockets[cs], msg)

        except Exception as e:
            # client no longer connected
            logging.error(f"[!] Error: {e}")
            handleDisconnection(cs)
            return


while True:
    # we keep listening for new connections all the time
    client_socket, client_address = s.accept()
    logging.warning(f"[+] {client_address} connected.")
    # add the new connected client to connected sockets
    client_sockets[client_socket] = Sender()
    client_sockets[client_socket].address = client_address
    # start a new thread that listens for each client's messages
    t = Thread(target=listen_for_client, args=(client_socket,))
    # make the thread daemon so it ends whenever the main thread ends
    t.daemon = True
    # start the thread
    t.start()

 # close client sockets
for cs in client_sockets:
    cs.close()
# close server socket
s.close()