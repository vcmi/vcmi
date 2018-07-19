/*
* ResourceManager.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "AIUtility.h"
#include "Goals.h"
#include "../../lib/GameConstants.h"
#include "../../lib/VCMI_Lib.h"
#include "VCAI.h"

class ResourceManager //: public: IAIManager
{
	friend class VCAI;

public:
	bool containsSavedRes(const TResources & cost) const;
	TResources freeResources() const; //owned resources minus gold reserve
	TResources estimateIncome() const;

private:
	TResources saving;

	//TODO: register?
	template<typename Handler> void serializeInternal(Handler & h, const int version)
	{
		h & saving;
	}
};