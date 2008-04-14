#include "CGameState.h"
#include "CGameInterface.h"
#include "CPlayerInterface.h"
#include <algorithm>
#include "SDL_Thread.h"
#include "SDL_Extensions.h"
#include <queue>


class CMP_stack
{
public:
	bool operator ()(const CStack* a, const CStack* b)
	{
		return (a->creature->speed)>(b->creature->speed);
	}
} cmpst ;

void CGameState::battle(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CArmedInstance *hero1, CArmedInstance *hero2)
{
	curB = new BattleInfo();
	std::vector<CStack*> & stacks = (curB->stacks);

	curB->army1=army1;
	curB->army2=army2;
	curB->hero1=dynamic_cast<CGHeroInstance*>(hero1);
	curB->hero2=dynamic_cast<CGHeroInstance*>(hero2);
	curB->side1=(hero1)?(hero1->tempOwner):(-1);
	curB->side2=(hero2)?(hero2->tempOwner):(-1);
	curB->round = -2;
	curB->stackActionPerformed = false;
	for(std::map<int,std::pair<CCreature*,int> >::iterator i = army1->slots.begin(); i!=army1->slots.end(); i++)
	{
		stacks.push_back(new CStack(i->second.first,i->second.second,0, stacks.size(), true));
		stacks[stacks.size()-1]->ID = stacks.size()-1;
	}
	//initialization of positions
	switch(army1->slots.size()) //for attacker
	{
	case 0:
		break;
	case 1:
		stacks[0]->position = 86; //6
		break;
	case 2:
		stacks[0]->position = 35; //3
		stacks[1]->position = 137; //9
		break;
	case 3:
		stacks[0]->position = 35; //3
		stacks[1]->position = 86; //6
		stacks[2]->position = 137; //9
		break;
	case 4:
		stacks[0]->position = 1; //1
		stacks[1]->position = 69; //5
		stacks[2]->position = 103; //7
		stacks[3]->position = 171; //11
		break;
	case 5:
		stacks[0]->position = 1; //1
		stacks[1]->position = 35; //3
		stacks[2]->position = 86; //6
		stacks[3]->position = 137; //9
		stacks[4]->position = 171; //11
		break;
	case 6:
		stacks[0]->position = 1; //1
		stacks[1]->position = 35; //3
		stacks[2]->position = 69; //5
		stacks[3]->position = 103; //7
		stacks[4]->position = 137; //9
		stacks[5]->position = 171; //11
		break;
	case 7:
		stacks[0]->position = 1; //1
		stacks[1]->position = 35; //3
		stacks[2]->position = 69; //5
		stacks[3]->position = 86; //6
		stacks[4]->position = 103; //7
		stacks[5]->position = 137; //9
		stacks[6]->position = 171; //11
		break;
	default: //fault
		break;
	}
	for(std::map<int,std::pair<CCreature*,int> >::iterator i = army2->slots.begin(); i!=army2->slots.end(); i++)
		stacks.push_back(new CStack(i->second.first,i->second.second,1, stacks.size(), false));
	switch(army2->slots.size()) //for defender
	{
	case 0:
		break;
	case 1:
		stacks[0+army1->slots.size()]->position = 100; //6
		break;
	case 2:
		stacks[0+army1->slots.size()]->position = 49; //3
		stacks[1+army1->slots.size()]->position = 151; //9
		break;
	case 3:
		stacks[0+army1->slots.size()]->position = 49; //3
		stacks[1+army1->slots.size()]->position = 100; //6
		stacks[2+army1->slots.size()]->position = 151; //9
		break;
	case 4:
		stacks[0+army1->slots.size()]->position = 15; //1
		stacks[1+army1->slots.size()]->position = 83; //5
		stacks[2+army1->slots.size()]->position = 117; //7
		stacks[3+army1->slots.size()]->position = 185; //11
		break;
	case 5:
		stacks[0+army1->slots.size()]->position = 15; //1
		stacks[1+army1->slots.size()]->position = 49; //3
		stacks[2+army1->slots.size()]->position = 100; //6
		stacks[3+army1->slots.size()]->position = 151; //9
		stacks[4+army1->slots.size()]->position = 185; //11
		break;
	case 6:
		stacks[0+army1->slots.size()]->position = 15; //1
		stacks[1+army1->slots.size()]->position = 49; //3
		stacks[2+army1->slots.size()]->position = 83; //5
		stacks[3+army1->slots.size()]->position = 117; //7
		stacks[4+army1->slots.size()]->position = 151; //9
		stacks[5+army1->slots.size()]->position = 185; //11
		break;
	case 7:
		stacks[0+army1->slots.size()]->position = 15; //1
		stacks[1+army1->slots.size()]->position = 49; //3
		stacks[2+army1->slots.size()]->position = 83; //5
		stacks[3+army1->slots.size()]->position = 100; //6
		stacks[4+army1->slots.size()]->position = 117; //7
		stacks[5+army1->slots.size()]->position = 151; //9
		stacks[6+army1->slots.size()]->position = 185; //11
		break;
	default: //fault
		break;
	}
	for(int g=0; g<stacks.size(); ++g) //shifting positions of two-hex creatures
	{
		if((stacks[g]->position%17)==1 && stacks[g]->creature->isDoubleWide())
		{
			stacks[g]->position += 1;
		}
		else if((stacks[g]->position%17)==15 && stacks[g]->creature->isDoubleWide())
		{
			stacks[g]->position -= 1;
		}
	}
	std::stable_sort(stacks.begin(),stacks.end(),cmpst);

	//for start inform players about battle
	for(std::map<int, PlayerState>::iterator j=CGI->state->players.begin(); j!=CGI->state->players.end(); ++j)//CGI->state->players.size(); ++j) //for testing
	{
		if (j->first > PLAYER_LIMIT)
			break;
		if(j->second.fogOfWarMap[tile.x][tile.y][tile.z])
		{ //player should be notified
			tribool side = tribool::indeterminate_value;
			if(j->first == curB->side1) //player is attacker
				side = false;
			else if(j->first == curB->side2) //player is defender
				side = true;
			else 
				return; //no witnesses
			if(CGI->playerint[j->second.serial]->human)
			{
				((CPlayerInterface*)( CGI->playerint[j->second.serial] ))->battleStart(army1, army2, tile, curB->hero1, curB->hero2, side);
			}
			else
			{
				//CGI->playerint[j->second.serial]->battleStart(army1, army2, tile, curB->hero1, curB->hero2, side);
			}
		}
	}

	curB->round++;
	if( (curB->hero1 && curB->hero1->getSecSkillLevel(19)>=0) || ( curB->hero2 && curB->hero2->getSecSkillLevel(19)>=0)  )//someone has tactics
	{
		//TODO: wywolania dla rundy -1, ograniczenie pola ruchu, etc
	}

	curB->round++;

	//SDL_Thread * eventh = SDL_CreateThread(battleEventThread, NULL);

	while(true) //do zwyciestwa jednej ze stron
	{
		for(int i=0;i<stacks.size();i++)
		{
			curB->activeStack = i;
			curB->stackActionPerformed = false;
			if(stacks[i]->alive) //niech interfejs ruszy oddzialem
			{
				unsigned char owner = (stacks[i]->owner)?(hero2 ? hero2->tempOwner : 255):(hero1->tempOwner);
				unsigned char serialOwner = -1;
				for(int g=0; g<CGI->playerint.size(); ++g)
				{
					if(CGI->playerint[g]->playerID == owner)
					{
						serialOwner = g;
						break;
					}
				}
				if(serialOwner==255) //neutral unit
				{
				}
				else if(CGI->playerint[serialOwner]->human)
				{
					((CPlayerInterface*)CGI->playerint[serialOwner])->activeStack(stacks[i]->ID);
				}
				else
				{
					//CGI->playerint[serialOwner]->activeStack(stacks[i]->ID);
				}
			}
			//sprawdzic czy po tej akcji ktoras strona nie wygrala bitwy
		}
		SDL_Delay(50);
	}

	for(int i=0;i<stacks.size();i++)
		delete stacks[i];
	delete curB;
	curB = NULL;
}

bool CGameState::battleMoveCreatureStack(int ID, int dest)
{
	//first checks
	if(curB->stackActionPerformed) //because unit cannot be moved more than once
		return false;
	bool stackAtEnd = false; //true if there is a stack at the end of the path (we should attack it)
	unsigned char owner = -1; //owner moved of unit
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->position == dest)
		{
			stackAtEnd = true;
			break;
		}
	}
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->ID == ID)
		{
			owner = curB->stacks[g]->owner;
			break;
		}
	}
	//selecting moved stack
	CStack * curStack = NULL;
	for(int y=0; y<curB->stacks.size(); ++y)
	{
		if(curB->stacks[y]->ID == ID)
		{
			curStack = curB->stacks[y];
			break;
		}
	}
	if(!curStack)
		return false;
	//initing necessary tables
	bool accessibility[187]; //accesibility of hexes
	for(int k=0; k<187; k++)
		accessibility[k] = true;
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->owner == owner) //we don't want to lock enemy's positions
		{
			accessibility[curB->stacks[g]->position] = false;
			if(curB->stacks[g]->creature->isDoubleWide()) //if it's a double hex creature
			{
				if(curB->stacks[g]->attackerOwned)
					accessibility[curB->stacks[g]->position-1] = false;
				else
					accessibility[curB->stacks[g]->position+1] = false;
			}
		}
	}
	int predecessor[187]; //for getting the Path
	for(int b=0; b<187; ++b)
		predecessor[b] = -1;
	//bfsing
	int dists[187]; //calculated distances
	std::queue<int> hexq; //bfs queue
	hexq.push(curStack->position);
	for(int g=0; g<187; ++g)
		dists[g] = 100000000;
	dists[hexq.front()] = 0;
	int curNext = -1; //for bfs loop only (helper var)
	while(!hexq.empty()) //bfs loop
	{
		int curHex = hexq.front();
		hexq.pop();
		curNext = curHex - ( (curHex/17)%2 ? 18 : 17 );
		if((curNext > 0) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //top left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex - ( (curHex/17)%2 ? 17 : 16 );
		if((curNext > 0) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //top right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex - 1;
		if((curNext > 0) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex + 1;
		if((curNext < 187) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex + ( (curHex/17)%2 ? 16 : 17 );
		if((curNext < 187) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //bottom left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex + ( (curHex/17)%2 ? 17 : 18 );
		if((curNext < 187) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //bottom right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
	}
	//following the Path
	if(dists[dest] > curStack->creature->speed)
		return false;
	std::vector<int> path;
	int curElem = dest;
	while(curElem!=curStack->position)
	{
		path.push_back(curElem);
		curElem = predecessor[curElem];
	}
	for(int v=path.size()-1; v>=0; --v)
	{
		if(v!=0 || !stackAtEnd) //it's not the last step
		{
			LOCPLINT->battleStackMoved(ID, path[v], v==path.size()-1, v==0);
			curStack->position = path[v];
		}
		else //if it's last step and we should attack unit at the end
		{
			LOCPLINT->battleStackAttacking(ID, path[v]);
		}
	}
	curB->stackActionPerformed = true;
	LOCPLINT->actionFinished(BattleAction());
	return true;
}

std::vector<int> CGameState::battleGetRange(int ID)
{
	int initialPlace=-1; //position of unit
	int radius=-1; //range of unit
	unsigned char owner = -1; //owner of unit
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->ID == ID)
		{
			initialPlace = curB->stacks[g]->position;
			radius = curB->stacks[g]->creature->speed;
			owner = curB->stacks[g]->owner;
			break;
		}
	}

	bool accessibility[187]; //accesibility of hexes
	for(int k=0; k<187; k++)
		accessibility[k] = true;
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->owner == owner) //we don't want to lock enemy's positions
		{
			accessibility[curB->stacks[g]->position] = false;
			if(curB->stacks[g]->creature->isDoubleWide()) //if it's a double hex creature
			{
				if(curB->stacks[g]->attackerOwned)
					accessibility[curB->stacks[g]->position-1] = false;
				else
					accessibility[curB->stacks[g]->position+1] = false;
			}
		}
	}


	int dists[187]; //calculated distances
	std::queue<int> hexq; //bfs queue
	hexq.push(initialPlace);
	for(int g=0; g<187; ++g)
		dists[g] = 100000000;
	dists[initialPlace] = 0;
	int curNext = -1; //for bfs loop only (helper var)
	while(!hexq.empty()) //bfs loop
	{
		int curHex = hexq.front();
		hexq.pop();
		curNext = curHex - ( (curHex/17)%2 ? 18 : 17 );
		if((curNext > 0) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //top left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
		curNext = curHex - ( (curHex/17)%2 ? 17 : 16 );
		if((curNext > 0) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //top right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
		curNext = curHex - 1;
		if((curNext > 0) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
		curNext = curHex + 1;
		if((curNext < 187) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
		curNext = curHex + ( (curHex/17)%2 ? 16 : 17 );
		if((curNext < 187) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //bottom left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
		curNext = curHex + ( (curHex/17)%2 ? 17 : 18 );
		if((curNext < 187) && accessibility[curNext]  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //bottom right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
		}
	}

	std::vector<int> ret;

	for(int i=0; i<187; ++i)
	{
		if(dists[i]<=radius)
		{
			ret.push_back(i);
		}
	}
	return ret;
}

