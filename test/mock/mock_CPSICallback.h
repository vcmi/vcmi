/*
* mock_CPLayerSpecificInfoCallback.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "gtest/gtest.h"

#include "../lib/callback/CPlayerSpecificInfoCallback.h"
#include "../lib/ResourceSet.h"

class CPSICallbackMock : public CPlayerSpecificInfoCallback
{
public:
	using CPlayerSpecificInfoCallback::CPlayerSpecificInfoCallback;

	MOCK_CONST_METHOD0(getResourceAmount, TResources());
	//std::vector <const CGTownInstance *> getTownsInfo(bool onlyOur = true) const;
	MOCK_CONST_METHOD0(getTownsInfo, std::vector <const CGTownInstance *>());
	MOCK_CONST_METHOD1(getTownsInfo, std::vector <const CGTownInstance *>(bool));
};
