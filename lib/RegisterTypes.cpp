#include "StdInc.h"
#define DO_NOT_DISABLE_REGISTER_TYPES_INSTANTIATION
#include "RegisterTypes.h"

#include "Mapping/CMapInfo.h"
#include "StartInfo.h"
#include "BattleState.h"
#include "CGameState.h"
#include "Mapping/CMap.h"
#include "CModHandler.h"
#include "CObjectHandler.h"
#include "CCreatureHandler.h"
#include "VCMI_Lib.h"
#include "CArtHandler.h"
#include "CHeroHandler.h"
#include "CSpellHandler.h"
#include "CTownHandler.h"
#include "Mapping/CCampaignHandler.h"
#include "NetPacks.h"
#include "CDefObjInfoHandler.h"

/*
 * RegisterTypes.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

template void registerTypes<CISer<CConnection>>(CISer<CConnection>& s);
template void registerTypes<COSer<CConnection>>(COSer<CConnection>& s);
template void registerTypes<CSaveFile>(CSaveFile & s);
template void registerTypes<CLoadFile>(CLoadFile & s);
template void registerTypes<CTypeList>(CTypeList & s);
template void registerTypes<CLoadIntegrityValidator>(CLoadIntegrityValidator & s);
