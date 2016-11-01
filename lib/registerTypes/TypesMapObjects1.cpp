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
#include "../spells/CSpellHandler.h"
#include "../CTownHandler.h"
#include "../mapping/CCampaignHandler.h"
#include "../NetPacks.h"
#include "../mapObjects/CObjectClassesHandler.h"

#include "../serializer/BinaryDeserializer.h"
#include "../serializer/BinarySerializer.h"
#include "../serializer/CTypeList.h"

template void registerTypesMapObjects1<BinaryDeserializer>(BinaryDeserializer & s);
template void registerTypesMapObjects1<BinarySerializer>(BinarySerializer & s);
template void registerTypesMapObjects1<CTypeList>(CTypeList & s);


