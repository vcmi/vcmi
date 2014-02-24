#include "StdInc.h"
#include "RegisterTypes.h"

#include "mapping/CMapInfo.h"
#include "StartInfo.h"
#include "BattleState.h"
#include "CGameState.h"
#include "mapping/CMap.h"
#include "CModHandler.h"
#include "CObjectHandler.h"
#include "CCreatureHandler.h"
#include "VCMI_Lib.h"
#include "CArtHandler.h"
#include "CHeroHandler.h"
#include "CSpellHandler.h"
#include "CTownHandler.h"
#include "mapping/CCampaignHandler.h"
#include "NetPacks.h"
#include "CDefObjInfoHandler.h"


template void registerTypesMapObjects<CISer<CConnection>>(CISer<CConnection>& s);
template void registerTypesMapObjects<COSer<CConnection>>(COSer<CConnection>& s);
template void registerTypesMapObjects<CISer<CMemorySerializer>>(CISer<CMemorySerializer>& s);
template void registerTypesMapObjects<COSer<CMemorySerializer>>(COSer<CMemorySerializer>& s);
template void registerTypesMapObjects<CSaveFile>(CSaveFile & s);
template void registerTypesMapObjects<CLoadFile>(CLoadFile & s);
template void registerTypesMapObjects<CTypeList>(CTypeList & s);
template void registerTypesMapObjects<CLoadIntegrityValidator>(CLoadIntegrityValidator & s);
