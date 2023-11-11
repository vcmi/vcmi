/*
 * LobbyServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LobbyServer.h"

#include "SQLiteConnection.h"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

static const std::string DATABASE_PATH = "~/vcmi.db";
static const int LISTENING_PORT = 30303;
//static const std::string SERVER_NAME = GameConstants::VCMI_VERSION + " (server)";
//static const std::string SERVER_UUID = boost::uuids::to_string(boost::uuids::random_generator()());

void LobbyServer::onNewConnection(std::shared_ptr<NetworkConnection>)
{

}

void LobbyServer::onPacketReceived(std::shared_ptr<NetworkConnection>, const std::vector<uint8_t> & message)
{

}

LobbyServer::LobbyServer()
{
	database = SQLiteInstance::open(DATABASE_PATH, true);

}

int main(int argc, const char * argv[])
{
	LobbyServer server;

	server.start(LISTENING_PORT);
	server.run();
}
