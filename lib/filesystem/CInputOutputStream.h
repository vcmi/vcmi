/*
 * CInputOutputStream.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CInputStream.h"
#include "COutputStream.h"

VCMI_LIB_NAMESPACE_BEGIN

class CInputOutputStream: public CInputStream, public COutputStream
{
	
};

VCMI_LIB_NAMESPACE_END
