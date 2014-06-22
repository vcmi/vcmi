#include "StdInc.h"
#include "RegisterTypes.h"

#include "../mapping/CMapInfo.h"
#include "../StartInfo.h"
#include "../BattleState.h"
#include "../CGameState.h"
#include "../mapping/CMap.h"
#include "../CModHandler.h"
#include "../mapObjects/CObjectHandler.h"
#include "../CCreatureHandler.h"
#include "../VCMI_Lib.h"
#include "../CArtHandler.h"
#include "../CHeroHandler.h"
#include "../CSpellHandler.h"
#include "../CTownHandler.h"
#include "../mapping/CCampaignHandler.h"
#include "../NetPacks.h"
#include "../mapObjects/CObjectClassesHandler.h"

template void registerTypesPregamePacks<CISer<CConnection>>(CISer<CConnection>& s);
template void registerTypesPregamePacks<COSer<CConnection>>(COSer<CConnection>& s);
template void registerTypesPregamePacks<CISer<CMemorySerializer>>(CISer<CMemorySerializer>& s);
template void registerTypesPregamePacks<COSer<CMemorySerializer>>(COSer<CMemorySerializer>& s);
template void registerTypesPregamePacks<CSaveFile>(CSaveFile & s);
template void registerTypesPregamePacks<CLoadFile>(CLoadFile & s);
template void registerTypesPregamePacks<CTypeList>(CTypeList & s);
template void registerTypesPregamePacks<CLoadIntegrityValidator>(CLoadIntegrityValidator & s);
