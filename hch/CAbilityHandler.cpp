#include "../stdafx.h"
#include "CAbilityHandler.h"
#include "../CGameInfo.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"
#include "CDefHandler.h"

void CAbilityHandler::loadAbilities()
{
	abils32 = CDefHandler::giveDef("SECSK32.DEF");
	abils44 = CDefHandler::giveDef("SECSKILL.DEF");
	abils82 = CDefHandler::giveDef("SECSK82.DEF");

}