#include "StdInc.h"

#include "mock_ResourceManager.h"

ResourceManagerMock::ResourceManagerMock(CPlayerSpecificInfoCallback * CB, VCAI * AI)
	: ResourceManager(CB, AI)
{
}
