/*
 * CGlobalAI.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CGameInterface.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CGlobalAI : public CGameInterface // AI class (to derivate)
{
public:
	std::shared_ptr<Environment> env;
	CGlobalAI();
};

VCMI_LIB_NAMESPACE_END
