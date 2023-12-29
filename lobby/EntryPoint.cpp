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

#include "../lib/VCMIDirs.h"

static const int LISTENING_PORT = 30303;

int main(int argc, const char * argv[])
{
	auto databasePath = VCMIDirs::get().userDataPath() / "vcmiLobby.db";

	LobbyServer server(databasePath);

	server.start(LISTENING_PORT);
	server.run();

	return 0;
}
