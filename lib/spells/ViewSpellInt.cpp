/*
 * ViewSpellInt.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "ViewSpellInt.h"

#include "../mapObjects/CObjectHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

ObjectPosInfo::ObjectPosInfo():
	pos(), id(Obj::NO_OBJ), subId(-1), owner(PlayerColor::CANNOT_DETERMINE)
{

}

ObjectPosInfo::ObjectPosInfo(const CGObjectInstance * obj):
	pos(obj->visitablePos()), id(obj->ID), subId(obj->subID), owner(obj->tempOwner)
{

}

VCMI_LIB_NAMESPACE_END
