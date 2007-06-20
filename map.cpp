#include "map.h"

int readNormalNr (unsigned char * bufor, int pos, int bytCon = 4)
{
	int ret=0;
	int amp=1;
	for (int i=0; i<bytCon; i++)
	{
		ret+=bufor[pos+i]*amp;
		amp*=256;
	}
	return ret;
}

CMapHeader::CMapHeader(unsigned char *map)
{
	this->version = (Eformat)map[0]; //wersja mapy
	this->areAnyPLayers = map[4];
	this->height = this->width = map[5]; // wymiary mapy
	this->twoLevel = map[9]; //czy sa lochy
	
	int length = map[10]; //name length
	int i=14, pom; 
	while (i-14<length)	//read name
		this->name+=map[i++];
	length = map[i] + map[i+1]*256; //description length
	i+=4;
	for (pom=0;pom<length;pom++)
		this->description+=map[i++];
	this->difficulty = map[i++]; // reading map difficulty
	this->levelLimit = map[i++]; // hero level limit
	for (pom=0;pom<8;pom++)
	{
		this->players[pom].canHumanPlay = map[i++];
		this->players[pom].canComputerPlay = map[i++];
		if ((!(this->players[pom].canHumanPlay || this->players[pom].canComputerPlay)) || (!this->areAnyPLayers))
		{
			i+=13;
			continue;
		}

		this->players[pom].AITactic = map[i++];
		if (map[i++])
		{
			this->players[pom].allowedFactions = 0;
			this->players[pom].allowedFactions += map[i++];
			this->players[pom].allowedFactions += (map[i++])*256;
		}
		else 
		{
			this->players[pom].allowedFactions = 511;
			i+=2;
		}
		this->players[pom].isFactionRandom = map[i++];
		this->players[pom].hasMainTown = map[i++];
		if (this->players[pom].hasMainTown)
		{
			this->players[pom].generateHeroAtMainTown = map[i++];
			i++; //unknown byte
			this->players[pom].posOfMainTown.x = map[i++];
			this->players[pom].posOfMainTown.y = map[i++];
			this->players[pom].posOfMainTown.z = map[i++];
		}
		i++; //unknown byte
		int unknown = map[i++];
		if (unknown == 255)
		{
			this->players[pom].mainHeroPortrait = 255;
			i+=5;
			continue;
		}
		this->players[pom].mainHeroPortrait = map[i++];
		int nameLength = map[i++];
		i+=3; 
		for (int pp=0;pp<nameLength;pp++)
			this->players[pom].mainHeroName+=map[i++];
		i++; ////unknown byte
		int heroCount = map[i++];
		i+=3;
		for (int pp=0;pp<heroCount;pp++)
		{
			SheroName vv;
			vv.heroID=map[i++];
			int hnl = map[i++];
			i+=3;
			for (int zz=0;zz<hnl;zz++)
			{
				vv.heroName+=map[i++];
			}
			this->players[pom].heroesNames.push_back(vv);
		}
	}
	this->victoryCondition = (EvictoryConditions)map[i++];
	if (this->victoryCondition != winStandard) //specific victory conditions
	{
		int nr;
		switch (this->victoryCondition) //read victory conditions
		{
		case artifact:
			{
				this->vicConDetails = new VicCon0();
				((VicCon0*)this->vicConDetails)->ArtifactID = map[i+2];
				nr=2;
				break;
			}
		case gatherTroop:
			{
				this->vicConDetails = new VicCon1();
				int temp1 = map[i+2];
				int temp2 = map[i+3];
				((VicCon1*)this->vicConDetails)->monsterID = map[i+2];
				((VicCon1*)this->vicConDetails)->neededQuantity=readNormalNr(map, i+4);
				nr=6;
				break;
			}
		case gatherResource:
			{
				this->vicConDetails = new VicCon2();
				((VicCon2*)this->vicConDetails)->resourceID = map[i+2];
				((VicCon2*)this->vicConDetails)->neededQuantity=readNormalNr(map, i+3);
				nr=5;
				break;
			}
		case buildCity:
			{
				this->vicConDetails = new VicCon3();
				((VicCon3*)this->vicConDetails)->posOfCity.x = map[i+2];
				((VicCon3*)this->vicConDetails)->posOfCity.y = map[i+3];
				((VicCon3*)this->vicConDetails)->posOfCity.z = map[i+4];
				((VicCon3*)this->vicConDetails)->councilNeededLevel = map[i+5];
				((VicCon3*)this->vicConDetails)->fortNeededLevel = map[i+6];
				nr=5;
				break;
			}
		case buildGrail:
			{
				this->vicConDetails = new VicCon4();
				if (map[i+4]>2)
					((VicCon4*)this->vicConDetails)->anyLocation = true;
				else
				{
					((VicCon4*)this->vicConDetails)->whereBuildGrail.x = map[i+2];
					((VicCon4*)this->vicConDetails)->whereBuildGrail.y = map[i+3];
					((VicCon4*)this->vicConDetails)->whereBuildGrail.z = map[i+4];
				}
				nr=3;
				break;
			}
		case beatHero:
			{
				this->vicConDetails = new VicCon5();
				((VicCon5*)this->vicConDetails)->locationOfHero.x = map[i+2];
				((VicCon5*)this->vicConDetails)->locationOfHero.y = map[i+3];
				((VicCon5*)this->vicConDetails)->locationOfHero.z = map[i+4];				
				nr=3;
				break;
			}
		case captureCity:
			{
				this->vicConDetails = new VicCon6();
				((VicCon6*)this->vicConDetails)->locationOfTown.x = map[i+2];
				((VicCon6*)this->vicConDetails)->locationOfTown.y = map[i+3];
				((VicCon6*)this->vicConDetails)->locationOfTown.z = map[i+4];				
				nr=3;
				break;
			}
		case beatMonster:
			{
				this->vicConDetails = new VicCon7();
				((VicCon7*)this->vicConDetails)->locationOfMonster.x = map[i+2];
				((VicCon7*)this->vicConDetails)->locationOfMonster.y = map[i+3];
				((VicCon7*)this->vicConDetails)->locationOfMonster.z = map[i+4];				
				nr=3;
				break;
			}
		case takeDwellings:
			{		
				this->vicConDetails = new CspecificVictoryConidtions();
				nr=3;
				break;
			}
		case takeMines:
			{	
				this->vicConDetails = new CspecificVictoryConidtions();	
				nr=3;
				break;
			}
		case transportItem:
			{
				this->vicConDetails = new VicCona();
				((VicCona*)this->vicConDetails)->artifactID =  map[i+2];
				((VicCona*)this->vicConDetails)->destinationPlace.x = map[i+3];
				((VicCona*)this->vicConDetails)->destinationPlace.y = map[i+4];
				((VicCona*)this->vicConDetails)->destinationPlace.z = map[i+5];				
				nr=3;
				break;
			}
		}
		this->vicConDetails->allowNormalVictory = map[i++];
		this->vicConDetails->appliesToAI = map[i++];
		i+=nr;
	}
	this->lossCondition.typeOfLossCon = (ElossCon)map[i++];
	switch (this->lossCondition.typeOfLossCon) //read loss conditions
	{
	case lossCastle:
		  {
			  this->lossCondition.castlePos.x=map[i++];
			  this->lossCondition.castlePos.y=map[i++];
			  this->lossCondition.castlePos.z=map[i++];
		  }
	case lossHero:
		  {
			  this->lossCondition.heroPos.x=map[i++];
			  this->lossCondition.heroPos.y=map[i++];
			  this->lossCondition.heroPos.z=map[i++];
		  }
	case timeExpires:
		{
			this->lossCondition.timeLimit = readNormalNr(map, i++,2);
			i++;
		}
	}
	this->howManyTeams=map[i++]; //read number of teams
	if(this->howManyTeams>0) //read team numbers
	{
		for(int rr=0; rr<8; ++rr)
		{
			this->players[rr].team=map[i++];
		}
	}
}