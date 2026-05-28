/*
 * CAdventureOptions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "AdventureOptions.h"

#include "../CPlayerInterface.h"
#include "../lobby/CCampaignInfoScreen.h"
#include "../lobby/CScenarioInfoScreen.h"
#include "../render/AssetGenerator.h"
#include "../render/IRenderHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/WindowHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/ShortcutHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/Slider.h"
#include "AdventureMapInterface.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/StartInfo.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../widgets/TextControls.h"

AdventureOptions::AdventureOptions()
	: CWindowObject(PLAYER_COLORED, settings["general"]["enableUiEnhancements"].Bool() ? ImagePath::builtin("AdventureOptionsBackground") : ImagePath::builtin("ADVOPTS"))
{
	OBJECT_CONSTRUCTION;

	const bool enhanced = settings["general"]["enableUiEnhancements"].Bool();

	// Build shortcut state lookup from the adventure map
	std::map<EShortcut, AdventureMapShortcutState> shortcutStates;
	if(adventureInt)
		for(const auto & s : adventureInt->getAdventureShortcuts().getShortcuts())
			shortcutStates[s.shortcut] = s;

	const JsonNode config(JsonPath::builtin("config/widgets/adventureOptions"));

	// Pre-parse all entries from JSON into ButtonEntry records
	for(const JsonNode & entry : config["buttons"].Vector())
	{
		const std::string shortcutId = entry["shortcut"].String();
		const EShortcut   shortcut   = ENGINE->shortcuts().findShortcut(shortcutId);

		AnimationPath animPath;
		if(enhanced && !entry["overlay"].isNull())
		{
			const std::string overlayName = entry["overlay"].String();
			animPath = AnimationPath::builtin("SPRITES/" + overlayName);

			auto assetGenerator = ENGINE->renderHandler().getAssetGenerator();
			auto layout = assetGenerator->createAdventureOptionsButton(ImagePath::builtin(overlayName));
			assetGenerator->addAnimationFile(animPath, layout);
			ENGINE->renderHandler().updateGeneratedAssets();
		}
		else
		{
			if(entry["legacyAnimation"].isNull())
				continue;
			animPath = AnimationPath::builtin(entry["legacyAnimation"].String());
		}

		bool isBlocked = false;
		std::function<void()> callback;
		auto it = shortcutStates.find(shortcut);
		if(it != shortcutStates.end())
		{
			isBlocked = !it->second.isEnabled;
			callback  = it->second.callback;
		}
		else
		{
			logGlobal->warn("AdventureOptions: no shortcut state found for '%s'", shortcutId);
		}

		ButtonEntry be;
		be.animPath  = animPath;
		be.shortcut  = shortcut;
		be.isBlocked = isBlocked;
		be.callback  = callback;
		be.labelKey  = (enhanced && !entry["label"].isNull()) ? entry["label"].String() : "";
		allEntries.push_back(std::move(be));
	}

	// Restore persisted scroll position
	const int savedScroll = settings["adventure"]["optionsScrollPos"].Integer();
	const int initScroll  = std::clamp(savedScroll, 0, std::max(0, static_cast<int>(allEntries.size()) - MAX_VISIBLE));

	rebuildVisibleButtons(initScroll, enhanced);

	// Scrollbar – only when there are more buttons than fit
	if(static_cast<int>(allEntries.size()) > MAX_VISIBLE)
	{
		const int sliderY      = 24;
		const int sliderLength = 282;

		scrollBar = std::make_shared<CSlider>(
			Point(252, sliderY),
			sliderLength,
			[this, enhanced](int val)
			{
				Settings adventure = settings.write["adventure"];
				adventure["optionsScrollPos"].Integer() = val;
				rebuildVisibleButtons(val, enhanced);
			},
			MAX_VISIBLE,
			static_cast<int>(allEntries.size()),
			initScroll,
			Orientation::VERTICAL,
			CSlider::BROWN);

		scrollBar->setScrollBounds(Rect(-228, 0, 228 + 16, MAX_VISIBLE * BUTTON_STEP));
		scrollBar->setPanningStep(BUTTON_STEP);
		scrollBar->setInertiaEnabled(true);
	}

	exit = std::make_shared<CButton>(Point(203, 313), AnimationPath::builtin("IOK6432.DEF"), CButton::tooltip(), std::bind(&AdventureOptions::close, this), EShortcut::GLOBAL_RETURN);
}

void AdventureOptions::rebuildVisibleButtons(int scrollPos, bool enhanced)
{
	buttons.clear();
	buttonLabels.clear();

	OBJECT_CONSTRUCTION;

	const int from = scrollPos;
	const int to   = std::min(from + MAX_VISIBLE, static_cast<int>(allEntries.size()));

	for(int i = from; i < to; ++i)
	{
		const ButtonEntry & be  = allEntries[i];
		const int slot          = i - from;
		const Point btnPos(BUTTON_X, BUTTON_START_Y + slot * BUTTON_STEP);

		auto btn = std::make_shared<CButton>(btnPos, be.animPath, CButton::tooltip(), [this](){ close(); }, be.shortcut);
		if(be.callback)
			btn->addCallback(be.callback);
		btn->block(be.isBlocked);

		if(!be.labelKey.empty())
		{
			const int margin = 5;
			const int bgX = 78;
			const int bgW = 189 - (enhanced ? 16 : 0);
			const int bgH = 48;
			buttonLabels.push_back(std::make_shared<CMultiLineLabel>(
				Rect(bgX + margin, BUTTON_START_Y + 1 + slot * BUTTON_STEP + margin, bgW - 2 * margin, bgH - 2 * margin),
				FONT_MEDIUM, ETextAlignment::CENTERLEFT, Colors::YELLOW,
				LIBRARY->generaltexth->translate(be.labelKey)));
		}

		buttons.push_back(btn);
	}

	redraw();
}

void AdventureOptions::showScenarioInfo()
{
	if(GAME->interface()->cb->getStartInfo()->campState)
	{
		ENGINE->windows().createAndPushWindow<CCampaignInfoScreen>();
	}
	else
	{
		ENGINE->windows().createAndPushWindow<CScenarioInfoScreen>();
	}
}
