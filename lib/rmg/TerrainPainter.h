/*
 * TerrainPainter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "Zone.h"

VCMI_LIB_NAMESPACE_BEGIN

class TerrainPainter: public Modificator
{
public:
	MODIFICATOR(TerrainPainter);
	
	void process() override;
	void init() override;

	void initTerrainType();
};

VCMI_LIB_NAMESPACE_END
