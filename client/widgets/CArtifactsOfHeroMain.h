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

class CArtifactsOfHeroMain : public CArtifactsOfHeroBase
{
public:
	CArtifactsOfHeroMain(const Point & position, bool costumesEnabled = false);
	~CArtifactsOfHeroMain() override;

private:
	// TODO may be removed if CKeyShortcut supports callbacks
	class CKeyShortcutWrapper : public CKeyShortcut
	{
	public:
		using KeyPressedFunctor = std::function<void()>;

		CKeyShortcutWrapper(const EShortcut & key, const KeyPressedFunctor & onCostumeSelect);
		void clickPressed(const Point & cursorPosition) override;

	private:
		KeyPressedFunctor onCostumeSelect;
	};

	const std::array<EShortcut, GameConstants::HERO_COSTUMES_ARTIFACTS> hotkeys =
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
	std::vector<std::shared_ptr<CKeyShortcutWrapper>> costumesSwitchers;

	void onCostumeSelect(const size_t costumeIndex);
};
