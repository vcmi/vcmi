import socket
from threading import Thread

# server's IP address
SERVER_HOST = "0.0.0.0"
SERVER_PORT = 5002 # port we want to use

# initialize list/set of all connected client's sockets
client_sockets = dict()

class Session:
    total = 1
    joined = 0
    password = ""
    protected = False
    name: str
    host: socket
    players = []
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
        
        
# create a TCP socket
s = socket.socket()
# make the port as reusable port
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
# bind the socket to the address we specified
s.bind((SERVER_HOST, SERVER_PORT))
# listen for upcoming connections
s.listen(5)
print(f"[*] Listening as {SERVER_HOST}:{SERVER_PORT}")

# list of active sessions
sessions = dict()


def handleDisconnection(client: socket):
    sender = client_sockets[client]
    if sender["joined"]:
        if sender["session"].host == client:
            #destroy the session, sending messages inside the function
            deleteSession(sender["session"])
        else:
            sender["session"].leave(client)
            sender["joined"] = False
            message = f":>>KICK:{sender['session'].name}:{sender['username']}"
            for client_socket in sender["session"].players:
                client_socket.send(message.encode())
        updateStatus(sender["session"])
        updateSessions()

    client.close()
    sender["valid"] = False


def send(client: socket, message: str):
    sender = client_sockets[client]
    if "valid" not in sender or sender["valid"]:
        client.send(message.encode())


def broadcast(clients: list, message: str):
    for c in clients:
        send(c, message)


def sendSessions(client: socket):
    msg = f":>>SESSIONS:{len(sessions.keys())}"
    for s in sessions.values():
        msg += f":{s.name}:{s.joined}:{s.total}:{s.protected}"

    send(client, msg)


def updateSessions():
    for s in client_sockets.keys():
        sendSessions(s)


def deleteSession(session: Session):
    msg = f":>>KICK:{session.name}"
    for player in session.players:
        client_sockets[player]["joined"] = False
        msg2 = msg + f":{client_sockets[player]['username']}"
        send(player, msg2)
    
    sessions.pop(session.name)


def updateStatus(session: Session):
    msg = f":>>STATUS:{session.joined}"
    for player in session.players:
        msg += f":{client_sockets[player]['username']}:{client_sockets[player]['ready']}"
    broadcast(session.players, msg)


def dispatch(client: socket, sender: dict, msg: str):
    if msg == '':
        return

    _open = msg.partition('<')
    _close = _open[2].partition('>')
    if _open[0] != '' or _open[1] == '' or _open[2] == '' or _close[0] == '' or _close[1] == '':
        print(f"[!] Incorrect message from {sender['address']}: {msg}")
        return

    _nextTag = _close[2].partition('<')
    tag = _close[0]
    tag_value = _nextTag[0]

    #greetings to the server
    if tag == "GREETINGS":
        if sender["auth"]:
            print(f"[!] Greetings from authorized user {sender['username']} {sender['address']}")
            return
        print(f"[*] User {sender['address']} autorized as {tag_value}")
        sender["username"] = tag_value
        sender["auth"] = True
        sender["joined"] = False
        sendSessions(client)

    #VCMI version received
    if tag == "VER" and sender["auth"]:
        print(f"[*] User {sender['username']} has version {tag_value}")

    #message received
    if tag == "MSG" and sender["auth"]:
        message = f":>>MSG:{sender['username']}:{tag_value}"
        if sender["joined"]:
            broadcast(sender["session"].players, message)
        else:
            broadcast(client_sockets.keys(), message)

    #new session
    if tag == "NEW" and sender["auth"] and not sender["joined"]:
        if tag_value in sessions:
            #refuse creating game
            message = f":>>ERROR:Cannot create session with name {tag_value}, session with this name already exists"
            send(client, message)
            return
        
        sessions[tag_value] = Session(client, tag_value)
        sender["joined"] = True
        sender["ready"] = False
        sender["session"] = sessions[tag_value]

    #set password for the session
    if tag == "PSWD" and sender["auth"] and sender["joined"] and sender["session"].host == client:
        sender["session"].password = tag_value
        sender["session"].protected = tag_value != ""

    #set amount of players to the new session
    if tag == "COUNT" and sender["auth"] and sender["joined"] and sender["session"].host == client:
        if sender["session"].total != 1:
            #refuse changing amount of players
            message = f":>>ERROR:Changing amount of players is not possible for existing session"
            send(client, message)
            return

        sender["session"].total = int(tag_value)
        #now session is ready to be broadcasted
        message = f":>>JOIN:{sender['session'].name}:{sender['username']}"
        send(client, message)
        updateStatus(sender["session"])
        updateSessions()

    #join session
    if tag == "JOIN" and sender["auth"] and not sender["joined"]:
        if tag_value not in sessions:
            message = f":>>ERROR:Session with name {tag_value} doesn't exist"
            send(client, message)
            return
        
        if sessions[tag_value].joined >= sessions[tag_value].total:
            message = f":>>ERROR:Session {tag_value} is full"
            send(client, message)
            return

        if sessions[tag_value].started:
            message = f":>>ERROR:Session {tag_value} is started"
            send(client, message)
            return

        sender["joined"] = True
        sender["ready"] = False
        sender["session"] = sessions[tag_value]

    if tag == "PSWD" and sender["auth"] and sender["joined"] and sender["session"].host != client:
        if not sender["session"].protected or sender["session"].password == tag_value:
            sender["session"].join(client)
            message = f":>>JOIN:{sender['session'].name}:{sender['username']}"
            broadcast(sender["session"].players, message)
            updateStatus(sender["session"])
            updateSessions()

        else:
            sender["joined"] = False
            message = f":>>ERROR:Incorrect password"
            send(client, message)
            return

    #leaving session
    if tag == "LEAVE" and sender["auth"] and sender["joined"] and sender["session"].name == tag_value:
        if sender["session"].host == client:
            #destroy the session, sending messages inside the function
            deleteSession(sender["session"])
        else:
            message = f":>>KICK:{sender['session'].name}:{sender['username']}"
            broadcast(sender["session"].players, message)
            sender["session"].leave(client)
            sender["joined"] = False
            updateStatus(sender["session"])
        updateSessions()


    dispatch(client, sender, _nextTag[1] + _nextTag[2])


def listen_for_client(cs):
    """
    This function keep listening for a message from `cs` socket
    Whenever a message is received, broadcast it to all other connected clients
    """
    while True:
        try:
            # keep listening for a message from `cs` socket
            msg = cs.recv(2048).decode()
        except Exception as e:
            # client no longer connected
            print(f"[!] Error: {e}")
            handleDisconnection(cs)            
            return
            
        dispatch(cs, client_sockets[cs], msg)


while True:
    # we keep listening for new connections all the time
    client_socket, client_address = s.accept()
    print(f"[+] {client_address} connected.")
    # add the new connected client to connected sockets
    client_sockets[client_socket] = {"address": client_address, "auth": False, "username": ""}
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