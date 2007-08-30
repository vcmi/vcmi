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
};

class CGameState
{
	int currentPlayer;
	std::map<int,PlayerState> players; //color <-> playerstate
public:
	friend CCallback;
	friend int _tmain(int argc, _TCHAR* argv[]);
	CCallback * cb; //for communication between PlayerInterface/AI and GameState
};

#endif //CGAMESTATE_H