/*
 * GeneralOptionsTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GeneralOptionsTab.h"

#include "CPlayerInterface.h"
#include "CServerHandler.h"
#include "media/IMusicPlayer.h"
#include "media/ISoundPlayer.h"
#include "render/IScreenHandler.h"
#include "windows/GUIClasses.h"

#include "../../eventsSDL/InputHandler.h"
#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/Images.h"
#include "../../widgets/Slider.h"
#include "../../widgets/TextControls.h"

#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../../../lib/filesystem/ResourcePath.h"
#include "../../../lib/GameLibrary.h"

static void setIntSetting(std::string group, std::string field, int value)
{
	Settings entry = settings.write[group][field];
	entry->Float() = value;
}

static void setBoolSetting(std::string group, std::string field, bool value)
{
	Settings entry = settings.write[group][field];
	entry->Bool() = value;
}

static std::string scalingToEntryString( int scaling)
{
	return std::to_string(scaling) + '%';
}

static std::string scalingToLabelString( int scaling)
{
	std::string string = LIBRARY->generaltexth->translate("vcmi.systemOptions.scalingButton.hover");
	boost::replace_all(string, "%p", std::to_string(scaling));

	return string;
}

static std::string longTouchToEntryString( int duration)
{
	std::string string = LIBRARY->generaltexth->translate("vcmi.systemOptions.longTouchMenu.entry");
	boost::replace_all(string, "%d", std::to_string(duration));

	return string;
}

static std::string longTouchToLabelString( int duration)
{
	std::string string = LIBRARY->generaltexth->translate("vcmi.systemOptions.longTouchButton.hover");
	boost::replace_all(string, "%d", std::to_string(duration));

	return string;
}

static std::string resolutionToEntryString( int w, int h)
{
	std::string string = "%wx%h";

	boost::replace_all(string, "%w", std::to_string(w));
	boost::replace_all(string, "%h", std::to_string(h));

	return string;
}

static std::string resolutionToLabelString( int w, int h)
{
	std::string string = LIBRARY->generaltexth->translate("vcmi.systemOptions.resolutionButton.hover");

	boost::replace_all(string, "%w", std::to_string(w));
	boost::replace_all(string, "%h", std::to_string(h));

	return string;
}

GeneralOptionsTab::GeneralOptionsTab()
		: InterfaceObjectConfigurable(),
		  onFullscreenChanged(settings.listen["video"]["fullscreen"])
{
	OBJECT_CONSTRUCTION;
	setRedrawParent(true);

	addConditional("touchscreen", ENGINE->input().getCurrentInputMode() == InputMode::TOUCH);
	addConditional("keyboardMouse", ENGINE->input().getCurrentInputMode() == InputMode::KEYBOARD_AND_MOUSE);
	addConditional("controller", ENGINE->input().getCurrentInputMode() == InputMode::CONTROLLER);
#ifdef VCMI_MOBILE
	addConditional("mobile", true);
	addConditional("desktop", false);
#else
	addConditional("mobile", false);
	addConditional("desktop", true);
#endif

	const JsonNode config(JsonPath::builtin("config/widgets/settings/generalOptionsTab.json"));
	addCallback("spellbookAnimationChanged", [](bool value)
	{
		setBoolSetting("video", "spellbookAnimation", value);
	});
	addCallback("setMusic", [this](int value)
	{
		setIntSetting("general", "music", value);
		widget<CSlider>("musicSlider")->redraw();

		auto targetLabel = widget<CLabel>("musicValueLabel");
		if (targetLabel)
			targetLabel->setText(std::to_string(value) + "%");
	});
	addCallback("setVolume", [this](int value)
	{
		setIntSetting("general", "sound", value);
		widget<CSlider>("soundVolumeSlider")->redraw();

		auto targetLabel = widget<CLabel>("soundValueLabel");
		if (targetLabel)
			targetLabel->setText(std::to_string(value) + "%");
	});
	//settings that do not belong to base game:
	addCallback("fullscreenBorderlessChanged", [this](bool value)
	{
		setFullscreenMode(value, false);
	});
	addCallback("fullscreenExclusiveChanged", [this](bool value)
	{
		setFullscreenMode(value, true);
	});
	addCallback("setGameResolution", [this](int dummyValue)
	{
		selectGameResolution();
	});
	addCallback("setGameScaling", [this](int dummyValue)
	{
		selectGameScaling();
	});
	addCallback("setLongTouchDuration", [this](int dummyValue)
	{
		selectLongTouchDuration();
	});
	addCallback("performanceOverlayChanged", [](bool value)
	{
		Settings gameRes = settings.write["video"]["performanceOverlay"];
		gameRes["show"].Bool() = value;
	});
	addCallback("hapticFeedbackChanged", [](bool value)
	{
		setBoolSetting("general", "hapticFeedback", value);
	});
	addCallback("enableOverlayChanged", [](bool value)
	{
		setBoolSetting("general", "enableOverlay", value);
	});
	addCallback("enableUiEnhancementsChanged", [](bool value)
	{
		setBoolSetting("general", "enableUiEnhancements", value);
	});

	addCallback("enableLargeSpellbookChanged", [this](bool value)
	{
		setBoolSetting("gameTweaks", "enableLargeSpellbook", value);
		std::shared_ptr<CToggleButton> spellbookAnimationCheckbox = widget<CToggleButton>("spellbookAnimationCheckbox");
		if(value)
			spellbookAnimationCheckbox->disable();
		else
			spellbookAnimationCheckbox->enable();
		redraw();
	});

	addCallback("audioMuteFocusChanged", [](bool value)
	{
		setBoolSetting("general", "audioMuteFocus", value);
	});

	addCallback("enableSubtitleChanged", [](bool value)
	{
		setBoolSetting("general", "enableSubtitle", value);
	});

	//moved from "other" tab that is disabled for now to avoid excessible tabs with barely any content
	addCallback("availableCreaturesAsDwellingChanged", [=](int value)
	{
		setBoolSetting("gameTweaks", "availableCreaturesAsDwellingLabel", value > 0);
	});

	addCallback("compactTownCreatureInfoChanged", [](bool value)
	{
		setBoolSetting("gameTweaks", "compactTownCreatureInfo", value);
	});

	build(config);

	std::shared_ptr<CLabel> scalingLabel = widget<CLabel>("scalingLabel");
	scalingLabel->setText(scalingToLabelString(ENGINE->screenHandler().getInterfaceScalingPercentage()));

	std::shared_ptr<CLabel> longTouchLabel = widget<CLabel>("longTouchLabel");
	if (longTouchLabel)
		longTouchLabel->setText(longTouchToLabelString(settings["general"]["longTouchTimeMilliseconds"].Integer()));

	std::shared_ptr<CToggleButton> spellbookAnimationCheckbox = widget<CToggleButton>("spellbookAnimationCheckbox");
	spellbookAnimationCheckbox->setSelected(settings["video"]["spellbookAnimation"].Bool());
	if(settings["gameTweaks"]["enableLargeSpellbook"].Bool())
		spellbookAnimationCheckbox->disable();
	else
		spellbookAnimationCheckbox->enable();

	std::shared_ptr<CToggleButton> fullscreenBorderlessCheckbox = widget<CToggleButton>("fullscreenBorderlessCheckbox");
	if (fullscreenBorderlessCheckbox)
		fullscreenBorderlessCheckbox->setSelected(settings["video"]["fullscreen"].Bool() && !settings["video"]["realFullscreen"].Bool());

	std::shared_ptr<CToggleButton> fullscreenExclusiveCheckbox = widget<CToggleButton>("fullscreenExclusiveCheckbox");
	if (fullscreenExclusiveCheckbox)
		fullscreenExclusiveCheckbox->setSelected(settings["video"]["fullscreen"].Bool() && settings["video"]["realFullscreen"].Bool());

	std::shared_ptr<CToggleButton> infoboxCheckbox = widget<CToggleButton>("performanceOverlayCheckbox");
	infoboxCheckbox->setSelected(settings["video"]["performanceOverlay"]["show"].Bool());

	std::shared_ptr<CToggleButton> hapticFeedbackCheckbox = widget<CToggleButton>("hapticFeedbackCheckbox");
	if (hapticFeedbackCheckbox)
		hapticFeedbackCheckbox->setSelected(settings["general"]["hapticFeedback"].Bool());

	std::shared_ptr<CToggleButton> enableOverlayCheckbox = widget<CToggleButton>("enableOverlayCheckbox");
	if (enableOverlayCheckbox)
		enableOverlayCheckbox->setSelected(settings["general"]["enableOverlay"].Bool());

	std::shared_ptr<CToggleButton> enableUiEnhancementsCheckbox = widget<CToggleButton>("enableUiEnhancementsCheckbox");
	if (enableUiEnhancementsCheckbox)
		enableUiEnhancementsCheckbox->setSelected(settings["general"]["enableUiEnhancements"].Bool());

	std::shared_ptr<CToggleButton> enableLargeSpellbookCheckbox = widget<CToggleButton>("enableLargeSpellbookCheckbox");
	if (enableLargeSpellbookCheckbox)
		enableLargeSpellbookCheckbox->setSelected(settings["gameTweaks"]["enableLargeSpellbook"].Bool());

	std::shared_ptr<CToggleButton> audioMuteFocusCheckbox = widget<CToggleButton>("audioMuteFocusCheckbox");
	if (audioMuteFocusCheckbox)
		audioMuteFocusCheckbox->setSelected(settings["general"]["audioMuteFocus"].Bool());

	std::shared_ptr<CToggleButton> enableSubtitleCheckbox = widget<CToggleButton>("enableSubtitleCheckbox");
	if (enableSubtitleCheckbox)
		enableSubtitleCheckbox->setSelected(settings["general"]["enableSubtitle"].Bool());

	std::shared_ptr<CSlider> musicSlider = widget<CSlider>("musicSlider");
	musicSlider->scrollTo(ENGINE->music().getVolume());

	std::shared_ptr<CSlider> volumeSlider = widget<CSlider>("soundVolumeSlider");
	volumeSlider->scrollTo(ENGINE->sound().getVolume());

	std::shared_ptr<CToggleGroup> creatureGrowthAsDwellingPicker = widget<CToggleGroup>("availableCreaturesAsDwellingPicker");
	creatureGrowthAsDwellingPicker->setSelected(settings["gameTweaks"]["availableCreaturesAsDwellingLabel"].Bool());

	std::shared_ptr<CToggleButton> compactTownCreatureInfo = widget<CToggleButton>("compactTownCreatureInfoCheckbox");
	compactTownCreatureInfo->setSelected(settings["gameTweaks"]["compactTownCreatureInfo"].Bool());

	std::shared_ptr<CLabel> musicVolumeLabel = widget<CLabel>("musicValueLabel");
	musicVolumeLabel->setText(std::to_string(ENGINE->music().getVolume()) + "%");

	std::shared_ptr<CLabel> soundVolumeLabel = widget<CLabel>("soundValueLabel");
	soundVolumeLabel->setText(std::to_string(ENGINE->sound().getVolume()) + "%");

	updateResolutionSelector();
}

void GeneralOptionsTab::updateResolutionSelector()
{
	std::shared_ptr<CButton> resolutionButton = widget<CButton>("resolutionButton");
	std::shared_ptr<CLabel> resolutionLabel = widget<CLabel>("resolutionLabel");

	if (resolutionButton)
	{
		if (settings["video"]["fullscreen"].Bool() && !settings["video"]["realFullscreen"].Bool())
			resolutionButton->disable();
		else
			resolutionButton->enable();
	}

	if (resolutionLabel)
	{
		Point resolution = ENGINE->screenHandler().getRenderResolution();
		resolutionLabel->setText(resolutionToLabelString(resolution.x, resolution.y));
	}
}

void GeneralOptionsTab::selectGameResolution()
{
	supportedResolutions = ENGINE->screenHandler().getSupportedResolutions();

	std::vector<std::string> items;
	size_t currentResolutionIndex = 0;
	size_t i = 0;
	for(const auto & it : supportedResolutions)
	{
		auto resolutionStr = resolutionToEntryString(it.x, it.y);
		if(widget<CLabel>("resolutionLabel")->getText() == resolutionToLabelString(it.x, it.y))
			currentResolutionIndex = i;

		items.push_back(std::move(resolutionStr));
		++i;
	}
	ENGINE->windows().createAndPushWindow<CObjectListWindow>(items, nullptr,
								   LIBRARY->generaltexth->translate("vcmi.systemOptions.resolutionMenu.hover"),
								   LIBRARY->generaltexth->translate("vcmi.systemOptions.resolutionMenu.help"),
								   [this](int index)
								   {
									   setGameResolution(index);
								   },
								   currentResolutionIndex);
}

void GeneralOptionsTab::setGameResolution(int index)
{
	assert(index >= 0 && index < supportedResolutions.size());

	if ( index < 0 || index >= supportedResolutions.size() )
		return;

	Point resolution = supportedResolutions[index];

	Settings gameRes = settings.write["video"]["resolution"];
	gameRes["width"].Float() = resolution.x;
	gameRes["height"].Float() = resolution.y;

	widget<CLabel>("resolutionLabel")->setText(resolutionToLabelString(resolution.x, resolution.y));

	ENGINE->dispatchMainThread([](){
		ENGINE->onScreenResize(true);
	});
}

void GeneralOptionsTab::setFullscreenMode(bool on, bool exclusive)
{
	if (on == settings["video"]["fullscreen"].Bool() && exclusive == settings["video"]["realFullscreen"].Bool())
		return;

	setBoolSetting("video", "realFullscreen", exclusive);
	setBoolSetting("video", "fullscreen", on);

	std::shared_ptr<CToggleButton> fullscreenExclusiveCheckbox = widget<CToggleButton>("fullscreenExclusiveCheckbox");
	std::shared_ptr<CToggleButton> fullscreenBorderlessCheckbox = widget<CToggleButton>("fullscreenBorderlessCheckbox");

	if (fullscreenBorderlessCheckbox)
		fullscreenBorderlessCheckbox->setSelectedSilent(on && !exclusive);

	if (fullscreenExclusiveCheckbox)
		fullscreenExclusiveCheckbox->setSelectedSilent(on && exclusive);

	updateResolutionSelector();

	ENGINE->dispatchMainThread([](){
		ENGINE->onScreenResize(true);
	});
}

void GeneralOptionsTab::selectGameScaling()
{
	supportedScaling.clear();

	// generate list of all possible scaling values, with 10% step
	// also add one value over maximum, so if player can use scaling up to 123.456% he will be able to select 130%
	// and let screen handler clamp that value to actual maximum
	auto [minimalScaling, maximalScaling] = ENGINE->screenHandler().getSupportedScalingRange();
	for (int i = 0; i <= maximalScaling + 10 - 1; i += 10)
	{
		if (i >= minimalScaling)
			supportedScaling.push_back(i);
	}

	std::vector<std::string> items;
	size_t currentIndex = 0;
	size_t i = 0;
	for(const auto & it : supportedScaling)
	{
		auto resolutionStr = scalingToEntryString(it);
		if(widget<CLabel>("scalingLabel")->getText() == scalingToLabelString(it))
			currentIndex = i;

		items.push_back(std::move(resolutionStr));
		++i;
	}

	ENGINE->windows().createAndPushWindow<CObjectListWindow>(
		items,
		nullptr,
		LIBRARY->generaltexth->translate("vcmi.systemOptions.scalingMenu.hover"),
		LIBRARY->generaltexth->translate("vcmi.systemOptions.scalingMenu.help"),
		[this](int index)
		{
			setGameScaling(index);
		},
		currentIndex
	);
}

void GeneralOptionsTab::setGameScaling(int index)
{
	assert(index >= 0 && index < supportedScaling.size());

	if ( index < 0 || index >= supportedScaling.size() )
		return;

	int scaling = supportedScaling[index];

	Settings gameRes = settings.write["video"]["resolution"];
	gameRes["scaling"].Float() = scaling;

	widget<CLabel>("scalingLabel")->setText(scalingToLabelString(scaling));

	ENGINE->dispatchMainThread([](){
		ENGINE->onScreenResize(true);
	});
}

void GeneralOptionsTab::selectLongTouchDuration()
{
	longTouchDurations = { 500, 750, 1000, 1250, 1500, 1750, 2000 };

	std::vector<std::string> items;
	size_t currentIndex = 0;
	size_t i = 0;
	for(const auto & it : longTouchDurations)
	{
		auto resolutionStr = longTouchToEntryString(it);
		if(widget<CLabel>("longTouchLabel")->getText() == longTouchToLabelString(it))
			currentIndex = i;

		items.push_back(std::move(resolutionStr));
		++i;
	}

	ENGINE->windows().createAndPushWindow<CObjectListWindow>(
		items,
		nullptr,
		LIBRARY->generaltexth->translate("vcmi.systemOptions.longTouchMenu.hover"),
		LIBRARY->generaltexth->translate("vcmi.systemOptions.longTouchMenu.help"),
		[this](int index)
		{
			setLongTouchDuration(index);
		},
		currentIndex
	);
}

void GeneralOptionsTab::setLongTouchDuration(int index)
{
	assert(index >= 0 && index < longTouchDurations.size());

	if ( index < 0 || index >= longTouchDurations.size() )
		return;

	int scaling = longTouchDurations[index];

	Settings longTouchTime = settings.write["general"]["longTouchTimeMilliseconds"];
	longTouchTime->Float() = scaling;

	widget<CLabel>("longTouchLabel")->setText(longTouchToLabelString(scaling));
}
