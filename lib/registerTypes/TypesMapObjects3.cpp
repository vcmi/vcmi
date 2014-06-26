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

template void registerTypesMapObjectTypes<CISer<CConnection>>(CISer<CConnection>& s);
template void registerTypesMapObjectTypes<COSer<CConnection>>(COSer<CConnection>& s);
template void registerTypesMapObjectTypes<CISer<CMemorySerializer>>(CISer<CMemorySerializer>& s);
template void registerTypesMapObjectTypes<COSer<CMemorySerializer>>(COSer<CMemorySerializer>& s);
template void registerTypesMapObjectTypes<CSaveFile>(CSaveFile & s);
template void registerTypesMapObjectTypes<CLoadFile>(CLoadFile & s);
template void registerTypesMapObjectTypes<CTypeList>(CTypeList & s);
template void registerTypesMapObjectTypes<CLoadIntegrityValidator>(CLoadIntegrityValidator & s);
