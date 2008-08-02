#include "CGameState.h"
#include "CGameInterface.h"
#include "CPlayerInterface.h"
#include <algorithm>
#include "SDL_thread.h"
#include "SDL_Extensions.h"
#include "CBattleInterface.h" //for CBattleHex
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
				continue; //no witnesses
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

	while(true) //till the end of the battle ;]
	{
		bool battleEnd = false;
		//tell players about next round
		for(int v=0; v<CGI->playerint.size(); ++v)
			CGI->playerint[v]->battleNewRound(curB->round);

		//stack loop
		for(int i=0;i<stacks.size();i++)
		{
			curB->activeStack = i;
			curB->stackActionPerformed = false;
			if(stacks[i]->alive) //indicate posiibility of making action for this unit
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
					BattleAction ba = ((CPlayerInterface*)CGI->playerint[serialOwner])->activeStack(stacks[i]->ID);
					switch(ba.actionType)
					{
					case 2: //walk
						{
							battleMoveCreatureStack(ba.stackNumber, ba.destinationTile);
						}
					case 3: //defend
						{
							break;
						}
					case 4: //retreat/flee
						{
							for(int v=0; v<CGI->playerint.size(); ++v) //tell about the end of this battle to interfaces
								CGI->playerint[v]->battleEnd(army1, army2, hero1, hero2, std::vector<int>(), 0, false);
							battleEnd = true;
							break;
						}
					case 6: //walk or attack
						{
							battleMoveCreatureStack(ba.stackNumber, ba.destinationTile);
							battleAttackCreatureStack(ba.stackNumber, ba.destinationTile);
							break;
						}
					case 7: //shoot
						{
							battleShootCreatureStack(ba.stackNumber, ba.destinationTile);
							break;
						}
					}
				}
				else
				{
					//CGI->playerint[serialOwner]->activeStack(stacks[i]->ID);
				}
			}
			if(battleEnd)
				break;
			//sprawdzic czy po tej akcji ktoras strona nie wygrala bitwy
		}
		if(battleEnd)
			break;
		curB->round++;
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

	unsigned char owner = -1; //owner moved of unit
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->ID == ID)
		{
			owner = curB->stacks[g]->owner;
			break;
		}
	}

	bool stackAtEnd = false; //true if there is a stack at the end of the path (we should attack it)
	int numberOfStackAtEnd = -1;
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->position == dest 
			|| (curB->stacks[g]->creature->isDoubleWide() && curB->stacks[g]->attackerOwned && curB->stacks[g]->position-1 == dest)
			|| (curB->stacks[g]->creature->isDoubleWide() && !curB->stacks[g]->attackerOwned && curB->stacks[g]->position+1 == dest))
		{
			if(curB->stacks[g]->alive)
			{
				stackAtEnd = true;
				numberOfStackAtEnd = g;
				break;
			}
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
		if(curB->stacks[g]->ID != ID && curB->stacks[g]->alive) //we don't want to lock enemy's positions and this units' position
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
	accessibility[dest] = true;
	if(curStack->creature->isDoubleWide()) //locking positions unreachable by two-hex creatures
	{
		bool mac[187];
		for(int b=0; b<187; ++b)
		{
			//
			//	&& (  ? (curStack->attackerOwned ? accessibility[curNext-1] : accessibility[curNext+1]) : true )
			mac[b] = accessibility[b];
			if( accessibility[b] && !(curStack->attackerOwned ? accessibility[b-1] : accessibility[b+1]))
			{
				mac[b] = false;
			}
		}
		mac[curStack->attackerOwned ? curStack->position+1 : curStack->position-1]=true;
		for(int v=0; v<187; ++v)
			accessibility[v] = mac[v];
		//removing accessibility for side hexes
		for(int v=0; v<187; ++v)
			if(curStack->attackerOwned ? (v%17)==1 : (v%17)==15)
				accessibility[v] = false;
	}
	if(!stackAtEnd && !accessibility[dest])
		return false;
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
		if((curNext > 0) && (accessibility[curNext] || curNext==dest) && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //top left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex - ( (curHex/17)%2 ? 17 : 16 );
		if((curNext > 0) && (accessibility[curNext] || curNext==dest)  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //top right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex - 1;
		if((curNext > 0) && (accessibility[curNext] || curNext==dest)  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex + 1;
		if((curNext < 187) && (accessibility[curNext] || curNext==dest)  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex + ( (curHex/17)%2 ? 16 : 17 );
		if((curNext < 187) && (accessibility[curNext] || curNext==dest)  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //bottom left
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
		curNext = curHex + ( (curHex/17)%2 ? 17 : 18 );
		if((curNext < 187) && (accessibility[curNext] || curNext==dest)  && (dists[curHex] + 1 < dists[curNext]) && (curNext)%17!=0 && (curNext)%17!=16) //bottom right
		{
			hexq.push(curNext);
			dists[curNext] = dists[curHex] + 1;
			predecessor[curNext] = curHex;
		}
	}
	//following the Path
	if(dists[dest] > curStack->creature->speed && !(stackAtEnd && dists[dest] == curStack->creature->speed+1)) //we can attack a stack if we can go to adjacent hex
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
			LOCPLINT->battleStackMoved(ID, path[v], v==path.size()-1, v==0 || (stackAtEnd && v==1) );
			curStack->position = path[v];
		}
		else //if it's last step and we should attack unit at the end
		{
			LOCPLINT->battleStackAttacking(ID, path[v]);
			//counting dealt damage
			int finalDmg = calculateDmg(curStack, curB->stacks[numberOfStackAtEnd]);

			//applying damages
			int cresKilled = finalDmg / curB->stacks[numberOfStackAtEnd]->creature->hitPoints;
			int damageFirst = finalDmg % curB->stacks[numberOfStackAtEnd]->creature->hitPoints;

			if( curB->stacks[numberOfStackAtEnd]->firstHPleft <= damageFirst )
			{
				curB->stacks[numberOfStackAtEnd]->amount -= 1;
				curB->stacks[numberOfStackAtEnd]->firstHPleft += curB->stacks[numberOfStackAtEnd]->creature->hitPoints - damageFirst;
			}
			else
			{
				curB->stacks[numberOfStackAtEnd]->firstHPleft -= damageFirst;
			}

			int cresInstackBefore = curB->stacks[numberOfStackAtEnd]->amount; 
			curB->stacks[numberOfStackAtEnd]->amount -= cresKilled;
			if(curB->stacks[numberOfStackAtEnd]->amount<=0) //stack killed
			{
				curB->stacks[numberOfStackAtEnd]->amount = 0;
				LOCPLINT->battleStackKilled(curB->stacks[numberOfStackAtEnd]->ID, finalDmg, std::min(cresKilled, cresInstackBefore) , ID, false);
				curB->stacks[numberOfStackAtEnd]->alive = false;
			}
			else
			{
				LOCPLINT->battleStackIsAttacked(curB->stacks[numberOfStackAtEnd]->ID, finalDmg, std::min(cresKilled, cresInstackBefore), ID, false);
			}

			//damage applied
		}
	}
	curB->stackActionPerformed = true;
	LOCPLINT->actionFinished(BattleAction());
	return true;
}

bool CGameState::battleAttackCreatureStack(int ID, int dest)
{
	int attackedCreaure = -1; //-1 - there is no attacked creature
	for(int b=0; b<curB->stacks.size(); ++b) //TODO: make upgrades for two-hex cres.
	{
		if(curB->stacks[b]->position == dest)
		{
			attackedCreaure = curB->stacks[b]->ID;
			break;
		}
	}
	if(attackedCreaure == -1)
		return false;
	//LOCPLINT->cb->
	return true;
}

bool CGameState::battleShootCreatureStack(int ID, int dest)
{
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
	int IDOfStackAtEnd = battleGetStack(dest);
	int numberOfStackAtEnd = -1;
	for(int v=0; v<curB->stacks.size(); ++v)
	{
		if(curB->stacks[v]->ID == IDOfStackAtEnd)
		{
			numberOfStackAtEnd = v;
			break;
		}
	}

	if(IDOfStackAtEnd == -1 || curB->stacks[numberOfStackAtEnd]->owner == curStack->owner || !curB->stacks[numberOfStackAtEnd]->alive)
		return false;

	LOCPLINT->battleStackIsShooting(ID, dest);

	//counting dealt damage
	int finalDmg = calculateDmg(curStack, curB->stacks[numberOfStackAtEnd]);

	//applying damages
	int cresKilled = finalDmg / curB->stacks[ID]->creature->hitPoints;
	int damageFirst = finalDmg % curB->stacks[ID]->creature->hitPoints;

	if( curB->stacks[numberOfStackAtEnd]->firstHPleft <= damageFirst )
	{
		curB->stacks[numberOfStackAtEnd]->amount -= 1;
		curB->stacks[numberOfStackAtEnd]->firstHPleft += curB->stacks[numberOfStackAtEnd]->creature->hitPoints - damageFirst;
	}
	else
	{
		curB->stacks[numberOfStackAtEnd]->firstHPleft -= damageFirst;
	}

	int cresInstackBefore = curB->stacks[numberOfStackAtEnd]->amount; 
	curB->stacks[numberOfStackAtEnd]->amount -= cresKilled;
	if(curB->stacks[numberOfStackAtEnd]->amount<=0) //stack killed
	{
		curB->stacks[numberOfStackAtEnd]->amount = 0;
		LOCPLINT->battleStackKilled(curB->stacks[numberOfStackAtEnd]->ID, finalDmg, std::min(cresKilled, cresInstackBefore), ID, true);
		curB->stacks[numberOfStackAtEnd]->alive = false;
	}
	else
	{
		LOCPLINT->battleStackIsAttacked(curB->stacks[numberOfStackAtEnd]->ID, finalDmg, std::min(cresKilled, cresInstackBefore), ID, true);
	}

	//damage applied
	return true;
}

int CGameState::battleGetStack(int pos)
{
	for(int g=0; g<curB->stacks.size(); ++g)
	{
		if(curB->stacks[g]->position == pos ||
				( curB->stacks[g]->creature->isDoubleWide() &&
					( (curB->stacks[g]->attackerOwned && curB->stacks[g]->position-1 == pos) ||
						(!curB->stacks[g]->attackerOwned && curB->stacks[g]->position+1 == pos)
					)
				)
			)
			return curB->stacks[g]->ID;
	}
	return -1;
}

int CGameState::calculateDmg(const CStack* attacker, const CStack* defender)
{
	int attackDefenseBonus = attacker->creature->attack - defender->creature->defence;
	int damageBase = 0;
	if(attacker->creature->damageMax == attacker->creature->damageMin) //constant damage
	{
		damageBase = attacker->creature->damageMin;
	}
	else
	{
		damageBase = rand()%(attacker->creature->damageMax - attacker->creature->damageMin) + attacker->creature->damageMin + 1;
	}

	float dmgBonusMultiplier = 1.0;
	if(attackDefenseBonus < 0) //decreasing dmg
	{
		if(0.02f * (-attackDefenseBonus) > 0.3f)
		{
			dmgBonusMultiplier += -0.3f;
		}
		else
		{
			dmgBonusMultiplier += 0.02f * attackDefenseBonus;
		}
	}
	else //increasing dmg
	{
		if(0.05f * attackDefenseBonus > 4.0f)
		{
			dmgBonusMultiplier += 4.0f;
		}
		else
		{
			dmgBonusMultiplier += 0.05f * attackDefenseBonus;
		}
	}

	return (float)damageBase * (float)attacker->amount * dmgBonusMultiplier;
}

std::vector<int> CGameState::battleGetRange(int ID)
{
	int initialPlace=-1; //position of unit
	int radius=-1; //range of unit
	unsigned char owner = -1; //owner of unit
	//selecting stack
	CStack * curStack = NULL;
	for(int y=0; y<curB->stacks.size(); ++y)
	{
		if(curB->stacks[y]->ID == ID)
		{
			curStack = curB->stacks[y];
			break;
		}
	}

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
		if(curB->stacks[g]->ID != ID && curB->stacks[g]->alive) //we don't want to lock current unit's position
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

	if(curStack->creature->isDoubleWide()) //locking positions unreachable by two-hex creatures
	{
		bool mac[187];
		for(int b=0; b<187; ++b)
		{
			//
			//	&& (  ? (curStack->attackerOwned ? accessibility[curNext-1] : accessibility[curNext+1]) : true )
			mac[b] = accessibility[b];
			if( accessibility[b] && !(curStack->attackerOwned ? accessibility[b-1] : accessibility[b+1]))
			{
				mac[b] = false;
			}
		}
		mac[curStack->attackerOwned ? curStack->position+1 : curStack->position-1]=true;
		for(int v=0; v<187; ++v)
			accessibility[v] = mac[v];
		//removing accessibility for side hexes
		for(int v=0; v<187; ++v)
			if(curStack->attackerOwned ? (v%17)==1 : (v%17)==15)
				accessibility[v] = false;
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

	std::vector<int> additionals;

	//adding enemies' positions
	for(int c=0; c<curB->stacks.size(); ++c)
	{
		if(curB->stacks[c]->alive && curB->stacks[c]->owner != owner)
		{
			for(int g=0; g<ret.size(); ++g)
			{
				if(CBattleHex::mutualPosition(ret[g], curB->stacks[c]->position) != -1)
				{
					additionals.push_back(curB->stacks[c]->position);
				}
				if(curB->stacks[c]->creature->isDoubleWide() && curB->stacks[c]->attackerOwned && CBattleHex::mutualPosition(ret[g], curB->stacks[c]->position-1) != -1)
				{
					additionals.push_back(curB->stacks[c]->position-1);
				}
				if(curB->stacks[c]->creature->isDoubleWide() && !curB->stacks[c]->attackerOwned && CBattleHex::mutualPosition(ret[g], curB->stacks[c]->position+1) != -1)
				{
					additionals.push_back(curB->stacks[c]->position+1);
				}
			}
		}
	}
	for(int g=0; g<additionals.size(); ++g)
	{
		ret.push_back(additionals[g]);
	}

	std::sort(ret.begin(), ret.end());
	std::vector<int>::iterator nend = std::unique(ret.begin(), ret.end());

	std::vector<int> ret2;

	for(std::vector<int>::iterator it = ret.begin(); it != nend; ++it)
	{
		ret2.push_back(*it);
	}

	return ret2;
}

