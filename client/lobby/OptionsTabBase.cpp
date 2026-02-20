/*
 * OptionsTabBase.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "OptionsTabBase.h"
#include "CSelectionBase.h"
#include "TurnOptionsTab.h"
#include "CLobbyScreen.h"

#include "../widgets/ComboBox.h"
#include "../widgets/CTextInput.h"
#include "../widgets/Images.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../CServerHandler.h"
#include "../GameInstance.h"

#include "../../lib/StartInfo.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/Languages.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/GameLibrary.h"

static std::string timeToString(int time)
{
	std::stringstream ss;
	ss << time / 1000 / 60 << ":" << std::setw(2) << std::setfill('0') << time / 1000 % 60;
	return ss.str();
};


std::vector<TurnTimerInfo> OptionsTabBase::getTimerPresets() const
{
	std::vector<TurnTimerInfo> result;

	for (auto const & tpreset : variables["timerPresets"].Vector())
	{
		TurnTimerInfo tinfo;
		tinfo.baseTimer = tpreset[0].Integer() * 1000;
		tinfo.turnTimer = tpreset[1].Integer() * 1000;
		tinfo.battleTimer = tpreset[2].Integer() * 1000;
		tinfo.unitTimer = tpreset[3].Integer() * 1000;
		tinfo.accumulatingTurnTimer = tpreset[4].Bool();
		tinfo.accumulatingUnitTimer = tpreset[5].Bool();
		result.push_back(tinfo);
	}
	return result;
}

std::vector<SimturnsInfo> OptionsTabBase::getSimturnsPresets() const
{
	std::vector<SimturnsInfo> result;

	for (auto const & tpreset : variables["simturnsPresets"].Vector())
	{
		SimturnsInfo tinfo;
		tinfo.optionalTurns = tpreset[0].Integer();
		tinfo.requiredTurns = tpreset[1].Integer();
		tinfo.allowHumanWithAI = tpreset[2].Bool();
		result.push_back(tinfo);
	}
	return result;
}

OptionsTabBase::OptionsTabBase(const JsonPath & configPath)
{
	recActions = 0;

	auto setTimerPresetCallback = [this](int index){
		GAME->server().setTurnTimerInfo(getTimerPresets().at(index));
	};

	auto setSimturnsPresetCallback = [this](int index){
		GAME->server().setSimturnsInfo(getSimturnsPresets().at(index));
	};

	addCallback("tabTurnOptions", [&](int)
	{
		auto lobby = (static_cast<CLobbyScreen *>(parent));
		lobby->toggleTab(lobby->tabTurnOptions);
	});

	addCallback("setTimerPreset", setTimerPresetCallback);
	addCallback("setSimturnPreset", setSimturnsPresetCallback);

	addCallback("setSimturnDurationMin", [&](int index){
		SimturnsInfo info = SEL->getStartInfo()->simturnsInfo;
		info.requiredTurns = index;
		info.optionalTurns = std::max(info.optionalTurns, index);
		GAME->server().setSimturnsInfo(info);
	});

	addCallback("setSimturnDurationMax", [&](int index){
		SimturnsInfo info = SEL->getStartInfo()->simturnsInfo;
		info.optionalTurns = index;
		info.requiredTurns = std::min(info.requiredTurns, index);
		GAME->server().setSimturnsInfo(info);
	});

	addCallback("setSimturnAI", [&](int index){
		SimturnsInfo info = SEL->getStartInfo()->simturnsInfo;
		info.allowHumanWithAI = index;
		GAME->server().setSimturnsInfo(info);
	});

	addCallback("setCheatAllowed", [&](int index){
		bool isMultiplayer = GAME->server().loadMode == ELoadMode::MULTI;
		Settings entry = persistentStorage.write["startExtraOptions"][isMultiplayer ? "multiPlayer" : "singlePlayer"][isMultiplayer ? "cheatsAllowed" : "cheatsNotAllowed"];
		entry->Bool() = isMultiplayer ? index : !index;
		ExtraOptionsInfo info = SEL->getStartInfo()->extraOptionsInfo;
		info.cheatsAllowed = index;
		GAME->server().setExtraOptionsInfo(info);
	});

	addCallback("setUnlimitedReplay", [&](int index){
		bool isMultiplayer = GAME->server().loadMode == ELoadMode::MULTI;
		Settings entry = persistentStorage.write["startExtraOptions"][isMultiplayer ? "multiPlayer" : "singlePlayer"]["unlimitedReplay"];
		entry->Bool() = index;
		ExtraOptionsInfo info = SEL->getStartInfo()->extraOptionsInfo;
		info.unlimitedReplay = index;
		GAME->server().setExtraOptionsInfo(info);
	});

	addCallback("setTurnTimerAccumulate", [&](int index){
		TurnTimerInfo info = SEL->getStartInfo()->turnTimerInfo;
		info.accumulatingTurnTimer = index;
		GAME->server().setTurnTimerInfo(info);
	});

	addCallback("setUnitTimerAccumulate", [&](int index){
		TurnTimerInfo info = SEL->getStartInfo()->turnTimerInfo;
		info.accumulatingUnitTimer = index;
		GAME->server().setTurnTimerInfo(info);
	});

	//helper function to parse string containing time to integer reflecting time in seconds
	//assumed that input string can be modified by user, function shall support user's intention
	// normal: 2:00, 12:30
	// adding symbol: 2:005 -> 2:05, 2:305 -> 23:05,
	// adding symbol (>60 seconds): 12:095 -> 129:05
	// removing symbol: 129:0 -> 12:09, 2:0 -> 0:20, 0:2 -> 0:02
	auto parseTimerString = [](const std::string & str) -> int
	{
		auto sc = str.find(":");
		if(sc == std::string::npos)
			return str.empty() ? 0 : std::stoi(str);

		auto l = str.substr(0, sc);
		auto r = str.substr(sc + 1, std::string::npos);
		if(r.length() == 3) //symbol added
		{
			l.push_back(r.front());
			r.erase(r.begin());
		}
		else if(r.length() == 1) //symbol removed
		{
			r.insert(r.begin(), l.back());
			l.pop_back();
		}
		else if(r.empty())
			r = "0";

		int sec = std::stoi(r);
		if(sec >= 60)
		{
			if(l.empty()) //9:00 -> 0:09
				return sec / 10;

			l.push_back(r.front()); //0:090 -> 9:00
			r.erase(r.begin());
		}
		else if(l.empty())
			return sec;

		return std::min(24*60, std::stoi(l)) * 60 + std::stoi(r);
	};

	addCallback("parseAndSetTimer_base", [this, parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.baseTimer = time;
			GAME->server().setTurnTimerInfo(tinfo);
			if(auto ww = widget<CTextInput>("chessFieldBase"))
				ww->setText(timeToString(time));
		}
	});
	addCallback("parseAndSetTimer_turn", [this, parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.turnTimer = time;
			GAME->server().setTurnTimerInfo(tinfo);
			if(auto ww = widget<CTextInput>("chessFieldTurn"))
				ww->setText(timeToString(time));
		}
	});
	addCallback("parseAndSetTimer_battle", [this, parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.battleTimer = time;
			GAME->server().setTurnTimerInfo(tinfo);
			if(auto ww = widget<CTextInput>("chessFieldBattle"))
				ww->setText(timeToString(time));
		}
	});
	addCallback("parseAndSetTimer_unit", [this, parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.unitTimer = time;
			GAME->server().setTurnTimerInfo(tinfo);
			if(auto ww = widget<CTextInput>("chessFieldUnit"))
				ww->setText(timeToString(time));
		}
	});

	const JsonNode config(configPath);
	build(config);

	//set timers combo box callbacks
	if(auto w = widget<ComboBox>("timerModeSwitch"))
	{
		w->onConstructItems = [&](std::vector<const void *> & curItems){
			if(variables["timers"].isNull())
				return;

			for(auto & p : variables["timers"].Vector())
			{
				curItems.push_back(&p);
			}
		};

		w->onSetItem = [&](const void * item){
			if(item)
			{
				if(auto * tObj = reinterpret_cast<const JsonNode *>(item))
				{
					for(auto wname : (*tObj)["hideWidgets"].Vector())
					{
						if(auto w = widget<CIntObject>(wname.String()))
							w->setEnabled(false);
					}
					for(auto wname : (*tObj)["showWidgets"].Vector())
					{
						if(auto w = widget<CIntObject>(wname.String()))
							w->setEnabled(true);
					}
					if((*tObj)["default"].isVector())
					{
						TurnTimerInfo tinfo;
						tinfo.baseTimer = (*tObj)["default"].Vector().at(0).Integer() * 1000;
						tinfo.turnTimer = (*tObj)["default"].Vector().at(1).Integer() * 1000;
						tinfo.battleTimer = (*tObj)["default"].Vector().at(2).Integer() * 1000;
						tinfo.unitTimer = (*tObj)["default"].Vector().at(3).Integer() * 1000;
						GAME->server().setTurnTimerInfo(tinfo);
					}
				}
				redraw();
			}
		};

		w->getItemText = [this](int idx, const void * item){
			if(item)
			{
				if(auto * tObj = reinterpret_cast<const JsonNode *>(item))
					return readText((*tObj)["text"]);
			}
			return std::string("");
		};

		w->setItem(0);
	}

	if(auto w = widget<ComboBox>("simturnsPresetSelector"))
	{
		w->onConstructItems = [this](std::vector<const void *> & curItems)
		{
			for (size_t i = 0; i < variables["simturnsPresets"].Vector().size(); ++i)
				curItems.push_back((void*)i);
		};

		w->onSetItem = [setSimturnsPresetCallback](const void * item){
			size_t itemIndex = (size_t)item;
			setSimturnsPresetCallback(itemIndex);
		};
	}

	if(auto w = widget<ComboBox>("timerPresetSelector"))
	{
		w->onConstructItems = [this](std::vector<const void *> & curItems)
		{
			for (size_t i = 0; i < variables["timerPresets"].Vector().size(); ++i)
				curItems.push_back((void*)i);
		};

		w->onSetItem = [setTimerPresetCallback](const void * item){
			size_t itemIndex = (size_t)item;
			setTimerPresetCallback(itemIndex);
		};
	}
}

void OptionsTabBase::recreate(bool campaign)
{
	auto const & generateSimturnsDurationText = [](int days) -> std::string
	{
		if (days == 0)
			return LIBRARY->generaltexth->translate("core.genrltxt.523");

		if (days >= 1000000) // Not "unlimited" but close enough
			return LIBRARY->generaltexth->translate("core.turndur.10");

		bool canUseMonth = days % 28 == 0 && days >= 28*2;
		bool canUseWeek = days % 7 == 0 && days >= 7*2;

		int value = days;
		std::string text = "vcmi.optionsTab.simturns.days";

		if (canUseWeek && !canUseMonth)
		{
			value = days / 7;
			text = "vcmi.optionsTab.simturns.weeks";
		}

		if (canUseMonth)
		{
			value = days / 28;
			text = "vcmi.optionsTab.simturns.months";
		}

		MetaString message;
		message.appendTextID(Languages::getPluralFormTextID( LIBRARY->generaltexth->getPreferredLanguage(), value, text));
		message.replaceNumber(value);
		return message.toString();
	};

	//Simultaneous turns
	if(auto turnSlider = widget<CSlider>("simturnsDurationMin"))
		turnSlider->setValue(SEL->getStartInfo()->simturnsInfo.requiredTurns, false);

	if(auto turnSlider = widget<CSlider>("simturnsDurationMax"))
		turnSlider->setValue(SEL->getStartInfo()->simturnsInfo.optionalTurns, false);

	if(auto w = widget<CLabel>("labelSimturnsDurationValueMin"))
		w->setText(generateSimturnsDurationText(SEL->getStartInfo()->simturnsInfo.requiredTurns));

	if(auto w = widget<CLabel>("labelSimturnsDurationValueMax"))
		w->setText(generateSimturnsDurationText(SEL->getStartInfo()->simturnsInfo.optionalTurns));

	if(auto buttonSimturnsAI = widget<CToggleButton>("buttonSimturnsAI"))
		buttonSimturnsAI->setSelectedSilent(SEL->getStartInfo()->simturnsInfo.allowHumanWithAI);

	if(auto buttonTurnTimerAccumulate = widget<CToggleButton>("buttonTurnTimerAccumulate"))
		buttonTurnTimerAccumulate->setSelectedSilent(SEL->getStartInfo()->turnTimerInfo.accumulatingTurnTimer);

	if(auto chessFieldTurnLabel = widget<CLabel>("chessFieldTurnLabel"))
	{
		if (SEL->getStartInfo()->turnTimerInfo.accumulatingTurnTimer)
			chessFieldTurnLabel->setText(LIBRARY->generaltexth->translate("vcmi.optionsTab.chessFieldTurnAccumulate.help"));
		else
			chessFieldTurnLabel->setText(LIBRARY->generaltexth->translate("vcmi.optionsTab.chessFieldTurnDiscard.help"));
	}

	if(auto chessFieldUnitLabel = widget<CLabel>("chessFieldUnitLabel"))
	{
		if (SEL->getStartInfo()->turnTimerInfo.accumulatingUnitTimer)
			chessFieldUnitLabel->setText(LIBRARY->generaltexth->translate("vcmi.optionsTab.chessFieldUnitAccumulate.help"));
		else
			chessFieldUnitLabel->setText(LIBRARY->generaltexth->translate("vcmi.optionsTab.chessFieldUnitDiscard.help"));
	}

	if(auto buttonUnitTimerAccumulate = widget<CToggleButton>("buttonUnitTimerAccumulate"))
		buttonUnitTimerAccumulate->setSelectedSilent(SEL->getStartInfo()->turnTimerInfo.accumulatingUnitTimer);

	const auto & turnTimerRemote = SEL->getStartInfo()->turnTimerInfo;

	//classic timer
	if(auto turnSlider = widget<CSlider>("sliderTurnDuration"))
	{
		if(!variables["timerPresets"].isNull() && !turnTimerRemote.battleTimer && !turnTimerRemote.unitTimer && !turnTimerRemote.baseTimer)
		{
			for(int idx = 0; idx < variables["timerPresets"].Vector().size(); ++idx)
			{
				auto & tpreset = variables["timerPresets"].Vector()[idx];
				if(tpreset.Vector().at(1).Integer() == turnTimerRemote.turnTimer / 1000)
				{
					turnSlider->scrollTo(idx, false);
					if(auto w = widget<CLabel>("labelTurnDurationValue"))
						w->setText(LIBRARY->generaltexth->turnDurations[idx]);
				}
			}
		}
	}

	if(auto ww = widget<CTextInput>("chessFieldBase"))
		ww->setText(timeToString(turnTimerRemote.baseTimer));
	if(auto ww = widget<CTextInput>("chessFieldTurn"))
		ww->setText(timeToString(turnTimerRemote.turnTimer));
	if(auto ww = widget<CTextInput>("chessFieldBattle"))
		ww->setText(timeToString(turnTimerRemote.battleTimer));
	if(auto ww = widget<CTextInput>("chessFieldUnit"))
		ww->setText(timeToString(turnTimerRemote.unitTimer));

	if(auto w = widget<ComboBox>("timerModeSwitch"))
	{
		if(turnTimerRemote.battleTimer || turnTimerRemote.unitTimer || turnTimerRemote.baseTimer)
		{
			if(auto turnSlider = widget<CSlider>("sliderTurnDuration"))
				if(turnSlider->isActive())
					w->setItem(1);
		}
	}

	if(auto buttonCheatAllowed = widget<CToggleButton>("buttonCheatAllowed"))
	{
		buttonCheatAllowed->setSelectedSilent(SEL->getStartInfo()->extraOptionsInfo.cheatsAllowed);
		buttonCheatAllowed->block(GAME->server().isGuest());
	}

	if(auto buttonUnlimitedReplay = widget<CToggleButton>("buttonUnlimitedReplay"))
	{
		buttonUnlimitedReplay->setSelectedSilent(SEL->getStartInfo()->extraOptionsInfo.unlimitedReplay);
		buttonUnlimitedReplay->block(GAME->server().isGuest());
	}

	if(auto buttonTurnOptions = widget<CButton>("buttonTurnOptions"))
	{
		buttonTurnOptions->block(GAME->server().isGuest() || campaign);
	}

	if(auto textureCampaignOverdraw = widget<CFilledTexture>("textureCampaignOverdraw"))
		textureCampaignOverdraw->setEnabled(campaign);
}
