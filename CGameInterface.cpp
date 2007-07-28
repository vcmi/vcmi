#include "stdafx.h"
#include "CGameInterface.h"
using namespace CSDL_Ext;
void CButtonBase::show()
{
	blitAt(imgs[state],pos.x,pos.y);
	updateRect(&pos);
}