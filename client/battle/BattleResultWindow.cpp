/*
 * BattleResultWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleResultWindow.h"

#include "BattleWindow.h"

#include "../GameInstance.h"
#include "../Client.h"
#include "../CServerHandler.h"
#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../media/IMusicPlayer.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../widgets/VideoWidget.h"

#include "../../lib/CStack.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/ConditionalWait.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/StartInfo.h"
#include "../../lib/battle/CPlayerBattleCallback.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/gameState/InfoAboutArmy.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/texts/CGeneralTextHandler.h"

BattleResultWindow::BattleResultWindow(const BattleResult & br, CPlayerInterface & _owner, bool allowReplay)
	: owner(_owner)
{
	OBJECT_CONSTRUCTION;

	background = std::make_shared<CPicture>(ImagePath::builtin("CPRESULT"));
	background->setPlayerColor(owner.playerID);
	pos = center(background->pos);

	exit = std::make_shared<CButton>(Point(384, 505), AnimationPath::builtin("iok6432.def"), std::make_pair("", ""), [this](){ bExitf();}, EShortcut::GLOBAL_ACCEPT);
	exit->setBorderColor(Colors::METALLIC_GOLD);

	auto battle = owner.cb->getBattle(br.battleID);
	const auto * attackerPlayer = GAME->server().client->gameInfo().getPlayerState(battle->sideToPlayer(BattleSide::ATTACKER));
	const auto * defenderPlayer = GAME->server().client->gameInfo().getPlayerState(battle->sideToPlayer(BattleSide::DEFENDER));
	bool isAttackerHuman = attackerPlayer && attackerPlayer->isHuman();
	bool isDefenderHuman = defenderPlayer && defenderPlayer->isHuman();
	bool onlyOnePlayerHuman = isAttackerHuman != isDefenderHuman;

	if((allowReplay || owner.cb->getStartInfo()->extraOptionsInfo.unlimitedReplay) && onlyOnePlayerHuman)
	{
		repeat = std::make_shared<CButton>(Point(24, 505), AnimationPath::builtin("icn6432.def"), std::make_pair("", ""), [this](){ bRepeatf();}, EShortcut::GLOBAL_CANCEL);
		repeat->setBorderColor(Colors::METALLIC_GOLD);
		labels.push_back(std::make_shared<CLabel>(232, 520, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.battleResultsWindow.applyResultsLabel")));
	}

	if(br.winner == BattleSide::ATTACKER)
	{
		labels.push_back(std::make_shared<CLabel>(59, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[410]));
	}
	else
	{
		labels.push_back(std::make_shared<CLabel>(59, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[411]));
	}

	if(br.winner == BattleSide::DEFENDER)
	{
		labels.push_back(std::make_shared<CLabel>(412, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[410]));
	}
	else
	{
		labels.push_back(std::make_shared<CLabel>(408, 124, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[411]));
	}

	labels.push_back(std::make_shared<CLabel>(232, 302, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,  LIBRARY->generaltexth->allTexts[407]));
	labels.push_back(std::make_shared<CLabel>(232, 332, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[408]));
	labels.push_back(std::make_shared<CLabel>(232, 428, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[409]));

	std::array<std::string, 2> sideNames = {"N/A", "N/A"};

	for(auto i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		auto heroInfo = owner.cb->getBattle(br.battleID)->battleGetHeroInfo(i);
		const std::array xs = {21, 392};

		if(heroInfo.portraitSource.isValid()) //attacking hero
		{
			icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), heroInfo.getIconIndex(), 0, xs[static_cast<int>(i)], 38));
			sideNames[static_cast<int>(i)] = heroInfo.name;
		}
		else
		{
			auto stacks = owner.cb->getBattle(br.battleID)->battleGetAllStacks();
			vstd::erase_if(stacks, [i](const CStack * stack) //erase stack of other side and not coming from garrison
						   {
							   return stack->unitSide() != i || !stack->base;
						   });

			auto best = vstd::maxElementByFun(stacks, [](const CStack * stack)
											  {
												  return stack->unitType()->getAIValue();
											  });

			if(best != stacks.end()) //should be always but to be safe...
			{
				icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("TWCRPORT"), (*best)->unitType()->getIconIndex(), 0, xs[static_cast<int>(i)], 38));
				sideNames[static_cast<int>(i)] = (*best)->unitType()->getNamePluralTranslated();
			}
		}
	}

	//printing attacker and defender's names
	labels.push_back(std::make_shared<CLabel>(89, 37, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, sideNames[0]));
	labels.push_back(std::make_shared<CLabel>(381, 53, FONT_SMALL, ETextAlignment::BOTTOMRIGHT, Colors::WHITE, sideNames[1]));

	//printing casualties
	for(auto step : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		if(br.casualties[step].empty())
		{
			labels.push_back(std::make_shared<CLabel>(235, 360 + 97 * static_cast<int>(step), FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[523]));
		}
		else
		{
			int casualties = br.casualties[step].size();
			int xPos = 235 - (casualties*32 + (casualties - 1)*10)/2; //increment by 42 with each picture
			int yPos = 344 + static_cast<int>(step) * 97;
			for(auto & elem : br.casualties[step])
			{
				const auto * creature = elem.first.toEntity(LIBRARY);
				if (creature->getId() == CreatureID::ARROW_TOWERS )
					continue; // do not show destroyed towers in battle results

				icons.push_back(std::make_shared<CAnimImage>(AnimationPath::builtin("CPRSMALL"), creature->getIconIndex(), 0, xPos, yPos));
				std::ostringstream amount;
				amount<<elem.second;
				labels.push_back(std::make_shared<CLabel>(xPos + 16, yPos + 42, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, amount.str()));
				xPos += 42;
			}
		}
	}

	auto resources = getResources(br);

	description = std::make_shared<CTextBox>(resources.resultText.toString(), Rect(69, 203, 330, 68), 0, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE);
	videoPlayer = std::make_shared<VideoWidget>(Point(107, 70), resources.prologueVideo, resources.loopedVideo, false);

	ENGINE->music().playMusic(resources.musicName, false, true);
}

BattleResultResources BattleResultWindow::getResources(const BattleResult & br)
{
	//printing result description
	bool weAreAttacker = owner.cb->getBattle(br.battleID)->battleGetMySide() == BattleSide::ATTACKER;
	bool weAreDefender = !weAreAttacker;
	bool weWon = (br.winner == BattleSide::ATTACKER && weAreAttacker) || (br.winner == BattleSide::DEFENDER && !weAreAttacker);
	bool isSiege = owner.cb->getBattle(br.battleID)->battleGetDefendedTown() != nullptr;

	BattleResultResources resources;

	if(weWon)
	{
		if(isSiege && weAreDefender)
		{
			resources.musicName = AudioPath::builtin("Music/Defend Castle");
			resources.prologueVideo = VideoPath::builtin("DEFENDALL.BIK");
			resources.loopedVideo = VideoPath::builtin("defendloop.bik");
		}
		else
		{
			resources.musicName = AudioPath::builtin("Music/Win Battle");
			resources.prologueVideo = VideoPath::builtin("WIN3.BIK");
			resources.loopedVideo = VideoPath::builtin("WIN3.BIK");
		}

		switch(br.result)
		{
			case EBattleResult::NORMAL:
				resources.resultText.appendTextID("core.genrltxt.304");
				break;
			case EBattleResult::ESCAPE:
				resources.resultText.appendTextID("core.genrltxt.303");
				break;
			case EBattleResult::SURRENDER:
				resources.resultText.appendTextID("core.genrltxt.302");
				break;
			default:
				throw std::runtime_error("Invalid battle result!");
		}

		const CGHeroInstance * ourHero = owner.cb->getBattle(br.battleID)->battleGetMyHero();
		if(ourHero)
		{
			resources.resultText.appendTextID("core.genrltxt.305");
			resources.resultText.replaceTextID(ourHero->getNameTextID());
			resources.resultText.replaceNumber(br.exp[weAreAttacker ? BattleSide::ATTACKER : BattleSide::DEFENDER]);
		}
	}
	else // we lose
	{
		switch(br.result)
		{
			case EBattleResult::NORMAL:
				resources.resultText.appendTextID("core.genrltxt.311");
				resources.musicName = AudioPath::builtin("Music/LoseCombat");
				resources.prologueVideo = VideoPath::builtin("LBSTART.BIK");
				resources.loopedVideo = VideoPath::builtin("LBLOOP.BIK");
				break;
			case EBattleResult::ESCAPE:
				resources.resultText.appendTextID("core.genrltxt.310");
				resources.musicName = AudioPath::builtin("Music/Retreat Battle");
				resources.prologueVideo = VideoPath::builtin("RTSTART.BIK");
				resources.loopedVideo = VideoPath::builtin("RTLOOP.BIK");
				break;
			case EBattleResult::SURRENDER:
				resources.resultText.appendTextID("core.genrltxt.309");
				resources.musicName = AudioPath::builtin("Music/Surrender Battle");
				resources.prologueVideo = VideoPath::builtin("SURRENDER.BIK");
				resources.loopedVideo = VideoPath::builtin("SURRENDER.BIK");
				break;
			default:
				throw std::runtime_error("Invalid battle result!");
		}

		if(isSiege && weAreDefender)
		{
			resources.musicName = AudioPath::builtin("Music/LoseCastle");
			resources.prologueVideo = VideoPath::builtin("LOSECSTL.BIK");
			resources.loopedVideo = VideoPath::builtin("LOSECSLP.BIK");
		}
	}

	return resources;
}

void BattleResultWindow::activate()
{
	owner.showingDialog->setBusy();
	CIntObject::activate();
}

void BattleResultWindow::buttonPressed(int button)
{
	if(resultCallback)
		resultCallback(button);

	CPlayerInterface & intTmp = owner; //copy reference because "this" will be destructed soon

	close();

	if(ENGINE->windows().topWindow<BattleWindow>())
		ENGINE->windows().popWindows(1); //pop battle interface if present

	//Result window and battle interface are gone. We requested all dialogs to be closed before opening the battle,
	//so we can be sure that there is no dialogs left on GUI stack.
	intTmp.showingDialog->setFree();
}

void BattleResultWindow::bExitf()
{
	buttonPressed(0);
}

void BattleResultWindow::bRepeatf()
{
	buttonPressed(1);
}
