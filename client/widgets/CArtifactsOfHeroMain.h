/*
 * CArtifactsOfHeroMain.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CArtifactsOfHeroBase.h"

#include "../gui/Shortcut.h"

class CArtifactsOfHeroMain : public CArtifactsOfHeroBase, public CKeyShortcut
{
public:
	CArtifactsOfHeroMain(const Point & position);
	~CArtifactsOfHeroMain() override;
	void enableArtifactsCostumeSwitcher();
	void keyPressed(EShortcut key) override;
	void keyReleased(EShortcut key) override;

private:
	const std::vector<EShortcut> costumesSwitcherHotkeys =
	{
		EShortcut::HERO_COSTUME_0,
		EShortcut::HERO_COSTUME_1,
		EShortcut::HERO_COSTUME_2,
		EShortcut::HERO_COSTUME_3,
		EShortcut::HERO_COSTUME_4,
		EShortcut::HERO_COSTUME_5,
		EShortcut::HERO_COSTUME_6,
		EShortcut::HERO_COSTUME_7,
		EShortcut::HERO_COSTUME_8,
		EShortcut::HERO_COSTUME_9
	};
};
