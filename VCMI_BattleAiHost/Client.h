#pragma once

class CGameState;
class CConnection;

class CClient/* : public IGameCallback*/
{
public:
	CGameState *gs;
	CConnection *serv;
	CClient()
	{

	}

	//void commitPackage(CPackForClient *) OVERRIDE {};
};