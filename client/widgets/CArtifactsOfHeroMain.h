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
	static constexpr std::array costumeSaveShortcuts = {
		EShortcut::HERO_COSTUME_SAVE_0,
		EShortcut::HERO_COSTUME_SAVE_1,
		EShortcut::HERO_COSTUME_SAVE_2,
		EShortcut::HERO_COSTUME_SAVE_3,
		EShortcut::HERO_COSTUME_SAVE_4,
		EShortcut::HERO_COSTUME_SAVE_5,
		EShortcut::HERO_COSTUME_SAVE_6,
		EShortcut::HERO_COSTUME_SAVE_7,
		EShortcut::HERO_COSTUME_SAVE_8,
		EShortcut::HERO_COSTUME_SAVE_9
	};

	static constexpr std::array costumeLoadShortcuts = {
		EShortcut::HERO_COSTUME_LOAD_0,
		EShortcut::HERO_COSTUME_LOAD_1,
		EShortcut::HERO_COSTUME_LOAD_2,
		EShortcut::HERO_COSTUME_LOAD_3,
		EShortcut::HERO_COSTUME_LOAD_4,
		EShortcut::HERO_COSTUME_LOAD_5,
		EShortcut::HERO_COSTUME_LOAD_6,
		EShortcut::HERO_COSTUME_LOAD_7,
		EShortcut::HERO_COSTUME_LOAD_8,
		EShortcut::HERO_COSTUME_LOAD_9
	};
};
