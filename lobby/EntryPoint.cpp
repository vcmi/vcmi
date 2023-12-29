/*
 * EntryPoint.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LobbyServer.h"

static const std::string DATABASE_PATH = "/home/ivan/vcmi.db";
static const int LISTENING_PORT = 30303;
//static const std::string SERVER_NAME = GameConstants::VCMI_VERSION + " (server)";
//static const std::string SERVER_UUID = boost::uuids::to_string(boost::uuids::random_generator()());

int main(int argc, const char * argv[])
{
	LobbyServer server(DATABASE_PATH);

	server.start(LISTENING_PORT);
	server.run();

	return 0;
}
