#define VCMI_DLL
#include "Connection.h"
#include "NetPacks.h"
#include "VCMI_Lib.h"
#include "../hch./CObjectHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CTownHandler.h"
#include "RegisterTypes.h"


void foofoofoo()
{
	//never called function to force instantation of templates
	int *ccc = NULL;
	registerTypes((CISer<CConnection>&)*ccc);
	registerTypes((COSer<CConnection>&)*ccc);
	registerTypes((CSaveFile&)*ccc);
	registerTypes((CLoadFile&)*ccc);
}