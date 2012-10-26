#include "StdInc.h"
#include "map.h"

#include "Filesystem/CBinaryReader.h"
#include "Filesystem/CResourceLoader.h"
#include "Filesystem/CCompressedStream.h"
#include "CObjectHandler.h"
#include "CDefObjInfoHandler.h"
#include "VCMI_Lib.h"
#include <boost/crc.hpp>
#include "CArtHandler.h"
#include "CCreatureHandler.h"
#include "CSpellHandler.h"
#include "../lib/JsonNode.h"
#include "vcmi_endian.h"
#include "GameConstants.h"
#include "CStopWatch.h"

/*
 * map.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

static std::set<si32> convertBuildings(const std::set<si32> h3m, int castleID, bool addAuxiliary = true)
{
	std::map<int,int> mapa;
	std::set<si32> ret;

	// Note: this file is parsed many times.
	const JsonNode config(ResourceID("config/buildings5.json"));

	BOOST_FOREACH(const JsonNode &entry, config["table"].Vector())
	{
		int town = entry["town"].Float();

		if ( town == castleID || town == -1 )
			mapa[entry["h3"].Float()] = entry["vcmi"].Float();
	}

	for(std::set<si32>::const_iterator i=h3m.begin();i!=h3m.end();i++)
	{
		if(mapa[*i]>=0)
			ret.insert(mapa[*i]);
		else if(mapa[*i]  >=  (-GameConstants::CREATURES_PER_TOWN)) // horde buildings
		{
			int level = (mapa[*i]);
			ret.insert(level-30);//(-30)..(-36) - horde buildings (for game loading only), don't see other way to handle hordes in random towns
		}
		else
		{
			tlog3<<"Conversion warning: unknown building "<<*i<<" in castle "<<castleID<<std::endl;
		}
	}

	if(addAuxiliary)
		ret.insert(EBuilding::VILLAGE_HALL); //village hall is always present

	if(ret.find(EBuilding::CITY_HALL)!=ret.end())
		ret.insert(EBuilding::EXTRA_CITY_HALL);
	if(ret.find(EBuilding::TOWN_HALL)!=ret.end())
		ret.insert(EBuilding::EXTRA_TOWN_HALL);
	if(ret.find(EBuilding::CAPITOL)!=ret.end())
		ret.insert(EBuilding::EXTRA_CAPITOL);

	return ret;
}
static ui32 intPow(ui32 a, ui32 b)
{
	ui32 ret=1;
	for(ui32 i=0; i<b; ++i)
		ret*=a;
	return ret;
}
static ui8 reverse(ui8 arg)
{
	ui8 ret = 0;
	for (int i=0; i<8;i++)
	{
		if(((arg)&(1<<i))>>i)
		{
			ret |= (128>>i);
		}
	}
	return ret;
}

void readCreatureSet(CCreatureSet *out, const ui8 * buffer, int &i, int number, bool version) //version==true for >RoE maps
{
	const int bytesPerCre = version ? 4 : 3,
		maxID = version ? 0xffff : 0xff;

	for(int ir=0;ir < number; ir++)
	{
		int creID;
		int count;

		if (version)
		{
            creID = read_le_u16(buffer + i+ir*bytesPerCre);
            count = read_le_u16(buffer + i+ir*bytesPerCre + 2);
		}
		else
		{
            creID = buffer[i+ir*bytesPerCre];
            count = read_le_u16(buffer + i+ir*bytesPerCre + 1);
		}

		if(creID == maxID) //empty slot
			continue;

		CStackInstance *hlp = new CStackInstance();
		hlp->count = count;

		if(creID > maxID - 0xf)
		{
			creID = maxID + 1 - creID + VLC->creh->creatures.size();//this will happen when random object has random army
			hlp->idRand = creID;
		}
		else
			hlp->setType(creID);

		out->putStack(ir, hlp);
	}
	i+=number*bytesPerCre;
	
	out->validTypes(true);
}

PlayerInfo::PlayerInfo(): p7(0), p8(0), p9(0), canHumanPlay(0), canComputerPlay(0),
    AITactic(0), isFactionRandom(0),
    mainHeroPortrait(0), hasMainTown(0), generateHeroAtMainTown(0),
    team(255), generateHero(0)
{

}

si8 PlayerInfo::defaultCastle() const
{
    assert(!allowedFactions.empty()); // impossible?

    if(allowedFactions.size() == 1)
    {
        // only one faction is available - pick it
        return *allowedFactions.begin();
    }

    // set to random
    return -1;
}

si8 PlayerInfo::defaultHero() const
{
    // we will generate hero in front of main town
    if((generateHeroAtMainTown && hasMainTown) || p8)
    {
        //random hero
        return -1;
    }

    return -2;
}

CMapHeader::CMapHeader(const ui8 *map)
{
	int i=0;
	initFromMemory(map,i);
}

CMapHeader::CMapHeader()
{
	areAnyPLayers = difficulty = levelLimit = howManyTeams = 0;
	height = width = twoLevel = -1;
}

void CMapHeader::initFromMemory( const ui8 *buffer, int &i )
{
    version = (EMapFormat::EMapFormat)(read_le_u32(buffer + i)); i+=4; //map version
    if(version != EMapFormat::ROE && version != EMapFormat::AB && version != EMapFormat::SOD
            && version != EMapFormat::WOG)
	{
		throw std::runtime_error("Invalid map format!");
	}
    areAnyPLayers = readChar(buffer,i); //invalid on some maps
    height = width = (read_le_u32(buffer + i)); i+=4; // dimensions of map
    twoLevel = readChar(buffer,i); //if there is underground
	int pom;
    name = readString(buffer,i);
    description= readString(buffer,i);
    difficulty = readChar(buffer,i); // reading map difficulty
    if(version != EMapFormat::ROE)
        levelLimit = readChar(buffer,i); // hero level limit
	else
		levelLimit = 0;
    loadPlayerInfo(pom, buffer, i);
    loadViCLossConditions(buffer, i);

    howManyTeams=buffer[i++]; //read number of teams
	if(howManyTeams>0) //read team numbers
	{
		for(int rr=0; rr<8; ++rr)
		{
            players[rr].team = buffer[i++];
		}
	}
	else//no alliances
	{
		for(int i=0;i<GameConstants::PLAYER_LIMIT;i++)
			if(players[i].canComputerPlay || players[i].canHumanPlay)
				players[i].team = howManyTeams++;
	}


	pom = i;
	allowedHeroes.resize(GameConstants::HEROES_QUANTITY,false);
    for(; i<pom+ (version == EMapFormat::ROE ? 16 : 20) ; ++i)
	{
        ui8 c = buffer[i];
		for(int yy=0; yy<8; ++yy)
			if((i-pom)*8+yy < GameConstants::HEROES_QUANTITY)
				if(c == (c|((ui8)intPow(2, yy))))
					allowedHeroes[(i-pom)*8+yy] = true;
	}
    if(version > EMapFormat::ROE) //probably reserved for further heroes
	{
        int placeholdersQty = read_le_u32(buffer + i); i+=4;
		for(int p = 0; p < placeholdersQty; p++)
            placeholdedHeroes.push_back(buffer[i++]);
	}
}
void CMapHeader::loadPlayerInfo( int &pom, const ui8 * buffer, int &i )
{
	players.resize(8);
	for (pom=0;pom<8;pom++)
	{
        players[pom].canHumanPlay = buffer[i++];
        players[pom].canComputerPlay = buffer[i++];
		if ((!(players[pom].canHumanPlay || players[pom].canComputerPlay))) //if nobody can play with this player
		{
			switch(version)
			{
            case EMapFormat::SOD:
            case EMapFormat::WOG:
				i+=13;
				break;
            case EMapFormat::AB:
				i+=12;
				break;
            case EMapFormat::ROE:
				i+=6;
				break;
			}
			continue;
		}

        players[pom].AITactic = buffer[i++];

        if(version == EMapFormat::SOD || version == EMapFormat::WOG)
            players[pom].p7= buffer[i++];
		else
			players[pom].p7= -1;

		//factions this player can choose
        ui16 allowedFactions = buffer[i++];
        if(version != EMapFormat::ROE)
            allowedFactions += (buffer[i++])*256;

		for (size_t fact=0; fact<16; fact++)
			if (allowedFactions & (1 << fact))
				players[pom].allowedFactions.insert(fact);

        players[pom].isFactionRandom = buffer[i++];
        players[pom].hasMainTown = buffer[i++];
		if (players[pom].hasMainTown)
		{
            if(version != EMapFormat::ROE)
			{
                players[pom].generateHeroAtMainTown = buffer[i++];
                players[pom].generateHero = buffer[i++];
			}
			else
			{
				players[pom].generateHeroAtMainTown = true;
				players[pom].generateHero = false;
			}

            players[pom].posOfMainTown.x = buffer[i++];
            players[pom].posOfMainTown.y = buffer[i++];
            players[pom].posOfMainTown.z = buffer[i++];


		}
        players[pom].p8= buffer[i++];
        players[pom].p9= buffer[i++];
		if(players[pom].p9!=0xff)
		{
            players[pom].mainHeroPortrait = buffer[i++];
            players[pom].mainHeroName = readString(buffer,i);
		}

        if(version != EMapFormat::ROE)
		{
            players[pom].powerPlacehodlers = buffer[i++];//unknown byte
            int heroCount = buffer[i++];
			i+=3;
			for (int pp=0;pp<heroCount;pp++)
			{
				SheroName vv;
                vv.heroID=buffer[i++];
                int hnl = buffer[i++];
				i+=3;
				for (int zz=0;zz<hnl;zz++)
				{
                    vv.heroName+=buffer[i++];
				}
				players[pom].heroesNames.push_back(vv);
			}
		}
	}
}

void CMapHeader::loadViCLossConditions( const ui8 * buffer, int &i)
{
	victoryCondition.obj = NULL;
    victoryCondition.condition = (EVictoryConditionType::EVictoryConditionType)buffer[i++];
	if (victoryCondition.condition != EVictoryConditionType::WINSTANDARD) //specific victory conditions
	{
		int nr=0;
		switch (victoryCondition.condition) //read victory conditions
		{
		case EVictoryConditionType::ARTIFACT:
			{
                victoryCondition.ID = buffer[i+2];
                nr=(version == EMapFormat::ROE ? 1 : 2);
				break;
			}
		case EVictoryConditionType::GATHERTROOP:
			{
// 				int temp1 = buffer[i+2];
// 				int temp2 = buffer[i+3];
                victoryCondition.ID = buffer[i+2];
                victoryCondition.count = read_le_u32(buffer + i+(version == EMapFormat::ROE ? 3 : 4));
                nr=(version == EMapFormat::ROE ? 5 : 6);
				break;
			}
		case EVictoryConditionType::GATHERRESOURCE:
			{
                victoryCondition.ID = buffer[i+2];
                victoryCondition.count = read_le_u32(buffer + i+3);
				nr = 5;
				break;
			}
		case EVictoryConditionType::BUILDCITY:
			{
                victoryCondition.pos.x = buffer[i+2];
                victoryCondition.pos.y = buffer[i+3];
                victoryCondition.pos.z = buffer[i+4];
                victoryCondition.count = buffer[i+5];
                victoryCondition.ID = buffer[i+6];
				nr = 5;
				break;
			}
		case EVictoryConditionType::BUILDGRAIL:
			{
                if (buffer[i+4]>2)
					victoryCondition.pos = int3(-1,-1,-1);
				else
				{
                    victoryCondition.pos.x = buffer[i+2];
                    victoryCondition.pos.y = buffer[i+3];
                    victoryCondition.pos.z = buffer[i+4];
				}
				nr = 3;
				break;
			}
		case EVictoryConditionType::BEATHERO:
		case EVictoryConditionType::CAPTURECITY:
		case EVictoryConditionType::BEATMONSTER:
			{
                victoryCondition.pos.x = buffer[i+2];
                victoryCondition.pos.y = buffer[i+3];
                victoryCondition.pos.z = buffer[i+4];
				nr = 3;
				break;
			}
		case EVictoryConditionType::TAKEDWELLINGS:
		case EVictoryConditionType::TAKEMINES:
			{
				nr = 0;
				break;
			}
		case EVictoryConditionType::TRANSPORTITEM:
			{
                victoryCondition.ID =  buffer[i+2];
                victoryCondition.pos.x = buffer[i+3];
                victoryCondition.pos.y = buffer[i+4];
                victoryCondition.pos.z = buffer[i+5];
				nr = 4;
				break;
			}
		default:
			assert(0);
		}
        victoryCondition.allowNormalVictory = buffer[i++];
        victoryCondition.appliesToAI = buffer[i++];
		i+=nr;
	}
    lossCondition.typeOfLossCon = (ELossConditionType::ELossConditionType)buffer[i++];
	switch (lossCondition.typeOfLossCon) //read loss conditions
	{
	case ELossConditionType::LOSSCASTLE:
	case ELossConditionType::LOSSHERO:
		{
            lossCondition.pos.x=buffer[i++];
            lossCondition.pos.y=buffer[i++];
            lossCondition.pos.z=buffer[i++];
			break;
		}
	case ELossConditionType::TIMEEXPIRES:
		{
            lossCondition.timeLimit = read_le_u16(buffer + i);
			i+=2;
			break;
		}
	}
}

CMapHeader::~CMapHeader()
{

}

void CMap::initFromBytes(const ui8 * buffer, size_t size)
{
	// Compute checksum
	boost::crc_32_type  result;
    result.process_bytes(buffer, size);
	checksum = result.checksum();
	tlog0 << "\tOur map checksum: "<<result.checksum() << std::endl;

	int i=0;
    CMapHeader::initFromMemory(buffer,i);
	CStopWatch th;
	th.getDiff();
    readHeader(buffer, i);
	tlog0<<"\tReading header: "<<th.getDiff()<<std::endl;

	if (victoryCondition.condition == EVictoryConditionType::ARTIFACT || victoryCondition.condition == EVictoryConditionType::TRANSPORTITEM)
	{ //messy, but needed
		allowedArtifact[victoryCondition.ID] = false;
	}

    readRumors(buffer, i);
	tlog0<<"\tReading rumors: "<<th.getDiff()<<std::endl;

    readPredefinedHeroes(buffer, i);
	tlog0<<"\tReading predefined heroes: "<<th.getDiff()<<std::endl;

    readTerrain(buffer, i);
	tlog0<<"\tReading terrain: "<<th.getDiff()<<std::endl;

    readDefInfo(buffer, i);
	tlog0<<"\tReading defs info: "<<th.getDiff()<<std::endl;

    readObjects(buffer, i);
	tlog0<<"\tReading objects: "<<th.getDiff()<<std::endl;
	
    readEvents(buffer, i);
	tlog0<<"\tReading events: "<<th.getDiff()<<std::endl;

    //map readed, buffer no longer needed
	for(ui32 f=0; f<objects.size(); ++f) //calculationg blocked / visitable positions
	{
		if(!objects[f]->defInfo)
			continue;
		addBlockVisTiles(objects[f]);
	}
	tlog0<<"\tCalculating blocked/visitable tiles: "<<th.getDiff()<<std::endl;
	tlog0 << "\tMap initialization done!" << std::endl;
}

void CMap::removeBlockVisTiles(CGObjectInstance * obj, bool total)
{
	for(int fx=0; fx<8; ++fx)
	{
		for(int fy=0; fy<6; ++fy)
		{
			int xVal = obj->pos.x + fx - 7;
			int yVal = obj->pos.y + fy - 5;
			int zVal = obj->pos.z;
			if(xVal>=0 && xVal<width && yVal>=0 && yVal<height)
			{
				TerrainTile & curt = terrain[xVal][yVal][zVal];
				if(total || ((obj->defInfo->visitMap[fy] >> (7 - fx)) & 1))
				{
					curt.visitableObjects -= obj;
					curt.visitable = curt.visitableObjects.size();
				}
				if(total || !((obj->defInfo->blockMap[fy] >> (7 - fx)) & 1))
				{
					curt.blockingObjects -= obj;
					curt.blocked = curt.blockingObjects.size();
				}
			}
		}
	}
}
void CMap::addBlockVisTiles(CGObjectInstance * obj)
{
	for(int fx=0; fx<8; ++fx)
	{
		for(int fy=0; fy<6; ++fy)
		{
			int xVal = obj->pos.x + fx - 7;
			int yVal = obj->pos.y + fy - 5;
			int zVal = obj->pos.z;
			if(xVal>=0 && xVal<width && yVal>=0 && yVal<height)
			{
				TerrainTile & curt = terrain[xVal][yVal][zVal];
				if(((obj->defInfo->visitMap[fy] >> (7 - fx)) & 1))
				{
					curt.visitableObjects.push_back(obj);
					curt.visitable = true;
				}
				if(!((obj->defInfo->blockMap[fy] >> (7 - fx)) & 1))
				{
					curt.blockingObjects.push_back(obj);
					curt.blocked = true;
				}
			}
		}
	}
}

TInputStreamPtr CMap::getMapStream(std::string name)
{
    TInputStreamPtr file = CResourceHandler::get()->load(ResourceID(name, EResType::MAP));

	CBinaryReader reader(*file.get());

	ui32 header = reader.readUInt32();
	file->seek(0); //reset file

	switch (header & 0xffffff) // gzip header is 3 bytes only in size
	{
		case 0x00088B1F: // gzip header magic number, reversed for LE
			return TInputStreamPtr(new CCompressedStream(std::move(file), true));
        case EMapFormat::WOG :
        case EMapFormat::AB  :
        case EMapFormat::ROE :
        case EMapFormat::SOD :
			return file;
		default :
            tlog0 << "Error: Failed to open map " << name << ": unknown format\n";
			return TInputStreamPtr();
	}
}

CMap::CMap(std::string filename)
    : grailPos(-1, -1, -1), grailRadious(0)
{
	tlog0<<"Opening map file: "<<filename<<"\t "<<std::flush;
	
	TInputStreamPtr file = getMapStream(filename);

	//load file and decompress
	size_t mapSize = file->getSize();
	std::unique_ptr<ui8[]>  data(new ui8 [mapSize] );
	file->read(data.get(), mapSize);

	tlog0<<"done."<<std::endl;

	initFromBytes(data.get(), mapSize);
}

CMap::CMap()
{
	terrain = NULL;

}
CMap::~CMap()
{
    //for(int i=0; i < customDefs.size(); i++)
    //	if(customDefs[i]->serial < 0) //def not present in the main vector in defobjinfo
    //		delete customDefs[i];

	if(terrain)
	{
        for(int ii=0;ii<width;ii++)
		{
			for(int jj=0;jj<height;jj++)
				delete [] terrain[ii][jj];
			delete [] terrain[ii];
		}
		delete [] terrain;
	}
	for(std::list<ConstTransitivePtr<CMapEvent> >::iterator i = events.begin(); i != events.end(); i++)
    {
		i->dellNull();
    }
}

CGHeroInstance * CMap::getHero(int heroID)
{
	for(ui32 i=0; i<heroes.size();i++)
        if(heroes[i]->subID == heroID)
			return heroes[i];
    return nullptr;
}

int CMap::loadSeerHut(const ui8 * buffer, int i, CGObjectInstance * & seerHut)
{
	CGSeerHut *hut = new CGSeerHut();
    seerHut = hut;

    if(version > EMapFormat::ROE)
	{
        loadQuest(hut,buffer,i);
	}
	else //RoE
	{
        int artID = buffer[i]; ++i;
		if (artID != 255) //not none quest
		{
			hut->quest->m5arts.push_back (artID);
			hut->quest->missionType = CQuest::MISSION_ART;
		}
		else
		{
			hut->quest->missionType = CQuest::MISSION_NONE; //no mission
		}
		hut->quest->lastDay = -1; //no timeout
		hut->quest->isCustomFirst = hut->quest->isCustomNext = hut->quest->isCustomComplete = false;
	}

	if (hut->quest->missionType)
	{
        ui8 rewardType = buffer[i]; ++i;
		hut->rewardType = rewardType;

		switch(rewardType)
		{
		case 1:
			{
                hut->rVal = read_le_u32(buffer + i); i+=4;
				break;
			}
		case 2:
			{
                hut->rVal = read_le_u32(buffer + i); i+=4;
				break;
			}
		case 3:
			{
                hut->rVal = buffer[i]; ++i;
				break;
			}
		case 4:
			{
                hut->rVal = buffer[i]; ++i;
				break;
			}
		case 5:
			{
                hut->rID = buffer[i]; ++i;
				/* Only the first 3 bytes are used. Skip the 4th. */
                hut->rVal = read_le_u32(buffer + i) & 0x00ffffff;
				i+=4;
				break;
			}
		case 6:
			{
                hut->rID = buffer[i]; ++i;
                hut->rVal = buffer[i]; ++i;
				break;
			}
		case 7:
			{
                hut->rID = buffer[i]; ++i;
                hut->rVal = buffer[i]; ++i;
				break;
			}
		case 8:
			{
                if (version == EMapFormat::ROE)
				{
                    hut->rID = buffer[i]; i++;
				}
				else
				{
                    hut->rID = read_le_u16(buffer + i); i+=2;
				}
				break;
			}
		case 9:
			{
                hut->rID = buffer[i]; ++i;
				break;
			}
		case 10:
			{
                if(version > EMapFormat::ROE)
				{
                    hut->rID = read_le_u16(buffer + i); i+=2;
                    hut->rVal = read_le_u16(buffer + i); i+=2;
				}
				else
				{
                    hut->rID = buffer[i]; ++i;
                    hut->rVal = read_le_u16(buffer + i); i+=2;
				}
				break;
			}
		}// end of internal switch
		i+=2;
	}
	else //missionType==255
	{
		i+=3;
	}
	return i;
}

void CMap::loadTown(CGObjectInstance * & town, const ui8 * buffer, int &i, int castleID)
{
	CGTownInstance * nt = new CGTownInstance();
    town = nt;
	nt->identifier = 0;
    if(version > EMapFormat::ROE)
	{	
        nt->identifier = read_le_u32(buffer + i); i+=4;
	}
    nt->tempOwner = buffer[i]; ++i;
    if(readChar(buffer,i)) //has name
        nt->name = readString(buffer,i);
    if(readChar(buffer,i))//true if garrison isn't empty
        readCreatureSet(nt, buffer, i, 7, version > EMapFormat::ROE);
    nt->formation = buffer[i]; ++i;
    if(readChar(buffer,i)) //custom buildings info
	{
		//built buildings
		for(int byte=0;byte<6;byte++)
		{
			for(int bit=0;bit<8;bit++)
                if(buffer[i] & (1<<bit))
					nt->builtBuildings.insert(byte*8+bit);
			i++;
		}
		//forbidden buildings
		for(int byte=6;byte<12;byte++)
		{
			for(int bit=0;bit<8;bit++)
                if(buffer[i] & (1<<bit))
					nt->forbiddenBuildings.insert((byte-6)*8+bit);
			i++;
		}
        nt->builtBuildings = convertBuildings(nt->builtBuildings,castleID);
        nt->forbiddenBuildings = convertBuildings(nt->forbiddenBuildings,castleID);
	}
	else //standard buildings
	{
        if(readChar(buffer,i)) //has fort
			nt->builtBuildings.insert(EBuilding::FORT);
		nt->builtBuildings.insert(-50); //means that set of standard building should be included
	}

	int ist = i;
    if(version > EMapFormat::ROE)
	{
		for(; i<ist+9; ++i)
		{
            ui8 c = buffer[i];
			for(int yy=0; yy<8; ++yy)
			{
				if((i-ist)*8+yy < GameConstants::SPELLS_QUANTITY)
				{
					if(c == (c|((ui8)intPow(2, yy))))
						nt->obligatorySpells.push_back((i-ist)*8+yy);
				}
			}
		}
	}

	ist = i;
	for(; i<ist+9; ++i)
	{
        ui8 c = buffer[i];
		for(int yy=0; yy<8; ++yy)
		{
			if((i-ist)*8+yy < GameConstants::SPELLS_QUANTITY)
			{
				if(c != (c|((ui8)intPow(2, yy))))
					nt->possibleSpells.push_back((i-ist)*8+yy);
			}
		}
	}

	/////// reading castle events //////////////////////////////////

    int numberOfEvent = read_le_u32(buffer + i); i+=4;

	for(int gh = 0; gh<numberOfEvent; ++gh)
	{
		CCastleEvent *nce = new CCastleEvent();
		nce->town = nt;
        nce->name = readString(buffer,i);
        nce->message = readString(buffer,i);
		for(int x=0; x < 7; x++)
		{
            nce->resources[x] = read_le_u32(buffer + i);
			i+=4;
		}

        nce->players = buffer[i]; ++i;
        if(version > EMapFormat::AB)
		{
            nce->humanAffected = buffer[i]; ++i;
		}
		else
			nce->humanAffected = true;

        nce->computerAffected = buffer[i]; ++i;
        nce->firstOccurence = read_le_u16(buffer + i); i+=2;
        nce->nextOccurence = buffer[i]; ++i;

		i+=17;

		//new buildings
		for(int byte=0;byte<6;byte++)
		{
			for(int bit=0;bit<8;bit++)
                if(buffer[i] & (1<<bit))
					nce->buildings.insert(byte*8+bit);
			i++;
		}
        nce->buildings = convertBuildings(nce->buildings,castleID, false);
		
		nce->creatures.resize(7);
		for(int vv=0; vv<7; ++vv)
		{
            nce->creatures[vv] = read_le_u16(buffer + i);i+=2;
		}
		i+=4;
		nt->events.push_back(nce);
	}//castle events have been read 

    if(version > EMapFormat::AB)
	{
        nt->alignment = buffer[i]; ++i;
	}
	else
		nt->alignment = 0xff;
	i+=3;

	nt->builded = 0;
	nt->destroyed = 0;
	nt->garrisonHero = NULL;
}

CGObjectInstance * CMap::loadHero(const ui8 * buffer, int &i, int idToBeGiven)
{
	CGHeroInstance * nhi = new CGHeroInstance();

	int identifier = 0;
    if(version > EMapFormat::ROE)
	{
        identifier = read_le_u32(buffer + i); i+=4;
		questIdentifierToId[identifier] = idToBeGiven;
	}

    ui8 owner = buffer[i++];
    nhi->subID = buffer[i++];
	
	for(ui32 j=0; j<predefinedHeroes.size(); j++)
	{
		if(predefinedHeroes[j]->subID == nhi->subID)
		{
			tlog0 << "Hero " << nhi->subID << " will be taken from the predefined heroes list.\n";
			delete nhi;
			nhi = predefinedHeroes[j];
			break;
		}
	}
	nhi->setOwner(owner);

	nhi->portrait = nhi->subID;

	for(ui32 j=0; j<disposedHeroes.size(); j++)
	{
		if(disposedHeroes[j].ID == nhi->subID)
		{
			nhi->name = disposedHeroes[j].name;
			nhi->portrait = disposedHeroes[j].portrait;
			break;
		}
	}
    if(readChar(buffer,i))//true if hero has nonstandard name
        nhi->name = readString(buffer,i);
    if(version > EMapFormat::AB)
	{
        if(readChar(buffer,i))//true if hero's experience is greater than 0
        {	nhi->exp = read_le_u32(buffer + i); i+=4;	}
		else
			nhi->exp = 0xffffffff;
	}
	else
	{	
        nhi->exp = read_le_u32(buffer + i); i+=4;
		if(!nhi->exp) //0 means "not set" in <=AB maps
			nhi->exp = 0xffffffff;
	}

    bool portrait=buffer[i]; ++i;
	if (portrait)
        nhi->portrait = buffer[i++];
    if(readChar(buffer,i))//true if hero has specified abilities
	{
        int howMany = read_le_u32(buffer + i); i+=4;
		nhi->secSkills.resize(howMany);
		for(int yy=0; yy<howMany; ++yy)
		{
            nhi->secSkills[yy].first = buffer[i++];
            nhi->secSkills[yy].second = buffer[i++];
		}
	}
    if(readChar(buffer,i))//true if hero has nonstandard garrison
        readCreatureSet(nhi, buffer, i, 7, version > EMapFormat::ROE);

    nhi->formation =buffer[i]; ++i; //formation
    loadArtifactsOfHero(buffer, i, nhi);
    nhi->patrol.patrolRadious = buffer[i]; ++i;
	if(nhi->patrol.patrolRadious == 0xff)
		nhi->patrol.patrolling = false;
	else 
		nhi->patrol.patrolling = true;

    if(version > EMapFormat::ROE)
	{
        if(readChar(buffer,i))//true if hero has nonstandard (mapmaker defined) biography
            nhi->biography = readString(buffer,i);
        nhi->sex = buffer[i]; ++i;
		
		if (nhi->sex != 0xFF)//remove trash
			nhi->sex &=1;
	}
	else
		nhi->sex = 0xFF;
	//spells
    if(version > EMapFormat::AB)
	{
        bool areSpells = buffer[i]; ++i;

		if(areSpells) //TODO: sprawdzi //seems to be ok - tow
		{
			nhi->spells.insert(0xffffffff); //placeholder "preset spells"
			int ist = i;
			for(; i<ist+9; ++i)
			{
                ui8 c = buffer[i];
				for(int yy=0; yy<8; ++yy)
				{
					if((i-ist)*8+yy < GameConstants::SPELLS_QUANTITY)
					{
						if(c == (c|((ui8)intPow(2, yy))))
							nhi->spells.insert((i-ist)*8+yy);
					}
				}
			}
		}
	}
    else if(version == EMapFormat::AB) //we can read one spell
	{
        ui8 buff = buffer[i]; ++i;
		if(buff != 254)
		{
			nhi->spells.insert(0xffffffff); //placeholder "preset spells"
			if(buff < 254) //255 means no spells
				nhi->spells.insert(buff);
		}
	}
	//spells loaded
    if(version > EMapFormat::AB)
	{
        if(readChar(buffer,i))//customPrimSkills
		{
			for(int xx=0; xx<GameConstants::PRIMARY_SKILLS; xx++)
                nhi->pushPrimSkill(xx, buffer[i++]);
		}
	}
	i+=16;

	return nhi;
}

void CMap::readRumors( const ui8 * buffer, int &i)
{
    int rumNr = read_le_u32(buffer + i);i+=4;
	for (int it=0;it<rumNr;it++)
	{
		Rumor ourRumor;
        int nameL = read_le_u32(buffer + i);i+=4; //read length of name of rumor
		for (int zz=0; zz<nameL; zz++)
            ourRumor.name+=buffer[i++];
        nameL = read_le_u32(buffer + i);i+=4; //read length of rumor
		for (int zz=0; zz<nameL; zz++)
            ourRumor.text+=buffer[i++];
		rumors.push_back(ourRumor); //add to our list
	}
}

void CMap::readHeader( const ui8 * buffer, int &i)
{
	//reading allowed heroes (20 bytes)
	int ist;
	ui8 disp = 0;

    if(version >= EMapFormat::SOD)
	{
        disp = buffer[i++];
		disposedHeroes.resize(disp);
		for(int g=0; g<disp; ++g)
		{
            disposedHeroes[g].ID = buffer[i++];
            disposedHeroes[g].portrait = buffer[i++];
            int lenbuf = read_le_u32(buffer + i); i+=4;
			for (int zz=0; zz<lenbuf; zz++)
                disposedHeroes[g].name+=buffer[i++];
            disposedHeroes[g].players = buffer[i++];
		}
	}

	i+=31; //omitting NULLS

	allowedArtifact.resize(GameConstants::ARTIFACTS_QUANTITY);
	for (ui32 x=0; x<allowedArtifact.size(); x++)
		allowedArtifact[x] = true;

	//reading allowed artifacts:  17 or 18 bytes
    if (version != EMapFormat::ROE)
	{
		ist=i; //starting i for loop
        for (; i<ist+(version == EMapFormat::AB ? 17 : 18); ++i)
		{
            ui8 c = buffer[i];
			for (int yy=0; yy<8; ++yy)
			{
				if ((i-ist)*8+yy < GameConstants::ARTIFACTS_QUANTITY)
				{
					if (c == (c|((ui8)intPow(2, yy))))
						allowedArtifact[(i-ist)*8+yy] = false;
				}
			}
		}//allowed artifacts have been read
	}
    if (version == EMapFormat::ROE || version == EMapFormat::AB) //ban combo artifacts
	{
		BOOST_FOREACH(CArtifact *artifact, VLC->arth->artifacts) 
		{
			if (artifact->constituents) //combo
			{
				allowedArtifact[artifact->id] = false;
			}
		}
        if (version == EMapFormat::ROE)
			allowedArtifact[128] = false; //Armageddon's Blade
	}

	allowedSpell.resize(GameConstants::SPELLS_QUANTITY);
	for(ui32 x=0;x<allowedSpell.size();x++)
		allowedSpell[x] = true;

	allowedAbilities.resize(GameConstants::SKILL_QUANTITY);
	for(ui32 x=0;x<allowedAbilities.size();x++)
		allowedAbilities[x] = true;

    if(version >= EMapFormat::SOD)
	{
		//reading allowed spells (9 bytes)
		ist=i; //starting i for loop
		for(; i<ist+9; ++i)
		{
            ui8 c = buffer[i];
			for(int yy=0; yy<8; ++yy)
				if((i-ist)*8+yy < GameConstants::SPELLS_QUANTITY)
					if(c == (c|((ui8)intPow(2, yy))))
						allowedSpell[(i-ist)*8+yy] = false;
		}


		//allowed hero's abilities (4 bytes)
		ist=i; //starting i for loop
		for(; i<ist+4; ++i)
		{
            ui8 c = buffer[i];
			for(int yy=0; yy<8; ++yy)
			{
				if((i-ist)*8+yy < GameConstants::SKILL_QUANTITY)
				{
					if(c == (c|((ui8)intPow(2, yy))))
						allowedAbilities[(i-ist)*8+yy] = false;
				}
			}
		}
	}
}

void CMap::readPredefinedHeroes( const ui8 * buffer, int &i)
{
	switch(version)
	{
    case EMapFormat::WOG:
    case EMapFormat::SOD:
		{
			for(int z=0;z<GameConstants::HEROES_QUANTITY;z++) //disposed heroes
			{
                int custom =  buffer[i++];
				if(!custom)
					continue;
				CGHeroInstance * cgh = new CGHeroInstance;
				cgh->ID = Obj::HERO;
				cgh->subID = z;
                if(readChar(buffer,i))//true if hore's experience is greater than 0
                {	cgh->exp = read_le_u32(buffer + i); i+=4;	}
				else
					cgh->exp = 0;
                if(readChar(buffer,i))//true if hero has specified abilities
				{
                    int howMany = read_le_u32(buffer + i); i+=4;
					cgh->secSkills.resize(howMany);
					for(int yy=0; yy<howMany; ++yy)
					{
                        cgh->secSkills[yy].first = buffer[i]; ++i;
                        cgh->secSkills[yy].second = buffer[i]; ++i;
					}
				}

                loadArtifactsOfHero(buffer, i, cgh);

                if(readChar(buffer,i))//customBio
                    cgh->biography = readString(buffer,i);
                cgh->sex = buffer[i++]; // 0xFF is default, 00 male, 01 female
                if(readChar(buffer,i))//are spells
				{
					int ist = i;
					for(; i<ist+9; ++i)
					{
                        ui8 c = buffer[i];
						for(int yy=0; yy<8; ++yy)
						{
							if((i-ist)*8+yy < GameConstants::SPELLS_QUANTITY)
							{
								if(c == (c|((ui8)intPow(2, yy))))
									cgh->spells.insert((i-ist)*8+yy);
							}
						}
					}
				}
                if(readChar(buffer,i))//customPrimSkills
				{
					for(int xx=0; xx<GameConstants::PRIMARY_SKILLS; xx++)
                        cgh->pushPrimSkill(xx, buffer[i++]);
				}
				predefinedHeroes.push_back(cgh);
			}
			break;
		}
    case EMapFormat::ROE:
		i+=0;
		break;
	}
}

void CMap::readTerrain( const ui8 * buffer, int &i)
{
	terrain = new TerrainTile**[width]; // allocate memory 
	for (int ii=0;ii<width;ii++)
	{
		terrain[ii] = new TerrainTile*[height]; // allocate memory 
		for(int jj=0;jj<height;jj++)
			terrain[ii][jj] = new TerrainTile[twoLevel+1];
	}

	for (int c=0; c<width; c++) // reading terrain
	{
		for (int z=0; z<height; z++)
		{
            terrain[z][c][0].tertype = static_cast<ETerrainType::ETerrainType>(buffer[i++]);
            terrain[z][c][0].terview = buffer[i++];
            terrain[z][c][0].riverType = static_cast<ERiverType::ERiverType>(buffer[i++]);
            terrain[z][c][0].riverDir = buffer[i++];
            terrain[z][c][0].roadType = static_cast<ERoadType::ERoadType>(buffer[i++]);
            terrain[z][c][0].roadDir = buffer[i++];
            terrain[z][c][0].extTileFlags = buffer[i++];
            terrain[z][c][0].blocked = (terrain[z][c][0].tertype == ETerrainType::ROCK ? 1 : 0); //underground tiles are always blocked
			terrain[z][c][0].visitable = 0;
		}
	}
	if (twoLevel) // read underground terrain
	{
		for (int c=0; c<width; c++) // reading terrain
		{
			for (int z=0; z<height; z++)
			{
                terrain[z][c][1].tertype = static_cast<ETerrainType::ETerrainType>(buffer[i++]);
                terrain[z][c][1].terview = buffer[i++];
                terrain[z][c][1].riverType = static_cast<ERiverType::ERiverType>(buffer[i++]);
                terrain[z][c][1].riverDir = buffer[i++];
                terrain[z][c][1].roadType = static_cast<ERoadType::ERoadType>(buffer[i++]);
                terrain[z][c][1].roadDir = buffer[i++];
                terrain[z][c][1].extTileFlags = buffer[i++];
                terrain[z][c][1].blocked = (terrain[z][c][1].tertype == ETerrainType::ROCK ? 1 : 0); //underground tiles are always blocked
				terrain[z][c][1].visitable = 0;
			}
		}
	}
}

void CMap::readDefInfo( const ui8 * buffer, int &i)
{
    int defAmount = read_le_u32(buffer + i); i+=4;
    customDefs.reserve(defAmount+8);
	for (int idd = 0 ; idd<defAmount; idd++) // reading defs
	{
		CGDefInfo * vinya = new CGDefInfo(); // info about new def 

		//reading name
        int nameLength = read_le_u32(buffer + i);i+=4;
		vinya->name.reserve(nameLength);
		for (int cd=0;cd<nameLength;cd++)
		{
            vinya->name += buffer[i++];
		}
		std::transform(vinya->name.begin(),vinya->name.end(),vinya->name.begin(),(int(*)(int))toupper);


		ui8 bytes[12];
		for (int v=0; v<12; v++) // read info
		{
            bytes[v] = buffer[i++];
		}
        vinya->terrainAllowed = read_le_u16(buffer + i);i+=2;
        vinya->terrainMenu = read_le_u16(buffer + i);i+=2;
        vinya->id = read_le_u32(buffer + i);i+=4;
        vinya->subid = read_le_u32(buffer + i);i+=4;
        vinya->type = buffer[i++];
        vinya->printPriority = buffer[i++];
		for (int zi=0; zi<6; zi++)
		{
			vinya->blockMap[zi] = reverse(bytes[zi]);
		}
		for (int zi=0; zi<6; zi++)
		{
			vinya->visitMap[zi] = reverse(bytes[6+zi]);
		}
		i+=16;
		if(vinya->id!=Obj::HERO && vinya->id!=70)
		{
			CGDefInfo *h = VLC->dobjinfo->gobjs[vinya->id][vinya->subid];
			if(!h) 
			{
				//remove fake entry
				VLC->dobjinfo->gobjs[vinya->id].erase(vinya->subid);
				if(VLC->dobjinfo->gobjs[vinya->id].size())
					VLC->dobjinfo->gobjs.erase(vinya->id);
				tlog2<<"\t\tWarning: no defobjinfo entry for object ID="<<vinya->id<<" subID=" << vinya->subid<<std::endl;
			}
			else
			{
				vinya->visitDir = VLC->dobjinfo->gobjs[vinya->id][vinya->subid]->visitDir;
			}
		}
		else
		{
			vinya->visitDir = 0xff;
		}

		if(vinya->id == Obj::EVENT)
			std::memset(vinya->blockMap,255,6);

		//calculating coverageMap
		vinya->fetchInfoFromMSK();

        customDefs.push_back(vinya); // add this def to the vector
	}

	//add holes - they always can appear 
	for (int i = 0; i < 8 ; i++)
	{
        customDefs.push_back(VLC->dobjinfo->gobjs[124][i]);
	}
}

class _HERO_SORTER
{
public:
	bool operator()(const ConstTransitivePtr<CGHeroInstance> & a, const ConstTransitivePtr<CGHeroInstance> & b)
	{
		return a->subID < b->subID;
	}
};

void CMap::readObjects(const ui8 * buffer, int &i)
{
    int howManyObjs = read_le_u32(buffer + i); i+=4;
	for(int ww=0; ww<howManyObjs; ++ww) //comment this line to turn loading objects off
	{
		CGObjectInstance * nobj = 0;

		int3 pos;
        pos.x = buffer[i++];
        pos.y = buffer[i++];
        pos.z = buffer[i++];

        int defnum = read_le_u32(buffer + i); i+=4;
		int idToBeGiven = objects.size();

        CGDefInfo * defInfo = customDefs.at(defnum);
		i+=5;

		switch(defInfo->id)
		{
		case Obj::EVENT: //for event objects
			{
				CGEvent *evnt = new CGEvent();
				nobj = evnt;

                bool guardMess = buffer[i]; ++i;
				if(guardMess)
				{
                    int messLong = read_le_u32(buffer + i); i+=4;
					if(messLong>0)
					{
						for(int yy=0; yy<messLong; ++yy)
						{
                            evnt->message +=buffer[i+yy];
						}
						i+=messLong;
					}
                    if(buffer[i++])
					{
                        readCreatureSet(evnt, buffer, i, 7, version > EMapFormat::ROE);
					}
					i+=4;
				}
                evnt->gainedExp = read_le_u32(buffer + i); i+=4;
                evnt->manaDiff = read_le_u32(buffer + i); i+=4;
                evnt->moraleDiff = (char)buffer[i]; ++i;
                evnt->luckDiff = (char)buffer[i]; ++i;

				evnt->resources.resize(GameConstants::RESOURCE_QUANTITY);
				for(int x=0; x<7; x++)
				{
                    evnt->resources[x] = read_le_u32(buffer + i);
					i+=4;
				}

				evnt->primskills.resize(GameConstants::PRIMARY_SKILLS);
				for(int x=0; x<4; x++)
				{
                    evnt->primskills[x] = buffer[i];
					i++;
				}

				int gabn; //number of gained abilities
                gabn = buffer[i]; ++i;
				for(int oo = 0; oo<gabn; ++oo)
				{
                    evnt->abilities.push_back(buffer[i]); ++i;
                    evnt->abilityLevels.push_back(buffer[i]); ++i;
				}

                int gart = buffer[i]; ++i; //number of gained artifacts
				for(int oo = 0; oo<gart; ++oo)
				{
                    if (version == EMapFormat::ROE)
					{
                        evnt->artifacts.push_back(buffer[i]); i++;
					}
					else
					{
                        evnt->artifacts.push_back(read_le_u16(buffer + i)); i+=2;
					}
				}

                int gspel = buffer[i]; ++i; //number of gained spells
				for(int oo = 0; oo<gspel; ++oo)
				{
                    evnt->spells.push_back(buffer[i]); ++i;
				}

                int gcre = buffer[i]; ++i; //number of gained creatures
                readCreatureSet(&evnt->creatures, buffer,i,gcre,(version > EMapFormat::ROE));

				i+=8;
                evnt->availableFor = buffer[i]; ++i;
                evnt->computerActivate = buffer[i]; ++i;
                evnt->removeAfterVisit = buffer[i]; ++i;
				evnt->humanActivate = true;

				i+=4;
				break;
			}
		case 34: case 70: case 62: //34 - hero; 70 - random hero; 62 - prison
			{
                nobj = loadHero(buffer, i, idToBeGiven);
				break;
			}
		case 4: //Arena
		case 51: //Mercenary Camp
		case 23: //Marletto Tower
		case 61: // Star Axis
		case 32: // Garden of Revelation
		case 100: //Learning Stone
		case 102: //Tree of Knowledge
		case 41: //Library of Enlightenment
		case 47: //School of Magic
		case 107: //School of War
			{
				nobj = new CGVisitableOPH();
				break;
			}
		case 55: //mystical garden
		case 112://windmill
		case 109://water wheel
			{
				nobj = new CGVisitableOPW();
				break;
			}
		case 43: //teleport
		case 44: //teleport
		case 45: //teleport
		case 103://subterranean gate
		case 111://Whirlpool
			{
				nobj = new CGTeleport();
				break;
			}
		case 12: //campfire
		case 29: //Flotsam
		case 82: //Sea Chest
		case 86: //Shipwreck Survivor
		case 101://treasure chest
			{
				nobj = new CGPickable();
				break;
			}
		case 54:  //Monster 
		case 71: case 72: case 73: case 74: case 75:	// Random Monster 1 - 4
		case 162: case 163: case 164:					// Random Monster 5 - 7
			{
				CGCreature *cre = new CGCreature();
				nobj = cre;

                if(version > EMapFormat::ROE)
				{
                    cre->identifier = read_le_u32(buffer + i); i+=4;
					questIdentifierToId[cre->identifier] = idToBeGiven;
					//monsters[cre->identifier] = cre;
				}

				CStackInstance *hlp = new CStackInstance();
                hlp->count =  read_le_u16(buffer + i); i+=2;
				//type will be set during initialization
				cre->putStack(0, hlp);

                cre->character = buffer[i]; ++i;
                bool isMesTre = buffer[i]; ++i; //true if there is message or treasury
				if(isMesTre)
				{
                    cre->message = readString(buffer,i);
					cre->resources.resize(GameConstants::RESOURCE_QUANTITY);
					for(int j=0; j<7; j++)
					{
                        cre->resources[j] = read_le_u32(buffer + i); i+=4;
					}

					int artID;
                    if (version == EMapFormat::ROE)
					{
                        artID = buffer[i]; i++;
					}
					else
					{
                        artID = read_le_u16(buffer + i); i+=2;
					}

                    if(version == EMapFormat::ROE)
					{
						if(artID!=0xff)
							cre->gainedArtifact = artID;
						else
							cre->gainedArtifact = -1;
					}
					else
					{
						if(artID!=0xffff)
							cre->gainedArtifact = artID;
						else
							cre->gainedArtifact = -1;
					}
				}
                cre->neverFlees = buffer[i]; ++i;
                cre->notGrowingTeam = buffer[i]; ++i;
				i+=2;;
				break;
			}
		case 59: case 91: //ocean bottle and sign
			{
				CGSignBottle *sb = new CGSignBottle();
				nobj = sb;
                sb->message = readString(buffer,i);
				i+=4;
				break;
			}
		case 83: //seer's hut
			{
                i = loadSeerHut(buffer, i, nobj);
				addQuest (nobj);
				break;
			}
		case 113: //witch hut
			{
				CGWitchHut *wh = new CGWitchHut();
				nobj = wh;
                if(version > EMapFormat::ROE) //in reo we cannot specify it - all are allowed (I hope)
				{
					int ist=i; //starting i for loop
					for(; i<ist+4; ++i)
					{
                        ui8 c = buffer[i];
						for(int yy=0; yy<8; ++yy)
						{
							if((i-ist)*8+yy < GameConstants::SKILL_QUANTITY)
							{
								if(c == (c|((ui8)intPow(2, yy))))
									wh->allowedAbilities.push_back((i-ist)*8+yy);
							}
						}
					}
				}
				else //(RoE map)
				{
					for(int gg=0; gg<GameConstants::SKILL_QUANTITY; ++gg)
					{
						wh->allowedAbilities.push_back(gg);
					}
				}
				break;
			}
		case 81: //scholar
			{
				CGScholar *sch = new CGScholar();
				nobj = sch;
                sch->bonusType = buffer[i++];
                sch->bonusID = buffer[i++];
				i+=6;
				break;
			}
		case 33: case 219: //garrison
			{
				CGGarrison *gar = new CGGarrison();
				nobj = gar;
                nobj->setOwner(buffer[i++]);
				i+=3;
                readCreatureSet(gar, buffer, i, 7, version > EMapFormat::ROE);
                if(version > EMapFormat::ROE)
				{
                    gar->removableUnits = buffer[i]; ++i;
				}
				else
					gar->removableUnits = true;
				i+=8;
				break;
			}
		case 5: //artifact	
		case 65: case 66: case 67: case 68: case 69: //random artifact
		case 93: //spell scroll
			{
				int artID = -1;
				int spellID = -1;
				CGArtifact *art = new CGArtifact();
				nobj = art;

                bool areSettings = buffer[i++];
				if(areSettings)
				{
                    art->message = readString(buffer,i);
                    bool areGuards = buffer[i++];
					if(areGuards)
					{
                        readCreatureSet(art, buffer, i, 7, version > EMapFormat::ROE);
					}
					i+=4;
				}

				if(defInfo->id==93)
				{
                    spellID = read_le_u32(buffer + i); i+=4;
					artID = 1;
				}
				else if(defInfo->id == 5) //specific artifact
				{
					artID = defInfo->subid;
				}

				art->storedArtifact = createArt(artID, spellID);
				break;
			}
		case 76: case 79: //random resource; resource
			{
				CGResource *res = new CGResource();
				nobj = res;

                bool isMessGuard = buffer[i]; ++i;
				if(isMessGuard)
				{
                    res->message = readString(buffer,i);
                    if(buffer[i++])
					{
                        readCreatureSet(res, buffer, i, 7, version > EMapFormat::ROE);
					}
					i+=4;
				}
                res->amount = read_le_u32(buffer + i); i+=4;
				if (defInfo->subid == 6) // Gold is multiplied by 100.
					res->amount *= 100;
				i+=4;

				break;
			}
		case 77: case 98: //random town; town
			{
                loadTown(nobj, buffer, i, defInfo->subid);
				break;
			}
		case 53: 
		case 220://mine (?)
			{
				nobj = new CGMine();
                nobj->setOwner(buffer[i++]);
				i+=3;
				break;
			}
		case 17: case 18: case 19: case 20: //dwellings
			{
				nobj = new CGDwelling();
                nobj->setOwner(buffer[i++]);
				i+=3;
				break;
			}
		case 78: //Refugee Camp
		case 106: //War Machine Factory
			{
				nobj = new CGDwelling();
				break;
			}
		case 88: case 89: case 90: //spell shrine
			{
				CGShrine * shr = new CGShrine();
				nobj = shr;
                shr->spell = buffer[i]; i+=4;
				break;
			}
		case 6: //pandora's box
			{
				CGPandoraBox *box = new CGPandoraBox();
				nobj = box;
                bool messg = buffer[i]; ++i;
				if(messg)
				{
                    box->message = readString(buffer,i);
                    if(buffer[i++])
					{
                        readCreatureSet(box, buffer, i, 7, version > EMapFormat::ROE);
					}
					i+=4;
				}

                box->gainedExp = read_le_u32(buffer + i); i+=4;
                box->manaDiff = read_le_u32(buffer + i); i+=4;
                box->moraleDiff = (si8)buffer[i]; ++i;
                box->luckDiff = (si8)buffer[i]; ++i;

				box->resources.resize(GameConstants::RESOURCE_QUANTITY);
				for(int x=0; x<7; x++)
				{
                    box->resources[x] = read_le_u32(buffer + i);
					i+=4;
				}

				box->primskills.resize(GameConstants::PRIMARY_SKILLS);
				for(int x=0; x<4; x++)
				{
                    box->primskills[x] = buffer[i];
					i++;
				}

				int gabn; //number of gained abilities
                gabn = buffer[i]; ++i;
				for(int oo = 0; oo<gabn; ++oo)
				{
                    box->abilities.push_back(buffer[i]); ++i;
                    box->abilityLevels.push_back(buffer[i]); ++i;
				}
                int gart = buffer[i]; ++i; //number of gained artifacts
				for(int oo = 0; oo<gart; ++oo)
				{
                    if(version > EMapFormat::ROE)
					{
                        box->artifacts.push_back(read_le_u16(buffer + i)); i+=2;
					}
					else
					{
                        box->artifacts.push_back(buffer[i]); i+=1;
					}
				}
                int gspel = buffer[i]; ++i; //number of gained spells
				for(int oo = 0; oo<gspel; ++oo)
				{
                    box->spells.push_back(buffer[i]); ++i;
				}
                int gcre = buffer[i]; ++i; //number of gained creatures
                readCreatureSet(&box->creatures, buffer,i,gcre,(version > EMapFormat::ROE));
				i+=8;
				break;
			}
		case 36: //grail
			{
				grailPos = pos;
                grailRadious = read_le_u32(buffer + i); i+=4;
				continue;
			}
		//dwellings
		case 216: //same as castle + level range
		case 217: //same as castle
		case 218: //level range
			{
				nobj = new CGDwelling();
				CSpecObjInfo * spec = nullptr;
				switch(defInfo->id)
				{
					break; case 216: spec = new CCreGenLeveledCastleInfo;
					break; case 217: spec = new CCreGenAsCastleInfo;
					break; case 218: spec = new CCreGenLeveledInfo;
				}

                spec->player = read_le_u32(buffer + i); i+=4;
				//216 and 217
				if (auto castleSpec = dynamic_cast<CCreGenAsCastleInfo*>(spec))
				{
                    castleSpec->identifier =  read_le_u32(buffer + i); i+=4;
					if(!castleSpec->identifier)
					{
						castleSpec->asCastle = false;
                        castleSpec->castles[0] = buffer[i]; ++i;
                        castleSpec->castles[1] = buffer[i]; ++i;
					}
					else
					{
						castleSpec->asCastle = true;
					}
				}

				//216 and 218
				if (auto lvlSpec = dynamic_cast<CCreGenLeveledInfo*>(spec))
				{
                    lvlSpec->minLevel = std::max(buffer[i], ui8(1)); ++i;
                    lvlSpec->maxLevel = std::min(buffer[i], ui8(7)); ++i;
				}
				nobj->setOwner(spec->player);
				static_cast<CGDwelling*>(nobj)->info = spec;
				break;
			}
		case 215:
			{
				CGQuestGuard *guard = new CGQuestGuard();
				addQuest (guard);
                loadQuest(guard, buffer, i);
				nobj = guard;
				break;
			}
		case 28: //faerie ring
		case 14: //Swan pond
		case 38: //idol of fortune
		case 30: //Fountain of Fortune
		case 64: //Rally Flag
		case 56: //oasis
		case 96: //temple
		case 110://Watering Hole
		case 31: //Fountain of Youth
		case 11: //Buoy
		case 52: //Mermaid
		case 94: //Stables
			{
				nobj = new CGBonusingObject();
				break;
			}
		case 49: //Magic Well
			{
				nobj = new CGMagicWell();
				break;
			}
		case 15: //Cover of darkness
		case 58: //Redwood Observatory
		case 60: //Pillar of Fire
			{
				nobj = new CGObservatory();
				break;
			}
		case 22: //Corpse
		case 39: //Lean To
		case 105://Wagon
		case 108://Warrior's Tomb
			{
				nobj = new CGOnceVisitable();
				break;
			}
		case 8: //Boat
			{
				nobj = new CGBoat();
				break;
			}
		case 92: //Sirens
			{
				nobj = new CGSirens();
				break;
			}
		case 87: //Shipyard
			{
				nobj = new CGShipyard();
                nobj->setOwner(read_le_u32(buffer + i)); i+=4;
				break;
			}
		case 214: //hero placeholder
			{
				CGHeroPlaceholder *hp = new CGHeroPlaceholder();;
				nobj = hp;

                int a = buffer[i++]; //unknown byte, seems to be always 0 (if not - scream!)
				tlog2 << "Unhandled Hero Placeholder detected: "<<a<<"\n";

                int htid = buffer[i++]; //hero type id
				nobj->subID = htid;

				if(htid == 0xff)
                    hp->power = buffer[i++];
				else
					hp->power = 0;

				break;
			}
		case 10: //Keymaster
			{
				nobj = new CGKeymasterTent();
				break;
			}
		case 9: //Border Guard
			{
				nobj = new CGBorderGuard();
				addQuest (nobj);
				break;
			}
		case 212: //Border Gate
			{
				nobj = new CGBorderGate();
				addQuest (nobj);
				break;
			}
		case 27: case 37: //Eye and Hut of Magi
			{
				nobj = new CGMagi();
				break;
			}
		case 16: case 24: case 25: case 84: case 85: //treasure bank
			{
				nobj = new CBank();
				break;
			}
		case 63: //Pyramid
			{
				nobj = new CGPyramid();
				break;
			}
		case 13: //Cartographer
			{
				nobj = new CCartographer();
				break;
			}
		case 48: //Magic Spring
			{
				nobj = new CGMagicSpring();
				break;
			}
		case 97: //den of thieves
			{
				nobj = new CGDenOfthieves();
				break;
			}
		case 57: //Obelisk
			{
				nobj = new CGObelisk();
				break;
			}
		case 42: //Lighthouse
			{
				nobj = new CGLighthouse();
                nobj->tempOwner = read_le_u32(buffer + i); i+=4;
				break;
			}
		case 2: //Altar of Sacrifice
		case 99: //Trading Post
		case 213: //Freelancer's Guild
		case 221: //Trading Post (snow)
			{
				nobj = new CGMarket();
				break;
			}
		case 104: //University
			{
				nobj = new CGUniversity();
				break;
			}
		case 7: //Black Market
			{
				nobj = new CGBlackMarket();
				break;
			}

		default: //any other object
			{
				nobj = new CGObjectInstance();
				break;
			}

		} //end of main switch

		nobj->pos = pos;
		nobj->ID = defInfo->id;
		nobj->id = idToBeGiven;
		if(nobj->ID != Obj::HERO && nobj->ID != Obj::HERO_PLACEHOLDER && nobj->ID != Obj::PRISON)
			nobj->subID = defInfo->subid;
		nobj->defInfo = defInfo;
		assert(idToBeGiven == objects.size());
		objects.push_back(nobj);
		if(nobj->ID==Obj::TOWN)
			towns.push_back(static_cast<CGTownInstance*>(nobj));
		if(nobj->ID==Obj::HERO)
			heroes.push_back(static_cast<CGHeroInstance*>(nobj));
	}

	std::sort(heroes.begin(), heroes.end(), _HERO_SORTER());
}

void CMap::readEvents( const ui8 * buffer, int &i )
{
    int numberOfEvents = read_le_u32(buffer + i); i+=4;
	for(int yyoo=0; yyoo<numberOfEvents; ++yyoo)
	{
		CMapEvent *ne = new CMapEvent();
		ne->name = std::string();
		ne->message = std::string();
        int nameLen = read_le_u32(buffer + i); i+=4;
		for(int qq=0; qq<nameLen; ++qq)
		{
            ne->name += buffer[i]; ++i;
		}
        int messLen = read_le_u32(buffer + i); i+=4;
		for(int qq=0; qq<messLen; ++qq)
		{
            ne->message +=buffer[i]; ++i;
		}
		for(int k=0; k < 7; k++)
		{
            ne->resources[k] = read_le_u32(buffer + i); i+=4;
		}
        ne->players = buffer[i]; ++i;
        if(version > EMapFormat::AB)
		{
            ne->humanAffected = buffer[i]; ++i;
		}
		else
			ne->humanAffected = true;
        ne->computerAffected = buffer[i]; ++i;
        ne->firstOccurence = read_le_u16(buffer + i); i+=2;
        ne->nextOccurence = buffer[i]; ++i;

		char unknown[17];
        memcpy(unknown, buffer+i, 17);
		i+=17;

		events.push_back(ne);
	}
}

bool CMap::isInTheMap(const int3 &pos) const
{
	if(pos.x<0 || pos.y<0 || pos.z<0 || pos.x >= width || pos.y >= height || pos.z > twoLevel)
		return false;
	else return true;
}

void CMap::loadQuest(IQuestObject * guard, const ui8 * buffer, int & i)
{
    guard->quest->missionType = buffer[i]; ++i;
	//int len1, len2, len3;
	switch(guard->quest->missionType)
	{
	case 0:
		return;
	case 2:
		{
			guard->quest->m2stats.resize(4);
			for(int x=0; x<4; x++)
			{
                guard->quest->m2stats[x] = buffer[i++];
			}
		}
		break;
	case 1:
	case 3:
	case 4:
		{
            guard->quest->m13489val = read_le_u32(buffer + i); i+=4;
			break;
		}
	case 5:
		{
            int artNumber = buffer[i]; ++i;
			for(int yy=0; yy<artNumber; ++yy)
			{
                int artid = read_le_u16(buffer + i); i+=2;
				guard->quest->m5arts.push_back(artid); 
				allowedArtifact[artid] = false; //these are unavailable for random generation
			}
			break;
		}
	case 6:
		{
            int typeNumber = buffer[i]; ++i;
			guard->quest->m6creatures.resize(typeNumber);
			for(int hh=0; hh<typeNumber; ++hh)
			{
                guard->quest->m6creatures[hh].type = VLC->creh->creatures[read_le_u16(buffer + i)]; i+=2;
                guard->quest->m6creatures[hh].count = read_le_u16(buffer + i); i+=2;
			}
			break;
		}
	case 7:
		{
			guard->quest->m7resources.resize(7);
			for(int x=0; x<7; x++)
			{
                guard->quest->m7resources[x] = read_le_u32(buffer + i);
				i+=4;
			}
			break;
		}
	case 8:
	case 9:
		{
            guard->quest->m13489val = buffer[i]; ++i;
			break;
		}
	}


    int limit = read_le_u32(buffer + i); i+=4;
	if(limit == ((int)0xffffffff))
	{
		guard->quest->lastDay = -1;
	}
	else
	{
		guard->quest->lastDay = limit;
	}
    guard->quest->firstVisitText = readString(buffer,i);
    guard->quest->nextVisitText = readString(buffer,i);
    guard->quest->completedText = readString(buffer,i);
	guard->quest->isCustomFirst = guard->quest->firstVisitText.size() > 0;
	guard->quest->isCustomNext = guard->quest->nextVisitText.size() > 0;
	guard->quest->isCustomComplete = guard->quest->completedText.size() > 0;
}

TerrainTile & CMap::getTile( const int3 & tile )
{
	return terrain[tile.x][tile.y][tile.z];
}

const TerrainTile & CMap::getTile( const int3 & tile ) const
{
	return terrain[tile.x][tile.y][tile.z];
}

bool CMap::isWaterTile(const int3 &pos) const
{
    return isInTheMap(pos) && getTile(pos).tertype == ETerrainType::WATER;
}

const CGObjectInstance *CMap::getObjectiveObjectFrom(int3 pos, bool lookForHero)
{
    const std::vector <CGObjectInstance *> & objs = getTile(pos).visitableObjects;
	assert(objs.size());
	if(objs.size() > 1 && lookForHero && objs.front()->ID != Obj::HERO)
	{
		assert(objs.back()->ID == Obj::HERO);
		return objs.back();
	}
	else
		return objs.front();
}

void CMap::checkForObjectives()
{
	if(isInTheMap(victoryCondition.pos))
		victoryCondition.obj = getObjectiveObjectFrom(victoryCondition.pos, victoryCondition.condition == EVictoryConditionType::BEATHERO);

	if(isInTheMap(lossCondition.pos))
		lossCondition.obj = getObjectiveObjectFrom(lossCondition.pos, lossCondition.typeOfLossCon == ELossConditionType::LOSSHERO);
}

void CMap::addNewArtifactInstance( CArtifactInstance *art )
{
	art->id = artInstances.size();
	artInstances.push_back(art);
}

void CMap::addQuest (CGObjectInstance * quest)
{
    auto q = dynamic_cast<IQuestObject *>(quest);
	q->quest->qid = quests.size();
	quests.push_back (q->quest);
}

bool CMap::loadArtifactToSlot(CGHeroInstance * hero, int slot, const ui8 * buffer, int & i)
{
    const int artmask = version == EMapFormat::ROE ? 0xff : 0xffff;
	int aid;

    if (version == EMapFormat::ROE)
	{
        aid = buffer[i]; i++;
	}
	else
	{
        aid = read_le_u16(buffer + i); i+=2;
	}

	bool isArt  =  aid != artmask;
	if(isArt)
	{
		if(vstd::contains(VLC->arth->bigArtifacts, aid) && slot >= GameConstants::BACKPACK_START)
		{
			tlog3 << "Warning: A big artifact (war machine) in hero's backpack, ignoring...\n";
			return false;
		}
		if(aid == 0 && slot == ArtifactPosition::MISC5)
		{
			//TODO: check how H3 handles it -> art 0 in slot 18 in AB map
			tlog3 << "Spellbook to MISC5 slot? Putting it spellbook place. AB format peculiarity ? (format " << (int)version << ")\n";
			slot = ArtifactPosition::SPELLBOOK;
		}
		
        hero->putArtifact(slot, createArt(aid));
	}
	return isArt;
}

void CMap::loadArtifactsOfHero(const ui8 * buffer, int & i, CGHeroInstance * hero)
{
    bool artSet = buffer[i]; ++i; //true if artifact set is not default (hero has some artifacts)
	if(artSet)
	{
		for(int pom=0;pom<16;pom++)
            loadArtifactToSlot(hero, pom, buffer, i);

		//misc5 art //17
        if(version >= EMapFormat::SOD)
		{
            if(!loadArtifactToSlot(hero, ArtifactPosition::MACH4, buffer, i))
                hero->putArtifact(ArtifactPosition::MACH4, createArt(GameConstants::ID_CATAPULT)); //catapult by default
		}

        loadArtifactToSlot(hero, ArtifactPosition::SPELLBOOK, buffer, i);

		//19 //???what is that? gap in file or what? - it's probably fifth slot..
        if(version > EMapFormat::ROE)
            loadArtifactToSlot(hero, ArtifactPosition::MISC5, buffer, i);
		else
			i+=1;

		//bag artifacts //20
        int amount = read_le_u16(buffer + i); i+=2; //number of artifacts in hero's bag
		for(int ss = 0; ss < amount; ++ss)
            loadArtifactToSlot(hero, GameConstants::BACKPACK_START + hero->artifactsInBackpack.size(), buffer, i);
	} //artifacts
}

CArtifactInstance * CMap::createArt(int aid, int spellID /*= -1*/)
{
	CArtifactInstance *a = NULL;
	if(aid >= 0)
	{
        if(spellID < 0)
			a = CArtifactInstance::createNewArtifactInstance(aid);
		else
            a = CArtifactInstance::createScroll(VLC->spellh->spells[spellID]);
	}
	else
		a = new CArtifactInstance();

	addNewArtifactInstance(a);
	if(a->artType && a->artType->constituents) //TODO make it nicer
	{
		CCombinedArtifactInstance *comb = dynamic_cast<CCombinedArtifactInstance*>(a);
		BOOST_FOREACH(CCombinedArtifactInstance::ConstituentInfo &ci, comb->constituentsInfo)
		{
			addNewArtifactInstance(ci.art);
		}
	}
	return a;
}

void CMap::eraseArtifactInstance(CArtifactInstance *art)
{
	assert(artInstances[art->id] == art);
	artInstances[art->id].dellNull();
}

LossCondition::LossCondition()
{
	obj = NULL;
	timeLimit = -1;
	pos = int3(-1,-1,-1);
}

VictoryCondition::VictoryCondition()
{
	pos = int3(-1,-1,-1);
	obj = NULL;
	ID = allowNormalVictory = appliesToAI = count = 0;
}

bool TerrainTile::entrableTerrain(const TerrainTile * from /*= NULL*/) const
{
    return entrableTerrain(from ? from->tertype != ETerrainType::WATER : true, from ? from->tertype == ETerrainType::WATER : true);
}

bool TerrainTile::entrableTerrain(bool allowLand, bool allowSea) const
{
    return tertype != ETerrainType::ROCK
        && ((allowSea && tertype == ETerrainType::WATER)  ||  (allowLand && tertype != ETerrainType::WATER));
}

bool TerrainTile::isClear(const TerrainTile *from /*= NULL*/) const
{
	return entrableTerrain(from) && !blocked;
}

int TerrainTile::topVisitableID() const
{
	return visitableObjects.size() ? visitableObjects.back()->ID : -1;
}

bool TerrainTile::isCoastal() const
{
    return extTileFlags & 64;
}

bool TerrainTile::hasFavourableWinds() const
{
    return extTileFlags & 128;
}

bool TerrainTile::isWater() const
{
    return tertype == ETerrainType::WATER;
}
