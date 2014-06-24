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

template void registerTypesServerPacks<CISer<CConnection>>(CISer<CConnection>& s);
template void registerTypesServerPacks<COSer<CConnection>>(COSer<CConnection>& s);
template void registerTypesServerPacks<CISer<CMemorySerializer>>(CISer<CMemorySerializer>& s);
template void registerTypesServerPacks<COSer<CMemorySerializer>>(COSer<CMemorySerializer>& s);
template void registerTypesServerPacks<CSaveFile>(CSaveFile & s);
template void registerTypesServerPacks<CLoadFile>(CLoadFile & s);
template void registerTypesServerPacks<CTypeList>(CTypeList & s);
template void registerTypesServerPacks<CLoadIntegrityValidator>(CLoadIntegrityValidator & s);
