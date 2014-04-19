#pragma once

#include "IGameCallback2.h" // for CPrivilagedInfoCallback, IGameEventCallback

/*#include "BattleHex.h"
#include "ResourceSet.h"
#include "int3.h"
#include "GameConstants.h"
#include "CBattleCallback.h"*/

/*
 * IGameCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/*struct SetMovePoints;
struct GiveBonus;
class CGObjectInstance;
class CGTownInstance;
class CGHeroInstance;
struct BlockingDialog;
struct InfoWindow;
struct MetaString;
struct ShowInInfobox;
struct BattleResult;
struct Component;
class CGameState;
struct PlayerSettings;
struct CPackForClient;
class CArtHandler;
class CArtifact;
class CArmedInstance;
struct TerrainTile;
struct PlayerState;
class CTown;
struct StackLocation;
struct ArtifactLocation;
class CArtifactInstance;
struct StartInfo;
struct InfoAboutTown;
struct UpgradeInfo;
struct SThievesGuildInfo;
struct CPath;
class CGDwelling;
struct InfoAboutHero;
class CMapHeader;
struct BattleAction;
class CStack;
class CSpell;
class CCreatureSet;
class CCreature;
class CStackBasicDescriptor;
struct TeamState;
struct QuestInfo;*/
class CGCreature;
/*class CSaveFile;
class CLoadFile;*/

/// Interface class for handling general game logic and actions
class DLL_LINKAGE IGameCallback : public CPrivilagedInfoCallback, public IGameEventCallback
{
public:
	virtual ~IGameCallback(){};

	//do sth
	const CGObjectInstance *putNewObject(Obj ID, int subID, int3 pos);
	const CGCreature *putNewMonster(CreatureID creID, int count, int3 pos);

	//get info
	virtual bool isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero);

	friend struct CPack;
	friend struct CPackForClient;
	friend struct CPackForServer;
};

