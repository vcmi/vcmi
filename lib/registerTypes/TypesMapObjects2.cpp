/*
 * TypesMapObjects2.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "RegisterTypes.h"

#include "../StartInfo.h"
#include "../CStack.h"
#include "../battle/BattleInfo.h"
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


template void registerTypesMapObjects2<BinaryDeserializer>(BinaryDeserializer & s);
template void registerTypesMapObjects2<BinarySerializer>(BinarySerializer & s);
template void registerTypesMapObjects2<CTypeList>(CTypeList & s);
