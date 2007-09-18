#ifndef CGAMESTATE_H
#define CGAMESTATE_H

class CHeroInstance;
class CTownInstance;
class CCallback;

struct PlayerState
{
public:
	int color;
	std::vector<std::vector<std::vector<char> > >fogOfWarMap; //true - visible, false - hidden
	std::vector<int> resources;
	std::vector<CHeroInstance *> heroes;
	std::vector<CTownInstance *> towns;
	PlayerState():color(-1){};
};

class CGameState
{
	int currentPlayer;

	int day; //total number of days in game
	std::map<int,PlayerState> players; //color <-> playerstate
public:
	friend CCallback;
	friend int _tmain(int argc, _TCHAR* argv[]);
	friend void initGameState(CGameInfo * cgi);
	CCallback * cb; //for communication between PlayerInterface/AI and GameState
};

#endif //CGAMESTATE_H
