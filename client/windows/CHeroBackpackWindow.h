/*
 * CHeroBackpackWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../widgets/CWindowWithArtifacts.h"
#include "CWindowObject.h"

class CHeroBackpackWindow : public CWindowObject, public CWindowWithArtifacts
{
public:
	CHeroBackpackWindow(const CGHeroInstance * hero);
	
private:
	std::shared_ptr<CArtifactsOfHeroBackpack> arts;
};