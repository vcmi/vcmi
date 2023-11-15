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

#include "../widgets/ComboBox.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../CServerHandler.h"
#include "../CGameInfo.h"

#include "../../lib/StartInfo.h"
#include "../../lib/Languages.h"
#include "../../lib/MetaString.h"
#include "../../lib/CGeneralTextHandler.h"

OptionsTabBase::OptionsTabBase(const JsonPath & configPath)
{
	recActions = 0;

	addCallback("setTimerPreset", [&](int index){
		if(!variables["timerPresets"].isNull())
		{
			auto tpreset = variables["timerPresets"].Vector().at(index).Vector();
			TurnTimerInfo tinfo;
			tinfo.baseTimer = tpreset.at(0).Integer() * 1000;
			tinfo.turnTimer = tpreset.at(1).Integer() * 1000;
			tinfo.battleTimer = tpreset.at(2).Integer() * 1000;
			tinfo.creatureTimer = tpreset.at(3).Integer() * 1000;
			CSH->setTurnTimerInfo(tinfo);
		}
	});

	addCallback("setSimturnDurationMin", [&](int index){
		SimturnsInfo info = SEL->getStartInfo()->simturnsInfo;
		info.requiredTurns = index;
		info.optionalTurns = std::max(info.optionalTurns, index);
		CSH->setSimturnsInfo(info);
	});

	addCallback("setSimturnDurationMax", [&](int index){
		SimturnsInfo info = SEL->getStartInfo()->simturnsInfo;
		info.optionalTurns = index;
		info.requiredTurns = std::min(info.requiredTurns, index);
		CSH->setSimturnsInfo(info);
	});

	addCallback("setSimturnAI", [&](int index){
		SimturnsInfo info = SEL->getStartInfo()->simturnsInfo;
		info.allowHumanWithAI = index;
		CSH->setSimturnsInfo(info);
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

		return std::stoi(l) * 60 + std::stoi(r);
	};

	addCallback("parseAndSetTimer_base", [parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.baseTimer = time;
			CSH->setTurnTimerInfo(tinfo);
		}
	});
	addCallback("parseAndSetTimer_turn", [parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.turnTimer = time;
			CSH->setTurnTimerInfo(tinfo);
		}
	});
	addCallback("parseAndSetTimer_battle", [parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.battleTimer = time;
			CSH->setTurnTimerInfo(tinfo);
		}
	});
	addCallback("parseAndSetTimer_creature", [parseTimerString](const std::string & str){
		int time = parseTimerString(str) * 1000;
		if(time >= 0)
		{
			TurnTimerInfo tinfo = SEL->getStartInfo()->turnTimerInfo;
			tinfo.creatureTimer = time;
			CSH->setTurnTimerInfo(tinfo);
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
						tinfo.creatureTimer = (*tObj)["default"].Vector().at(3).Integer() * 1000;
						CSH->setTurnTimerInfo(tinfo);
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
}

void OptionsTabBase::recreate()
{
	auto const & generateSimturnsDurationText = [](int days) -> std::string
	{
		if (days == 0)
			return CGI->generaltexth->translate("core.genrltxt.523");

		if (days >= 1000000) // Not "unlimited" but close enough
			return CGI->generaltexth->translate("core.turndur.10");

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
		message.appendTextID(Languages::getPluralFormTextID( CGI->generaltexth->getPreferredLanguage(), value, text));
		message.replaceNumber(value);
		return message.toString();
	};

	//Simultaneous turns
	if(auto turnSlider = widget<CSlider>("simturnsDurationMin"))
		turnSlider->setValue(SEL->getStartInfo()->simturnsInfo.requiredTurns);

	if(auto turnSlider = widget<CSlider>("simturnsDurationMax"))
		turnSlider->setValue(SEL->getStartInfo()->simturnsInfo.optionalTurns);

	if(auto w = widget<CLabel>("labelSimturnsDurationValueMin"))
		w->setText(generateSimturnsDurationText(SEL->getStartInfo()->simturnsInfo.requiredTurns));

	if(auto w = widget<CLabel>("labelSimturnsDurationValueMax"))
		w->setText(generateSimturnsDurationText(SEL->getStartInfo()->simturnsInfo.optionalTurns));

	if(auto buttonSimturnsAI = widget<CToggleButton>("buttonSimturnsAI"))
		buttonSimturnsAI->setSelectedSilent(SEL->getStartInfo()->simturnsInfo.allowHumanWithAI);

	const auto & turnTimerRemote = SEL->getStartInfo()->turnTimerInfo;

	//classic timer
	if(auto turnSlider = widget<CSlider>("sliderTurnDuration"))
	{
		if(!variables["timerPresets"].isNull() && !turnTimerRemote.battleTimer && !turnTimerRemote.creatureTimer && !turnTimerRemote.baseTimer)
		{
			for(int idx = 0; idx < variables["timerPresets"].Vector().size(); ++idx)
			{
				auto & tpreset = variables["timerPresets"].Vector()[idx];
				if(tpreset.Vector().at(1).Integer() == turnTimerRemote.turnTimer / 1000)
				{
					turnSlider->scrollTo(idx);
					if(auto w = widget<CLabel>("labelTurnDurationValue"))
						w->setText(CGI->generaltexth->turnDurations[idx]);
				}
			}
		}
	}

	//chess timer
	auto timeToString = [](int time) -> std::string
	{
		std::stringstream ss;
		ss << time / 1000 / 60 << ":" << std::setw(2) << std::setfill('0') << time / 1000 % 60;
		return ss.str();
	};

	if(auto ww = widget<CTextInput>("chessFieldBase"))
		ww->setText(timeToString(turnTimerRemote.baseTimer), false);
	if(auto ww = widget<CTextInput>("chessFieldTurn"))
		ww->setText(timeToString(turnTimerRemote.turnTimer), false);
	if(auto ww = widget<CTextInput>("chessFieldBattle"))
		ww->setText(timeToString(turnTimerRemote.battleTimer), false);
	if(auto ww = widget<CTextInput>("chessFieldCreature"))
		ww->setText(timeToString(turnTimerRemote.creatureTimer), false);

	if(auto w = widget<ComboBox>("timerModeSwitch"))
	{
		if(turnTimerRemote.battleTimer || turnTimerRemote.creatureTimer || turnTimerRemote.baseTimer)
		{
			if(auto turnSlider = widget<CSlider>("sliderTurnDuration"))
				if(turnSlider->isActive())
					w->setItem(1);
		}
	}
}
