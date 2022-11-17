/*
 * CBattleInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleInterface.h"

#include "CBattleAnimations.h"
#include "CBattleInterfaceClasses.h"
#include "CCreatureAnimation.h"
#include "CBattleProjectileController.h"
#include "CBattleSiegeController.h"

#include "../CBitmapHandler.h"
#include "../CGameInfo.h"
#include "../CMessage.h"
#include "../CMT.h"
#include "../CMusicHandler.h"
#include "../CPlayerInterface.h"
#include "../CVideoHandler.h"
#include "../Graphics.h"
#include "../gui/CAnimation.h"
#include "../gui/CCursorHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/SDL_Extensions.h"
#include "../windows/CAdvmapInterface.h"
#include "../windows/CCreatureWindow.h"
#include "../windows/CSpellWindow.h"

#include "../../CCallback.h"
#include "../../lib/CStack.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CHeroHandler.h"
#include "../../lib/CondSh.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/BattleFieldHandler.h"
#include "../../lib/ObstacleHandler.h"
#include "../../lib/CGameState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/NetPacks.h"
#include "../../lib/UnlockGuard.h"

CondSh<bool> CBattleInterface::animsAreDisplayed(false);
CondSh<BattleAction *> CBattleInterface::givenCommand(nullptr);

static void onAnimationFinished(const CStack *stack, std::weak_ptr<CCreatureAnimation> anim)
{
	std::shared_ptr<CCreatureAnimation> animation = anim.lock();
	if(!animation)
		return;

	if (animation->isIdle())
	{
		const CCreature *creature = stack->getCreature();

		if (animation->framesInGroup(CCreatureAnim::MOUSEON) > 0)
		{
			if (CRandomGenerator::getDefault().nextDouble(99.0) < creature->animation.timeBetweenFidgets *10)
				animation->playOnce(CCreatureAnim::MOUSEON);
			else
				animation->setType(CCreatureAnim::HOLDING);
		}
		else
		{
			animation->setType(CCreatureAnim::HOLDING);
		}
	}
	// always reset callback
	animation->onAnimationReset += std::bind(&onAnimationFinished, stack, anim);
}

static void transformPalette(SDL_Surface *surf, double rCor, double gCor, double bCor)
{
	SDL_Color *colorsToChange = surf->format->palette->colors;
	for (int g=0; g<surf->format->palette->ncolors; ++g)
	{
		SDL_Color *color = &colorsToChange[g];
		if (color->b != 132 &&
			color->g != 231 &&
			color->r != 255) //it's not yellow border
		{
			color->r = static_cast<Uint8>(color->r * rCor);
			color->g = static_cast<Uint8>(color->g * gCor);
			color->b = static_cast<Uint8>(color->b * bCor);
		}
	}
}

void CBattleInterface::addNewAnim(CBattleAnimation *anim)
{
	pendingAnims.push_back( std::make_pair(anim, false) );
	animsAreDisplayed.setn(true);
}

CBattleInterface::CBattleInterface(const CCreatureSet *army1, const CCreatureSet *army2,
		const CGHeroInstance *hero1, const CGHeroInstance *hero2,
		const SDL_Rect & myRect,
		std::shared_ptr<CPlayerInterface> att, std::shared_ptr<CPlayerInterface> defen, std::shared_ptr<CPlayerInterface> spectatorInt)
	: background(nullptr), attackingHeroInstance(hero1), defendingHeroInstance(hero2), animCount(0),
	activeStack(nullptr), mouseHoveredStack(nullptr), stackToActivate(nullptr), selectedStack(nullptr), previouslyHoveredHex(-1),
	currentlyHoveredHex(-1), attackingHex(-1), stackCanCastSpell(false), creatureCasting(false), spellDestSelectMode(false), spellToCast(nullptr), sp(nullptr),
	creatureSpellToCast(-1),
	attackerInt(att), defenderInt(defen), curInt(att), animIDhelper(0),
	myTurn(false), moveStarted(false), moveSoundHander(-1), bresult(nullptr), battleActionsStarted(false)
{
	OBJ_CONSTRUCTION;

	projectilesController.reset(new CBattleProjectileController(this));

	if(spectatorInt)
	{
		curInt = spectatorInt;
	}
	else if(!curInt)
	{
		//May happen when we are defending during network MP game -> attacker interface is just not present
		curInt = defenderInt;
	}

	animsAreDisplayed.setn(false);
	pos = myRect;
	strongInterest = true;
	givenCommand.setn(nullptr);

	//hot-seat -> check tactics for both players (defender may be local human)
	if(attackerInt && attackerInt->cb->battleGetTacticDist())
		tacticianInterface = attackerInt;
	else if(defenderInt && defenderInt->cb->battleGetTacticDist())
		tacticianInterface = defenderInt;

	//if we found interface of player with tactics, then enter tactics mode
	tacticsMode = static_cast<bool>(tacticianInterface);

	//create stack queue

	bool embedQueue;

	std::string queueSize = settings["battle"]["queueSize"].String();

	if(queueSize == "auto")
		embedQueue = screen->h < 700;
	else
		embedQueue = screen->h < 700 || queueSize == "small";

	queue = std::make_shared<CStackQueue>(embedQueue, this);
	if(!embedQueue)
	{
		if (settings["battle"]["showQueue"].Bool())
			pos.y += queue->pos.h / 2; //center whole window

		queue->moveTo(Point(pos.x, pos.y - queue->pos.h));
	}
	queue->update();

	//preparing siege info
	const CGTownInstance *town = curInt->cb->battleGetDefendedTown();
	if(town && town->hasFort())
		siegeController.reset(new CBattleSiegeController(this, town));

	CPlayerInterface::battleInt = this;

	//initializing armies
	this->army1 = army1;
	this->army2 = army2;
	std::vector<const CStack*> stacks = curInt->cb->battleGetAllStacks(true);
	for(const CStack * s : stacks)
	{
		unitAdded(s);
	}

	//preparing menu background and terrain
	if(!siegeController)
	{
		auto bfieldType = curInt->cb->battleGetBattlefieldType();

		if(bfieldType == BattleField::NONE)
		{
			logGlobal->error("Invalid battlefield returned for current battle");
		}
		else
		{
			background = BitmapHandler::loadBitmap(bfieldType.getInfo()->graphics, false);
		}
	}

	//preparing graphics for displaying amounts of creatures
	amountNormal = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountNormal);
	transformPalette(amountNormal, 0.59, 0.19, 0.93);

	amountPositive = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountPositive);
	transformPalette(amountPositive, 0.18, 1.00, 0.18);

	amountNegative = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountNegative);
	transformPalette(amountNegative, 1.00, 0.18, 0.18);

	amountEffNeutral = BitmapHandler::loadBitmap("CMNUMWIN.BMP");
	CSDL_Ext::alphaTransform(amountEffNeutral);
	transformPalette(amountEffNeutral, 1.00, 1.00, 0.18);

	//preparing buttons and console
	bOptions = std::make_shared<CButton>(Point(  3, 561), "icm003.def", CGI->generaltexth->zelp[381], std::bind(&CBattleInterface::bOptionsf,this), SDLK_o);
	bSurrender = std::make_shared<CButton>(Point( 54, 561), "icm001.def", CGI->generaltexth->zelp[379], std::bind(&CBattleInterface::bSurrenderf,this), SDLK_s);
	bFlee = std::make_shared<CButton>(Point(105, 561), "icm002.def", CGI->generaltexth->zelp[380], std::bind(&CBattleInterface::bFleef,this), SDLK_r);
	bAutofight = std::make_shared<CButton>(Point(157, 561), "icm004.def", CGI->generaltexth->zelp[382], std::bind(&CBattleInterface::bAutofightf,this), SDLK_a);
	bSpell = std::make_shared<CButton>(Point(645, 561), "icm005.def", CGI->generaltexth->zelp[385], std::bind(&CBattleInterface::bSpellf,this), SDLK_c);
	bWait = std::make_shared<CButton>(Point(696, 561), "icm006.def", CGI->generaltexth->zelp[386], std::bind(&CBattleInterface::bWaitf,this), SDLK_w);
	bDefence = std::make_shared<CButton>(Point(747, 561), "icm007.def", CGI->generaltexth->zelp[387], std::bind(&CBattleInterface::bDefencef,this), SDLK_d);
	bDefence->assignedKeys.insert(SDLK_SPACE);
	bConsoleUp = std::make_shared<CButton>(Point(624, 561), "ComSlide.def", std::make_pair("", ""), std::bind(&CBattleInterface::bConsoleUpf,this), SDLK_UP);
	bConsoleDown = std::make_shared<CButton>(Point(624, 580), "ComSlide.def", std::make_pair("", ""), std::bind(&CBattleInterface::bConsoleDownf,this), SDLK_DOWN);
	bConsoleUp->setImageOrder(0, 1, 0, 0);
	bConsoleDown->setImageOrder(2, 3, 2, 2);

	console = std::make_shared<CBattleConsole>();
	console->pos.x += 211;
	console->pos.y += 560;
	console->pos.w = 406;
	console->pos.h = 38;
	if(tacticsMode)
	{
		btactNext = std::make_shared<CButton>(Point(213, 560), "icm011.def", std::make_pair("", ""), [&](){ bTacticNextStack(nullptr);}, SDLK_SPACE);
		btactEnd = std::make_shared<CButton>(Point(419, 560), "icm012.def", std::make_pair("", ""), [&](){ bEndTacticPhase();}, SDLK_RETURN);
		menu = BitmapHandler::loadBitmap("COPLACBR.BMP");
	}
	else
	{
		menu = BitmapHandler::loadBitmap("CBAR.BMP");
	}
	graphics->blueToPlayersAdv(menu, curInt->playerID);

	//loading hero animations
	if(hero1) // attacking hero
	{
		std::string battleImage;
		if(!hero1->type->battleImage.empty())
		{
			battleImage = hero1->type->battleImage;
		}
		else
		{
			if(hero1->sex)
				battleImage = hero1->type->heroClass->imageBattleFemale;
			else
				battleImage = hero1->type->heroClass->imageBattleMale;
		}

		attackingHero = std::make_shared<CBattleHero>(battleImage, false, hero1->tempOwner, hero1->tempOwner == curInt->playerID ? hero1 : nullptr, this);

		auto img = attackingHero->animation->getImage(0, 0, true);
		if(img)
			attackingHero->pos = genRect(img->height(), img->width(), pos.x - 43, pos.y - 19);
	}


	if(hero2) // defending hero
	{
		std::string battleImage;

		if(!hero2->type->battleImage.empty())
		{
			battleImage = hero2->type->battleImage;
		}
		else
		{
			if(hero2->sex)
				battleImage = hero2->type->heroClass->imageBattleFemale;
			else
				battleImage = hero2->type->heroClass->imageBattleMale;
		}

		defendingHero = std::make_shared<CBattleHero>(battleImage, true, hero2->tempOwner, hero2->tempOwner == curInt->playerID ? hero2 : nullptr, this);

		auto img = defendingHero->animation->getImage(0, 0, true);
		if(img)
			defendingHero->pos = genRect(img->height(), img->width(), pos.x + 693, pos.y - 19);
	}


	//preparing cells and hexes
	cellBorder = BitmapHandler::loadBitmap("CCELLGRD.BMP");
	CSDL_Ext::alphaTransform(cellBorder);
	cellShade = BitmapHandler::loadBitmap("CCELLSHD.BMP");
	CSDL_Ext::alphaTransform(cellShade);
	for (int h = 0; h < GameConstants::BFIELD_SIZE; ++h)
	{
		auto hex = std::make_shared<CClickableHex>();
		hex->myNumber = h;
		hex->pos = hexPosition(h);
		hex->accessible = true;
		hex->myInterface = this;
		bfield.push_back(hex);
	}
	//locking occupied positions on batlefield
	for(const CStack * s : stacks)  //stacks gained at top of this function
		if(s->initialPosition >= 0) //turrets have position < 0
			bfield[s->getPosition()]->accessible = false;

	//preparing graphic with cell borders
	cellBorders = CSDL_Ext::newSurface(background->w, background->h, cellBorder);
	//copying palette
	for (int g=0; g<cellBorder->format->palette->ncolors; ++g) //we assume that cellBorders->format->palette->ncolors == 256
	{
		cellBorders->format->palette->colors[g] = cellBorder->format->palette->colors[g];
	}
	//palette copied
	for (int i=0; i<GameConstants::BFIELD_HEIGHT; ++i) //rows
	{
		for (int j=0; j<GameConstants::BFIELD_WIDTH-2; ++j) //columns
		{
			int x = 58 + (i%2==0 ? 22 : 0) + 44*j;
			int y = 86 + 42 *i;
			for (int cellX = 0; cellX < cellBorder->w; ++cellX)
			{
				for (int cellY = 0; cellY < cellBorder->h; ++cellY)
				{
					if (y+cellY < cellBorders->h && x+cellX < cellBorders->w)
						* ((Uint8*)cellBorders->pixels + (y+cellY) *cellBorders->pitch + (x+cellX)) |= *((Uint8*)cellBorder->pixels + cellY *cellBorder->pitch + cellX);
				}
			}
		}
	}

	backgroundWithHexes = CSDL_Ext::newSurface(background->w, background->h, screen);

	//preparing obstacle defs
	auto obst = curInt->cb->battleGetAllObstacles();
	for(auto & elem : obst)
	{
		if(elem->obstacleType == CObstacleInstance::USUAL)
		{
			std::string animationName = elem->getInfo().animation;

			auto cached = animationsCache.find(animationName);

			if(cached == animationsCache.end())
			{
				auto animation = std::make_shared<CAnimation>(animationName);
				animationsCache[animationName] = animation;
				obstacleAnimations[elem->uniqueID] = animation;
				animation->preload();
			}
			else
			{
				obstacleAnimations[elem->uniqueID] = cached->second;
			}
		}
		else if (elem->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			std::string animationName = elem->getInfo().animation;

			auto cached = animationsCache.find(animationName);

			if(cached == animationsCache.end())
			{
				auto animation = std::make_shared<CAnimation>();
				animation->setCustom(animationName, 0, 0);
				animationsCache[animationName] = animation;
				obstacleAnimations[elem->uniqueID] = animation;
				animation->preload();
			}
			else
			{
				obstacleAnimations[elem->uniqueID] = cached->second;
			}
		}
	}

	for(auto hex : bfield)
		addChild(hex.get());

	if(tacticsMode)
		bTacticNextStack();

	CCS->musich->stopMusic();
	battleIntroSoundChannel = CCS->soundh->playSoundFromSet(CCS->soundh->battleIntroSounds);
	auto onIntroPlayed = [&]()
	{
		if(LOCPLINT->battleInt)
		{
			CCS->musich->playMusicFromSet("battle", true, true);
			battleActionsStarted = true;
			blockUI(settings["session"]["spectate"].Bool());
			battleIntroSoundChannel = -1;
		}
	};

	CCS->soundh->setCallback(battleIntroSoundChannel, onIntroPlayed);

	currentAction = PossiblePlayerBattleAction::INVALID;
	selectedAction = PossiblePlayerBattleAction::INVALID;
	addUsedEvents(RCLICK | MOVE | KEYBOARD);
	blockUI(true);
}

CBattleInterface::~CBattleInterface()
{
	CPlayerInterface::battleInt = nullptr;
	givenCommand.cond.notify_all(); //that two lines should make any activeStack waiting thread to finish

	if (active) //dirty fix for #485
	{
		deactivate();
	}
	SDL_FreeSurface(background);
	SDL_FreeSurface(menu);
	SDL_FreeSurface(amountNormal);
	SDL_FreeSurface(amountNegative);
	SDL_FreeSurface(amountPositive);
	SDL_FreeSurface(amountEffNeutral);
	SDL_FreeSurface(cellBorders);
	SDL_FreeSurface(backgroundWithHexes);


	SDL_FreeSurface(cellBorder);
	SDL_FreeSurface(cellShade);

	//TODO: play AI tracks if battle was during AI turn
	//if (!curInt->makingTurn)
	//CCS->musich->playMusicFromSet(CCS->musich->aiMusics, -1);

	if (adventureInt && adventureInt->selection)
	{
		const auto & terrain = *(LOCPLINT->cb->getTile(adventureInt->selection->visitablePos())->terType);
		CCS->musich->playMusicFromSet("terrain", terrain.name, true, false);
	}
	animsAreDisplayed.setn(false);
}

void CBattleInterface::setPrintCellBorders(bool set)
{
	Settings cellBorders = settings.write["battle"]["cellBorders"];
	cellBorders->Bool() = set;

	redrawBackgroundWithHexes(activeStack);
	GH.totalRedraw();
}

void CBattleInterface::setPrintStackRange(bool set)
{
	Settings stackRange = settings.write["battle"]["stackRange"];
	stackRange->Bool() = set;

	redrawBackgroundWithHexes(activeStack);
	GH.totalRedraw();
}

void CBattleInterface::setPrintMouseShadow(bool set)
{
	Settings shadow = settings.write["battle"]["mouseShadow"];
	shadow->Bool() = set;
}

void CBattleInterface::activate()
{
	if (curInt->isAutoFightOn)
	{
		bAutofight->activate();
		return;
	}

	CIntObject::activate();
	bOptions->activate();
	bSurrender->activate();
	bFlee->activate();
	bAutofight->activate();
	bSpell->activate();
	bWait->activate();
	bDefence->activate();

	if (attackingHero)
		attackingHero->activate();
	if (defendingHero)
		defendingHero->activate();

	for (auto hex : bfield)
		hex->activate();

	if (settings["battle"]["showQueue"].Bool())
		queue->activate();

	if (tacticsMode)
	{
		btactNext->activate();
		btactEnd->activate();
	}
	else
	{
		bConsoleUp->activate();
		bConsoleDown->activate();
	}

	LOCPLINT->cingconsole->activate();
}

void CBattleInterface::deactivate()
{
	CIntObject::deactivate();

	bOptions->deactivate();
	bSurrender->deactivate();
	bFlee->deactivate();
	bAutofight->deactivate();
	bSpell->deactivate();
	bWait->deactivate();
	bDefence->deactivate();

	for (auto hex : bfield)
		hex->deactivate();

	if (attackingHero)
		attackingHero->deactivate();
	if (defendingHero)
		defendingHero->deactivate();
	if (settings["battle"]["showQueue"].Bool())
		queue->deactivate();

	if (tacticsMode)
	{
		btactNext->deactivate();
		btactEnd->deactivate();
	}
	else
	{
		bConsoleUp->deactivate();
		bConsoleDown->deactivate();
	}

	LOCPLINT->cingconsole->deactivate();
}

void CBattleInterface::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_q && key.state == SDL_PRESSED)
	{
		if(settings["battle"]["showQueue"].Bool()) //hide queue
			hideQueue();
		else
			showQueue();

	}
	else if(key.keysym.sym == SDLK_f && key.state == SDL_PRESSED)
	{
		enterCreatureCastingMode();
	}
	else if(key.keysym.sym == SDLK_ESCAPE)
	{
		if(!battleActionsStarted)
			CCS->soundh->stopSound(battleIntroSoundChannel);
		else
			endCastingSpell();
	}
}
void CBattleInterface::mouseMoved(const SDL_MouseMotionEvent &sEvent)
{
	auto hexItr = std::find_if(bfield.begin(), bfield.end(), [](std::shared_ptr<CClickableHex> hex)
	{
		return hex->hovered && hex->strictHovered;
	});

	handleHex(hexItr == bfield.end() ? -1 : (*hexItr)->myNumber, MOVE);
}

void CBattleInterface::setBattleCursor(const int myNumber)
{
	const CClickableHex & hoveredHex = *bfield[myNumber];
	CCursorHandler *cursor = CCS->curh;

	const double subdividingAngle = 2.0*M_PI/6.0; // Divide a hex into six sectors.
	const double hexMidX = hoveredHex.pos.x + hoveredHex.pos.w/2.0;
	const double hexMidY = hoveredHex.pos.y + hoveredHex.pos.h/2.0;
	const double cursorHexAngle = M_PI - atan2(hexMidY - cursor->ypos, cursor->xpos - hexMidX) + subdividingAngle/2; //TODO: refactor this nightmare
	const double sector = fmod(cursorHexAngle/subdividingAngle, 6.0);
	const int zigzagCorrection = !((myNumber/GameConstants::BFIELD_WIDTH)%2); // Off-by-one correction needed to deal with the odd battlefield rows.

	std::vector<int> sectorCursor; // From left to bottom left.
	sectorCursor.push_back(8);
	sectorCursor.push_back(9);
	sectorCursor.push_back(10);
	sectorCursor.push_back(11);
	sectorCursor.push_back(12);
	sectorCursor.push_back(7);

	const bool doubleWide = activeStack->doubleWide();
	bool aboveAttackable = true, belowAttackable = true;

	// Exclude directions which cannot be attacked from.
	// Check to the left.
	if (myNumber%GameConstants::BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber - 1))
	{
		sectorCursor[0] = -1;
	}
	// Check top left, top right as well as above for 2-hex creatures.
	if (myNumber/GameConstants::BFIELD_WIDTH == 0)
	{
			sectorCursor[1] = -1;
			sectorCursor[2] = -1;
			aboveAttackable = false;
	}
	else
	{
		if (doubleWide)
		{
			bool attackRow[4] = {true, true, true, true};

			if (myNumber%GameConstants::BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH - 2 + zigzagCorrection))
				attackRow[0] = false;
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
				attackRow[1] = false;
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH + zigzagCorrection))
				attackRow[2] = false;
			if (myNumber%GameConstants::BFIELD_WIDTH >= GameConstants::BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH + 1 + zigzagCorrection))
				attackRow[3] = false;

			if (!(attackRow[0] && attackRow[1]))
				sectorCursor[1] = -1;
			if (!(attackRow[1] && attackRow[2]))
				aboveAttackable = false;
			if (!(attackRow[2] && attackRow[3]))
				sectorCursor[2] = -1;
		}
		else
		{
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
				sectorCursor[1] = -1;
			if (!vstd::contains(occupyableHexes, myNumber - GameConstants::BFIELD_WIDTH + zigzagCorrection))
				sectorCursor[2] = -1;
		}
	}
	// Check to the right.
	if (myNumber%GameConstants::BFIELD_WIDTH >= GameConstants::BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber + 1))
	{
		sectorCursor[3] = -1;
	}
	// Check bottom right, bottom left as well as below for 2-hex creatures.
	if (myNumber/GameConstants::BFIELD_WIDTH == GameConstants::BFIELD_HEIGHT - 1)
	{
		sectorCursor[4] = -1;
		sectorCursor[5] = -1;
		belowAttackable = false;
	}
	else
	{
		if (doubleWide)
		{
			bool attackRow[4] = {true, true, true, true};

			if (myNumber%GameConstants::BFIELD_WIDTH <= 1 || !vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH - 2 + zigzagCorrection))
				attackRow[0] = false;
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
				attackRow[1] = false;
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH + zigzagCorrection))
				attackRow[2] = false;
			if (myNumber%GameConstants::BFIELD_WIDTH >= GameConstants::BFIELD_WIDTH - 2 || !vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH + 1 + zigzagCorrection))
				attackRow[3] = false;

			if (!(attackRow[0] && attackRow[1]))
				sectorCursor[5] = -1;
			if (!(attackRow[1] && attackRow[2]))
				belowAttackable = false;
			if (!(attackRow[2] && attackRow[3]))
				sectorCursor[4] = -1;
		}
		else
		{
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH + zigzagCorrection))
				sectorCursor[4] = -1;
			if (!vstd::contains(occupyableHexes, myNumber + GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection))
				sectorCursor[5] = -1;
		}
	}

	// Determine index from sector.
	int cursorIndex;
	if (doubleWide)
	{
		sectorCursor.insert(sectorCursor.begin() + 5, belowAttackable ? 13 : -1);
		sectorCursor.insert(sectorCursor.begin() + 2, aboveAttackable ? 14 : -1);

		if (sector < 1.5)
			cursorIndex = static_cast<int>(sector);
		else if (sector >= 1.5 && sector < 2.5)
			cursorIndex = 2;
		else if (sector >= 2.5 && sector < 4.5)
			cursorIndex = (int) sector + 1;
		else if (sector >= 4.5 && sector < 5.5)
			cursorIndex = 6;
		else
			cursorIndex = (int) sector + 2;
	}
	else
	{
		cursorIndex = static_cast<int>(sector);
	}

	// Generally should NEVER happen, but to avoid the possibility of having endless loop below... [#1016]
	if (!vstd::contains_if (sectorCursor, [](int sc) { return sc != -1; }))
	{
		logGlobal->error("Error: for hex %d cannot find a hex to attack from!", myNumber);
		attackingHex = -1;
		return;
	}

	// Find the closest direction attackable, starting with the right one.
	// FIXME: Is this really how the original H3 client does it?
	int i = 0;
	while (sectorCursor[(cursorIndex + i)%sectorCursor.size()] == -1) //Why hast thou forsaken me?
		i = i <= 0 ? 1 - i : -i; // 0, 1, -1, 2, -2, 3, -3 etc..
	int index = (cursorIndex + i)%sectorCursor.size(); //hopefully we get elements from sectorCursor
	cursor->changeGraphic(ECursor::COMBAT, sectorCursor[index]);
	switch (index)
	{
		case 0:
			attackingHex = myNumber - 1; //left
			break;
		case 1:
			attackingHex = myNumber - GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection; //top left
			break;
		case 2:
			attackingHex = myNumber - GameConstants::BFIELD_WIDTH + zigzagCorrection; //top right
			break;
		case 3:
			attackingHex = myNumber + 1; //right
			break;
		case 4:
			attackingHex = myNumber + GameConstants::BFIELD_WIDTH + zigzagCorrection; //bottom right
			break;
		case 5:
			attackingHex = myNumber + GameConstants::BFIELD_WIDTH - 1 + zigzagCorrection; //bottom left
			break;
	}
	BattleHex hex(attackingHex);
	if (!hex.isValid())
		attackingHex = -1;
}

void CBattleInterface::clickRight(tribool down, bool previousState)
{
	if (!down)
	{
		endCastingSpell();
	}
}

void CBattleInterface::bOptionsf()
{
	if (spellDestSelectMode) //we are casting a spell
		return;

	CCS->curh->changeGraphic(ECursor::ADVENTURE,0);

	Rect tempRect = genRect(431, 481, 160, 84);
	tempRect += pos.topLeft();
	GH.pushIntT<CBattleOptionsWindow>(tempRect, this);
}

void CBattleInterface::bSurrenderf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	int cost = curInt->cb->battleGetSurrenderCost();
	if(cost >= 0)
	{
		std::string enemyHeroName = curInt->cb->battleGetEnemyHero().name;
		if(enemyHeroName.empty())
		{
			logGlobal->warn("Surrender performed without enemy hero, should not happen!");
			enemyHeroName = "#ENEMY#";
		}

		std::string surrenderMessage = boost::str(boost::format(CGI->generaltexth->allTexts[32]) % enemyHeroName % cost); //%s states: "I will accept your surrender and grant you and your troops safe passage for the price of %d gold."
		curInt->showYesNoDialog(surrenderMessage, [this](){ reallySurrender(); }, nullptr);
	}
}

void CBattleInterface::bFleef()
{
	if (spellDestSelectMode) //we are casting a spell
		return;

	if ( curInt->cb->battleCanFlee() )
	{
		CFunctionList<void()> ony = std::bind(&CBattleInterface::reallyFlee,this);
		curInt->showYesNoDialog(CGI->generaltexth->allTexts[28], ony, nullptr); //Are you sure you want to retreat?
	}
	else
	{
		std::vector<std::shared_ptr<CComponent>> comps;
		std::string heroName;
		//calculating fleeing hero's name
		if (attackingHeroInstance)
			if (attackingHeroInstance->tempOwner == curInt->cb->getMyColor())
				heroName = attackingHeroInstance->name;
		if (defendingHeroInstance)
			if (defendingHeroInstance->tempOwner == curInt->cb->getMyColor())
				heroName = defendingHeroInstance->name;
		//calculating text
		auto txt = boost::format(CGI->generaltexth->allTexts[340]) % heroName; //The Shackles of War are present.  %s can not retreat!

		//printing message
		curInt->showInfoDialog(boost::to_string(txt), comps);
	}
}

void CBattleInterface::reallyFlee()
{
	giveCommand(EActionType::RETREAT);
	CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
}

void CBattleInterface::reallySurrender()
{
	if (curInt->cb->getResourceAmount(Res::GOLD) < curInt->cb->battleGetSurrenderCost())
	{
		curInt->showInfoDialog(CGI->generaltexth->allTexts[29]); //You don't have enough gold!
	}
	else
	{
		giveCommand(EActionType::SURRENDER);
		CCS->curh->changeGraphic(ECursor::ADVENTURE, 0);
	}
}

void CBattleInterface::bAutofightf()
{
	if(spellDestSelectMode) //we are casting a spell
		return;

	//Stop auto-fight mode
	if(curInt->isAutoFightOn)
	{
		assert(curInt->autofightingAI);
		curInt->isAutoFightOn = false;
		logGlobal->trace("Stopping the autofight...");
	}
	else if(!curInt->autofightingAI)
	{
		curInt->isAutoFightOn = true;
		blockUI(true);

		auto ai = CDynLibHandler::getNewBattleAI(settings["server"]["friendlyAI"].String());
		ai->init(curInt->env, curInt->cb);
		ai->battleStart(army1, army2, int3(0,0,0), attackingHeroInstance, defendingHeroInstance, curInt->cb->battleGetMySide());
		curInt->autofightingAI = ai;
		curInt->cb->registerBattleInterface(ai);

		requestAutofightingAIToTakeAction();
	}
}

void CBattleInterface::bSpellf()
{
	if (spellDestSelectMode) //we are casting a spell
		return;

	if (!myTurn)
		return;

	auto myHero = currentHero();
	if(!myHero)
		return;

	CCS->curh->changeGraphic(ECursor::ADVENTURE,0);

	ESpellCastProblem::ESpellCastProblem spellCastProblem = curInt->cb->battleCanCastSpell(myHero, spells::Mode::HERO);

	if(spellCastProblem == ESpellCastProblem::OK)
	{
		GH.pushIntT<CSpellWindow>(myHero, curInt.get());
	}
	else if (spellCastProblem == ESpellCastProblem::MAGIC_IS_BLOCKED)
	{
		//TODO: move to spell mechanics, add more information to spell cast problem
		//Handle Orb of Inhibition-like effects -> we want to display dialog with info, why casting is impossible
		auto blockingBonus = currentHero()->getBonusLocalFirst(Selector::type()(Bonus::BLOCK_ALL_MAGIC));
		if (!blockingBonus)
			return;

		if (blockingBonus->source == Bonus::ARTIFACT)
		{
			const int32_t artID = blockingBonus->sid;
			//If we have artifact, put name of our hero. Otherwise assume it's the enemy.
			//TODO check who *really* is source of bonus
			std::string heroName = myHero->hasArt(artID) ? myHero->name : enemyHero().name;

			//%s wields the %s, an ancient artifact which creates a p dead to all magic.
			LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[683])
										% heroName % CGI->artifacts()->getByIndex(artID)->getName()));
		}
	}
}

void CBattleInterface::bWaitf()
{
	if (spellDestSelectMode) //we are casting a spell
		return;

	if (activeStack != nullptr)
		giveCommand(EActionType::WAIT);
}

void CBattleInterface::bDefencef()
{
	if (spellDestSelectMode) //we are casting a spell
		return;

	if (activeStack != nullptr)
		giveCommand(EActionType::DEFEND);
}

void CBattleInterface::bConsoleUpf()
{
	if (spellDestSelectMode) //we are casting a spell
		return;

	console->scrollUp();
}

void CBattleInterface::bConsoleDownf()
{
	if (spellDestSelectMode) //we are casting a spell
		return;

	console->scrollDown();
}

void CBattleInterface::unitAdded(const CStack * stack)
{
	creDir[stack->ID] = stack->side == BattleSide::ATTACKER; // must be set before getting stack position

	Point coords = CClickableHex::getXYUnitAnim(stack->getPosition(), stack, this);

	if(stack->initialPosition < 0) //turret
	{
		assert(siegeController);

		const CCreature *turretCreature = siegeController->turretCreature();

		creAnims[stack->ID] = AnimationControls::getAnimation(turretCreature);
		creAnims[stack->ID]->pos.h = 225;

		coords = siegeController->turretCreaturePosition(stack->initialPosition);
	}
	else
	{
		creAnims[stack->ID] = AnimationControls::getAnimation(stack->getCreature());
		creAnims[stack->ID]->onAnimationReset += std::bind(&onAnimationFinished, stack, creAnims[stack->ID]);
		creAnims[stack->ID]->pos.h = creAnims[stack->ID]->getHeight();
	}
	creAnims[stack->ID]->pos.x = coords.x;
	creAnims[stack->ID]->pos.y = coords.y;
	creAnims[stack->ID]->pos.w = creAnims[stack->ID]->getWidth();
	creAnims[stack->ID]->setType(CCreatureAnim::HOLDING);

	//loading projectiles for units
	if(stack->isShooter())
	{
		projectilesController->initStackProjectile(stack);
	}
}

void CBattleInterface::stackRemoved(uint32_t stackID)
{
	if (activeStack != nullptr)
	{
		if (activeStack->ID == stackID)
		{
			BattleAction *action = new BattleAction();
			action->side = defendingHeroInstance ? (curInt->playerID == defendingHeroInstance->tempOwner) : false;
			action->actionType = EActionType::CANCEL;
			action->stackNumber = activeStack->ID;
			givenCommand.setn(action);
			setActiveStack(nullptr);
		}
	}

	//todo: ensure that ghost stack animation has fadeout effect

	redrawBackgroundWithHexes(activeStack);
	queue->update();
}

void CBattleInterface::stackActivated(const CStack *stack) //TODO: check it all before game state is changed due to abilities
{
	stackToActivate = stack;
	waitForAnims();
	if (stackToActivate) //during waiting stack may have gotten activated through show
		activateStack();
}

void CBattleInterface::stackMoved(const CStack *stack, std::vector<BattleHex> destHex, int distance)
{
	addNewAnim(new CMovementAnimation(this, stack, destHex, distance));
	waitForAnims();
}

void CBattleInterface::stacksAreAttacked(std::vector<StackAttackedInfo> attackedInfos)
{
	for(auto & attackedInfo : attackedInfos)
	{
		//if (!attackedInfo.cloneKilled) //FIXME: play dead animation for cloned creature before it vanishes
			addNewAnim(new CDefenceAnimation(attackedInfo, this));

		if(attackedInfo.rebirth)
		{
			displayEffect(50, attackedInfo.defender->getPosition()); //TODO: play reverse death animation
			CCS->soundh->playSound(soundBase::RESURECT);
		}
	}
	waitForAnims();

	std::array<int, 2> killedBySide = {0, 0};

	int targets = 0;
	for(const StackAttackedInfo & attackedInfo : attackedInfos)
	{
		++targets;

		ui8 side = attackedInfo.defender->side;
		killedBySide.at(side) += attackedInfo.amountKilled;
	}

	for(ui8 side = 0; side < 2; side++)
	{
		if(killedBySide.at(side) > killedBySide.at(1-side))
			setHeroAnimation(side, 2);
		else if(killedBySide.at(side) < killedBySide.at(1-side))
			setHeroAnimation(side, 3);
	}

	for (auto & attackedInfo : attackedInfos)
	{
		if (attackedInfo.rebirth)
			creAnims[attackedInfo.defender->ID]->setType(CCreatureAnim::HOLDING);
		if (attackedInfo.cloneKilled)
			stackRemoved(attackedInfo.defender->ID);
	}
}

void CBattleInterface::stackAttacking( const CStack *attacker, BattleHex dest, const CStack *attacked, bool shooting )
{
	if (shooting)
	{
		addNewAnim(new CShootingAnimation(this, attacker, dest, attacked));
	}
	else
	{
		addNewAnim(new CMeleeAttackAnimation(this, attacker, dest, attacked));
	}
	//waitForAnims();
}

void CBattleInterface::newRoundFirst( int round )
{
	waitForAnims();
}

void CBattleInterface::newRound(int number)
{
	console->addText(CGI->generaltexth->allTexts[412]);
}

void CBattleInterface::giveCommand(EActionType action, BattleHex tile, si32 additional)
{
	const CStack * actor = nullptr;
	if(action != EActionType::HERO_SPELL && action != EActionType::RETREAT && action != EActionType::SURRENDER)
	{
		actor = activeStack;
	}

	auto side = curInt->cb->playerToSide(curInt->playerID);
	if(!side)
	{
		logGlobal->error("Player %s is not in battle", curInt->playerID.getStr());
		return;
	}

	auto ba = new BattleAction(); //is deleted in CPlayerInterface::activeStack()
	ba->side = side.get();
	ba->actionType = action;
	ba->aimToHex(tile);
	ba->actionSubtype = additional;

	sendCommand(ba, actor);
}

void CBattleInterface::sendCommand(BattleAction *& command, const CStack * actor)
{
	command->stackNumber = actor ? actor->unitId() : ((command->side == BattleSide::ATTACKER) ? -1 : -2);

	if(!tacticsMode)
	{
		logGlobal->trace("Setting command for %s", (actor ? actor->nodeName() : "hero"));
		myTurn = false;
		setActiveStack(nullptr);
		givenCommand.setn(command);
	}
	else
	{
		curInt->cb->battleMakeTacticAction(command);
		vstd::clear_pointer(command);
		setActiveStack(nullptr);
		//next stack will be activated when action ends
	}
}

bool CBattleInterface::isTileAttackable(const BattleHex & number) const
{
	for (auto & elem : occupyableHexes)
	{
		if (BattleHex::mutualPosition(elem, number) != -1 || elem == number)
			return true;
	}
	return false;
}


const CGHeroInstance * CBattleInterface::getActiveHero()
{
	const CStack *attacker = activeStack;
	if(!attacker)
	{
		return nullptr;
	}

	if(attacker->side == BattleSide::ATTACKER)
	{
		return attackingHeroInstance;
	}

	return defendingHeroInstance;
}

void CBattleInterface::hexLclicked(int whichOne)
{
	handleHex(whichOne, LCLICK);
}

void CBattleInterface::stackIsCatapulting(const CatapultAttack & ca)
{
	if (siegeController)
		siegeController->stackIsCatapulting(ca);
}

void CBattleInterface::gateStateChanged(const EGateState state)
{
	if (siegeController)
		siegeController->gateStateChanged(state);
}

void CBattleInterface::battleFinished(const BattleResult& br)
{
	bresult = &br;
	{
		auto unlockPim = vstd::makeUnlockGuard(*CPlayerInterface::pim);
		animsAreDisplayed.waitUntil(false);
	}
	setActiveStack(nullptr);
	displayBattleFinished();
}

void CBattleInterface::displayBattleFinished()
{
	CCS->curh->changeGraphic(ECursor::ADVENTURE,0);
	if(settings["session"]["spectate"].Bool() && settings["session"]["spectate-skip-battle-result"].Bool())
	{
		close();
		return;
	}

	GH.pushInt(std::make_shared<CBattleResultWindow>(*bresult, *(this->curInt)));
	curInt->waitWhileDialog(); // Avoid freeze when AI end turn after battle. Check bug #1897
	CPlayerInterface::battleInt = nullptr;
}

void CBattleInterface::spellCast(const BattleSpellCast * sc)
{
	const SpellID spellID = sc->spellID;
	const CSpell * spell = spellID.toSpell();

	if(!spell)
		return;

	const std::string & castSoundPath = spell->getCastSound();

	if (!castSoundPath.empty())
		CCS->soundh->playSound(castSoundPath);

	const auto casterStackID = sc->casterStack;
	const CStack * casterStack = nullptr;
	if(casterStackID >= 0)
	{
		casterStack = curInt->cb->battleGetStackByID(casterStackID);
	}

	Point srccoord = (sc->side ? Point(770, 60) : Point(30, 60)) + pos;	//hero position by default
	{
		if(casterStack != nullptr)
		{
			srccoord = CClickableHex::getXYUnitAnim(casterStack->getPosition(), casterStack, this);
			srccoord.x += 250;
			srccoord.y += 240;
		}
	}

	if(casterStack != nullptr && sc->activeCast)
	{
		//todo: custom cast animation for hero
		displaySpellCast(spellID, casterStack->getPosition());

		addNewAnim(new CCastAnimation(this, casterStack, sc->tile, curInt->cb->battleGetStackByPos(sc->tile)));
	}

	waitForAnims(); //wait for cast animation

	//playing projectile animation
	if (sc->tile.isValid())
	{
		Point destcoord = CClickableHex::getXYUnitAnim(sc->tile, curInt->cb->battleGetStackByPos(sc->tile), this); //position attacked by projectile
		destcoord.x += 250; destcoord.y += 240;

		//animation angle
		double angle = atan2(static_cast<double>(destcoord.x - srccoord.x), static_cast<double>(destcoord.y - srccoord.y));
		bool Vflip = (angle < 0);
		if (Vflip)
			angle = -angle;

		std::string animToDisplay = spell->animationInfo.selectProjectile(angle);

		if(!animToDisplay.empty())
		{
			//TODO: calculate inside CEffectAnimation
			std::shared_ptr<CAnimation> tmp = std::make_shared<CAnimation>(animToDisplay);
			tmp->load(0, 0);
			auto first = tmp->getImage(0, 0);

			//displaying animation
			double diffX = (destcoord.x - srccoord.x)*(destcoord.x - srccoord.x);
			double diffY = (destcoord.y - srccoord.y)*(destcoord.y - srccoord.y);
			double distance = sqrt(diffX + diffY);

			int steps = static_cast<int>(distance / AnimationControls::getSpellEffectSpeed() + 1);
			int dx = (destcoord.x - srccoord.x - first->width())/steps;
			int dy = (destcoord.y - srccoord.y - first->height())/steps;

			addNewAnim(new CEffectAnimation(this, animToDisplay, srccoord.x, srccoord.y, dx, dy, Vflip));
		}
	}

	waitForAnims(); //wait for projectile animation

	displaySpellHit(spellID, sc->tile);

	//queuing affect animation
	for(auto & elem : sc->affectedCres)
	{
		auto stack = curInt->cb->battleGetStackByID(elem, false);
		if(stack)
			displaySpellEffect(spellID, stack->getPosition());
	}

	//queuing additional animation
	for(auto & elem : sc->customEffects)
	{
		auto stack = curInt->cb->battleGetStackByID(elem.stack, false);
		if(stack)
			displayEffect(elem.effect, stack->getPosition());
	}

	waitForAnims();
	//mana absorption
	if (sc->manaGained > 0)
	{
		Point leftHero = Point(15, 30) + pos;
		Point rightHero = Point(755, 30) + pos;
		addNewAnim(new CEffectAnimation(this, sc->side ? "SP07_A.DEF" : "SP07_B.DEF", leftHero.x, leftHero.y, 0, 0, false));
		addNewAnim(new CEffectAnimation(this, sc->side ? "SP07_B.DEF" : "SP07_A.DEF", rightHero.x, rightHero.y, 0, 0, false));
	}
}

void CBattleInterface::battleStacksEffectsSet(const SetStackEffect & sse)
{
	if(activeStack != nullptr)
		redrawBackgroundWithHexes(activeStack);
}

void CBattleInterface::setHeroAnimation(ui8 side, int phase)
{
	if(side == BattleSide::ATTACKER)
	{
		if(attackingHero)
			attackingHero->setPhase(phase);
	}
	else
	{
		if(defendingHero)
			defendingHero->setPhase(phase);
	}
}

void CBattleInterface::castThisSpell(SpellID spellID)
{
	spellToCast = std::make_shared<BattleAction>();
	spellToCast->actionType = EActionType::HERO_SPELL;
	spellToCast->actionSubtype = spellID; //spell number
	spellToCast->stackNumber = (attackingHeroInstance->tempOwner == curInt->playerID) ? -1 : -2;
	spellToCast->side = defendingHeroInstance ? (curInt->playerID == defendingHeroInstance->tempOwner) : false;
	spellDestSelectMode = true;
	creatureCasting = false;

	//choosing possible targets
	const CGHeroInstance *castingHero = (attackingHeroInstance->tempOwner == curInt->playerID) ? attackingHeroInstance : defendingHeroInstance;
	assert(castingHero); // code below assumes non-null hero
	sp = spellID.toSpell();
	PossiblePlayerBattleAction spellSelMode = curInt->cb->getCasterAction(sp, castingHero, spells::Mode::HERO);

	if (spellSelMode == PossiblePlayerBattleAction::NO_LOCATION) //user does not have to select location
	{
		spellToCast->aimToHex(BattleHex::INVALID);
		curInt->cb->battleMakeAction(spellToCast.get());
		endCastingSpell();
	}
	else
	{
		possibleActions.clear();
		possibleActions.push_back (spellSelMode); //only this one action can be performed at the moment
		GH.fakeMouseMove();//update cursor
	}
}

void CBattleInterface::displayBattleLog(const std::vector<MetaString> & battleLog)
{
	for(const auto & line : battleLog)
	{
		std::string formatted = line.toString();
		boost::algorithm::trim(formatted);
		if(!console->addText(formatted))
			logGlobal->warn("Too long battle log line");
	}
}

void CBattleInterface::displayCustomEffects(const std::vector<CustomEffectInfo> & customEffects)
{
	for(const CustomEffectInfo & one : customEffects)
	{
		if(one.sound != 0)
			CCS->soundh->playSound(soundBase::soundID(one.sound));
		const CStack * s = curInt->cb->battleGetStackByID(one.stack, false);
		if(s && one.effect != 0)
			displayEffect(one.effect, s->getPosition());
	}
}

void CBattleInterface::displayEffect(ui32 effect, BattleHex destTile)
{
	std::string customAnim = graphics->battleACToDef[effect][0];

	addNewAnim(new CEffectAnimation(this, customAnim, destTile));
}

void CBattleInterface::displaySpellAnimationQueue(const CSpell::TAnimationQueue & q, BattleHex destinationTile)
{
	for(const CSpell::TAnimation & animation : q)
	{
		if(animation.pause > 0)
			addNewAnim(new CDummyAnimation(this, animation.pause));
		else
			addNewAnim(new CEffectAnimation(this, animation.resourceName, destinationTile, false, animation.verticalPosition == VerticalPosition::BOTTOM));
	}
}

void CBattleInterface::displaySpellCast(SpellID spellID, BattleHex destinationTile)
{
	const CSpell * spell = spellID.toSpell();

	if(spell)
		displaySpellAnimationQueue(spell->animationInfo.cast, destinationTile);
}

void CBattleInterface::displaySpellEffect(SpellID spellID, BattleHex destinationTile)
{
	const CSpell *spell = spellID.toSpell();

	if(spell)
		displaySpellAnimationQueue(spell->animationInfo.affect, destinationTile);
}

void CBattleInterface::displaySpellHit(SpellID spellID, BattleHex destinationTile)
{
	const CSpell * spell = spellID.toSpell();

	if(spell)
		displaySpellAnimationQueue(spell->animationInfo.hit, destinationTile);
}

void CBattleInterface::battleTriggerEffect(const BattleTriggerEffect & bte)
{
	const CStack * stack = curInt->cb->battleGetStackByID(bte.stackID);
	if(!stack)
	{
		logGlobal->error("Invalid stack ID %d", bte.stackID);
		return;
	}
	//don't show animation when no HP is regenerated
	switch(bte.effect)
	{
		//TODO: move to bonus type handler
		case Bonus::HP_REGENERATION:
			displayEffect(74, stack->getPosition());
			CCS->soundh->playSound(soundBase::REGENER);
			break;
		case Bonus::MANA_DRAIN:
			displayEffect(77, stack->getPosition());
			CCS->soundh->playSound(soundBase::MANADRAI);
			break;
		case Bonus::POISON:
			displayEffect(67, stack->getPosition());
			CCS->soundh->playSound(soundBase::POISON);
			break;
		case Bonus::FEAR:
			displayEffect(15, stack->getPosition());
			CCS->soundh->playSound(soundBase::FEAR);
			break;
		case Bonus::MORALE:
		{
			std::string hlp = CGI->generaltexth->allTexts[33];
			boost::algorithm::replace_first(hlp,"%s",(stack->getName()));
			displayEffect(20,stack->getPosition());
			CCS->soundh->playSound(soundBase::GOODMRLE);
			console->addText(hlp);
			break;
		}
		default:
			return;
	}
	//waitForAnims(); //fixme: freezes game :?
}

void CBattleInterface::setAnimSpeed(int set)
{
	Settings speed = settings.write["battle"]["animationSpeed"];
	speed->Float() = float(set) / 100;
}

int CBattleInterface::getAnimSpeed() const
{
	if(settings["session"]["spectate"].Bool() && !settings["session"]["spectate-battle-speed"].isNull())
		return static_cast<int>(vstd::round(settings["session"]["spectate-battle-speed"].Float() *100));

	return static_cast<int>(vstd::round(settings["battle"]["animationSpeed"].Float() *100));
}

CPlayerInterface *CBattleInterface::getCurrentPlayerInterface() const
{
	return curInt.get();
}

bool CBattleInterface::shouldRotate(const CStack * stack, const BattleHex & oldPos, const BattleHex & nextHex)
{
	Point begPosition = CClickableHex::getXYUnitAnim(oldPos,stack, this);
	Point endPosition = CClickableHex::getXYUnitAnim(nextHex, stack, this);

	if((begPosition.x > endPosition.x) && creDir[stack->ID])
		return true;
	else if((begPosition.x < endPosition.x) && !creDir[stack->ID])
		return true;

	return false;
}

void CBattleInterface::setActiveStack(const CStack *stack)
{
	if (activeStack) // update UI
		creAnims[activeStack->ID]->setBorderColor(AnimationControls::getNoBorder());

	activeStack = stack;

	if (activeStack) // update UI
		creAnims[activeStack->ID]->setBorderColor(AnimationControls::getGoldBorder());

	blockUI(activeStack == nullptr);
}

void CBattleInterface::setHoveredStack(const CStack *stack)
{
	if (mouseHoveredStack)
		creAnims[mouseHoveredStack->ID]->setBorderColor(AnimationControls::getNoBorder());

	// stack must be alive and not active (which uses gold border instead)
	if (stack && stack->alive() && stack != activeStack)
	{
		mouseHoveredStack = stack;

		if (mouseHoveredStack)
		{
			creAnims[mouseHoveredStack->ID]->setBorderColor(AnimationControls::getBlueBorder());
			if (creAnims[mouseHoveredStack->ID]->framesInGroup(CCreatureAnim::MOUSEON) > 0)
				creAnims[mouseHoveredStack->ID]->playOnce(CCreatureAnim::MOUSEON);
		}
	}
	else
		mouseHoveredStack = nullptr;
}

void CBattleInterface::activateStack()
{
	if(!battleActionsStarted)
		return; //"show" function should re-call this function

	myTurn = true;
	if (!!attackerInt && defenderInt) //hotseat -> need to pick which interface "takes over" as active
		curInt = attackerInt->playerID == stackToActivate->owner ? attackerInt : defenderInt;

	setActiveStack(stackToActivate);
	stackToActivate = nullptr;
	const CStack * s = activeStack;
	if(!s)
		return;

	queue->update();
	redrawBackgroundWithHexes(activeStack);

	//set casting flag to true if creature can use it to not check it every time
	const auto spellcaster = s->getBonusLocalFirst(Selector::type()(Bonus::SPELLCASTER)),
		randomSpellcaster = s->getBonusLocalFirst(Selector::type()(Bonus::RANDOM_SPELLCASTER));
	if(s->canCast() && (spellcaster || randomSpellcaster))
	{
		stackCanCastSpell = true;
		if(randomSpellcaster)
			creatureSpellToCast = -1; //spell will be set later on cast
		else
			creatureSpellToCast = curInt->cb->battleGetRandomStackSpell(CRandomGenerator::getDefault(), s, CBattleInfoCallback::RANDOM_AIMED); //faerie dragon can cast only one spell until their next move
		//TODO: what if creature can cast BOTH random genie spell and aimed spell?
		//TODO: faerie dragon type spell should be selected by server
	}
	else
	{
		stackCanCastSpell = false;
		creatureSpellToCast = -1;
	}

	possibleActions = getPossibleActionsForStack(s);

	GH.fakeMouseMove();
}

void CBattleInterface::endCastingSpell()
{
	if(spellDestSelectMode)
	{
		spellToCast.reset();

		sp = nullptr;
		spellDestSelectMode = false;
		CCS->curh->changeGraphic(ECursor::COMBAT, ECursor::COMBAT_POINTER);

		if(activeStack)
		{
			possibleActions = getPossibleActionsForStack(activeStack); //restore actions after they were cleared
			myTurn = true;
		}
	}
	else
	{
		if(activeStack)
		{
			possibleActions = getPossibleActionsForStack(activeStack);
			GH.fakeMouseMove();
		}
	}
}

void CBattleInterface::enterCreatureCastingMode()
{
	//silently check for possible errors
	if (!myTurn)
		return;

	if (tacticsMode)
		return;

	//hero is casting a spell
	if (spellDestSelectMode)
		return;

	if (!activeStack)
		return;

	if (!stackCanCastSpell)
		return;

	//random spellcaster
	if (creatureSpellToCast == -1)
		return;

	if (vstd::contains(possibleActions, PossiblePlayerBattleAction::NO_LOCATION))
	{
		const spells::Caster * caster = activeStack;
		const CSpell * spell = SpellID(creatureSpellToCast).toSpell();

		spells::Target target;
		target.emplace_back();

		spells::BattleCast cast(curInt->cb.get(), caster, spells::Mode::CREATURE_ACTIVE, spell);

		auto m = spell->battleMechanics(&cast);
		spells::detail::ProblemImpl ignored;

		const bool isCastingPossible = m->canBeCastAt(target, ignored);

		if (isCastingPossible)
		{
			myTurn = false;
			giveCommand(EActionType::MONSTER_SPELL, BattleHex::INVALID, creatureSpellToCast);
			selectedStack = nullptr;

			CCS->curh->changeGraphic(ECursor::COMBAT, ECursor::COMBAT_POINTER);
		}
	}
	else
	{
		possibleActions = getPossibleActionsForStack(activeStack);

		auto actionFilterPredicate = [](const PossiblePlayerBattleAction x)
		{
			return (x != PossiblePlayerBattleAction::ANY_LOCATION) && (x != PossiblePlayerBattleAction::NO_LOCATION) &&
				(x != PossiblePlayerBattleAction::FREE_LOCATION) && (x != PossiblePlayerBattleAction::AIMED_SPELL_CREATURE) &&
				(x != PossiblePlayerBattleAction::OBSTACLE);
		};

		vstd::erase_if(possibleActions, actionFilterPredicate);
		GH.fakeMouseMove();
	}
}

std::vector<PossiblePlayerBattleAction> CBattleInterface::getPossibleActionsForStack(const CStack *stack)
{
	BattleClientInterfaceData data; //hard to get rid of these things so for now they're required data to pass
	data.creatureSpellToCast = creatureSpellToCast;
	data.tacticsMode = tacticsMode;
	auto allActions = curInt->cb->getClientActionsForStack(stack, data);

	return std::vector<PossiblePlayerBattleAction>(allActions);
}

void CBattleInterface::reorderPossibleActionsPriority(const CStack * stack, MouseHoveredHexContext context)
{
	if(tacticsMode || possibleActions.empty()) return; //this function is not supposed to be called in tactics mode or before getPossibleActionsForStack

	auto assignPriority = [&](PossiblePlayerBattleAction const & item) -> uint8_t //large lambda assigning priority which would have to be part of possibleActions without it
	{
		switch(item)
		{
		case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
		case PossiblePlayerBattleAction::ANY_LOCATION:
		case PossiblePlayerBattleAction::NO_LOCATION:
		case PossiblePlayerBattleAction::FREE_LOCATION:
		case PossiblePlayerBattleAction::OBSTACLE:
			if(!stack->hasBonusOfType(Bonus::NO_SPELLCAST_BY_DEFAULT) && context == MouseHoveredHexContext::OCCUPIED_HEX)
				return 1;
			else
				return 100;//bottom priority
			break;
		case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
			return 2; break;
		case PossiblePlayerBattleAction::RISE_DEMONS:
			return 3; break;
		case PossiblePlayerBattleAction::SHOOT:
			return 4; break;
		case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
			return 5; break;
		case PossiblePlayerBattleAction::ATTACK:
			return 6; break;
		case PossiblePlayerBattleAction::WALK_AND_ATTACK:
			return 7; break;
		case PossiblePlayerBattleAction::MOVE_STACK:
			return 8; break;
		case PossiblePlayerBattleAction::CATAPULT:
			return 9; break;
		case PossiblePlayerBattleAction::HEAL:
			return 10; break;
		default:
			return 200; break;
		}
	};

	auto comparer = [&](PossiblePlayerBattleAction const & lhs, PossiblePlayerBattleAction const & rhs)
	{
		return assignPriority(lhs) > assignPriority(rhs);
	};

	std::make_heap(possibleActions.begin(), possibleActions.end(), comparer);
}

void CBattleInterface::endAction(const BattleAction* action)
{
	const CStack *stack = curInt->cb->battleGetStackByID(action->stackNumber);

	if(action->actionType == EActionType::HERO_SPELL)
		setHeroAnimation(action->side, 0);

	//check if we should reverse stacks
	//for some strange reason, it's not enough
	TStacks stacks = curInt->cb->battleGetStacks(CBattleCallback::MINE_AND_ENEMY);

	for (const CStack *s : stacks)
	{
		if (s && creDir[s->ID] != (s->side == BattleSide::ATTACKER) && s->alive()
		   && creAnims[s->ID]->isIdle())
		{
			addNewAnim(new CReverseAnimation(this, s, s->getPosition(), false));
		}
	}

	queue->update();

	if (tacticsMode) //stack ended movement in tactics phase -> select the next one
		bTacticNextStack(stack);

	if(action->actionType == EActionType::HERO_SPELL) //we have activated next stack after sending request that has been just realized -> blockmap due to movement has changed
		redrawBackgroundWithHexes(activeStack);

	if (activeStack && !animsAreDisplayed.get() && pendingAnims.empty() && !active)
	{
		logGlobal->warn("Something wrong... interface was deactivated but there is no animation. Reactivating...");
		blockUI(false);
	}
	else
	{
		// block UI if no active stack (e.g. enemy turn);
		blockUI(activeStack == nullptr);
	}
}

void CBattleInterface::hideQueue()
{
	Settings showQueue = settings.write["battle"]["showQueue"];
	showQueue->Bool() = false;

	queue->deactivate();

	if (!queue->embedded)
	{
		moveBy(Point(0, -queue->pos.h / 2));
		GH.totalRedraw();
	}
}

void CBattleInterface::showQueue()
{
	Settings showQueue = settings.write["battle"]["showQueue"];
	showQueue->Bool() = true;

	queue->activate();

	if (!queue->embedded)
	{
		moveBy(Point(0, +queue->pos.h / 2));
		GH.totalRedraw();
	}
}

void CBattleInterface::blockUI(bool on)
{
	bool canCastSpells = false;
	auto hero = curInt->cb->battleGetMyHero();

	if(hero)
	{
		ESpellCastProblem::ESpellCastProblem spellcastingProblem = curInt->cb->battleCanCastSpell(hero, spells::Mode::HERO);

		//if magic is blocked, we leave button active, so the message can be displayed after button click
		canCastSpells = spellcastingProblem == ESpellCastProblem::OK || spellcastingProblem == ESpellCastProblem::MAGIC_IS_BLOCKED;
	}

	bool canWait = activeStack ? !activeStack->waitedThisTurn : false;

	bOptions->block(on);
	bFlee->block(on || !curInt->cb->battleCanFlee());
	bSurrender->block(on || curInt->cb->battleGetSurrenderCost() < 0);

	// block only if during enemy turn and auto-fight is off
	// otherwise - crash on accessing non-exisiting active stack
	bAutofight->block(!curInt->isAutoFightOn && !activeStack);

	if (tacticsMode && btactEnd && btactNext)
	{
		btactNext->block(on);
		btactEnd->block(on);
	}
	else
	{
		bConsoleUp->block(on);
		bConsoleDown->block(on);
	}


	bSpell->block(on || tacticsMode || !canCastSpells);
	bWait->block(on || tacticsMode || !canWait);
	bDefence->block(on || tacticsMode);
}

void CBattleInterface::startAction(const BattleAction* action)
{
	//setActiveStack(nullptr);
	setHoveredStack(nullptr);
	blockUI(true);

	if(action->actionType == EActionType::END_TACTIC_PHASE)
	{
		SDL_FreeSurface(menu);
		menu = BitmapHandler::loadBitmap("CBAR.bmp");

		graphics->blueToPlayersAdv(menu, curInt->playerID);
		return;
	}

	const CStack *stack = curInt->cb->battleGetStackByID(action->stackNumber);

	if (stack)
	{
		queue->update();
	}
	else
	{
		assert(action->actionType == EActionType::HERO_SPELL); //only cast spell is valid action without acting stack number
	}

	auto actionTarget = action->getTarget(curInt->cb.get());

	if(action->actionType == EActionType::WALK
		|| (action->actionType == EActionType::WALK_AND_ATTACK && actionTarget.at(0).hexValue != stack->getPosition()))
	{
		assert(stack);
		moveStarted = true;
		if (creAnims[action->stackNumber]->framesInGroup(CCreatureAnim::MOVE_START))
		{
			pendingAnims.push_back(std::make_pair(new CMovementStartAnimation(this, stack), false));
		}

		if(shouldRotate(stack, stack->getPosition(), actionTarget.at(0).hexValue))
			pendingAnims.push_back(std::make_pair(new CReverseAnimation(this, stack, stack->getPosition(), true), false));
	}

	redraw(); // redraw after deactivation, including proper handling of hovered hexes

	if(action->actionType == EActionType::HERO_SPELL) //when hero casts spell
	{
		setHeroAnimation(action->side, 4);
		return;
	}

	if (!stack)
	{
		logGlobal->error("Something wrong with stackNumber in actionStarted. Stack number: %d", action->stackNumber);
		return;
	}

	int txtid = 0;
	switch(action->actionType)
	{
	case EActionType::WAIT:
		txtid = 136;
		break;
	case EActionType::BAD_MORALE:
		txtid = -34; //negative -> no separate singular/plural form
		displayEffect(30, stack->getPosition());
		CCS->soundh->playSound(soundBase::BADMRLE);
		break;
	}

	if(txtid != 0)
		console->addText(stack->formatGeneralMessage(txtid));

	//displaying special abilities
	switch(action->actionType)
	{
		case EActionType::STACK_HEAL:
			displayEffect(74, actionTarget.at(0).hexValue);
			CCS->soundh->playSound(soundBase::REGENER);
			break;
	}
}

void CBattleInterface::waitForAnims()
{
	auto unlockPim = vstd::makeUnlockGuard(*CPlayerInterface::pim);
	animsAreDisplayed.waitWhileTrue();
}

void CBattleInterface::bEndTacticPhase()
{
	setActiveStack(nullptr);
	blockUI(true);
	tacticsMode = false;
}

static bool immobile(const CStack *s)
{
	return !s->Speed(0, true); //should bound stacks be immobile?
}

void CBattleInterface::bTacticNextStack(const CStack * current)
{
	if (!current)
		current = activeStack;

	//no switching stacks when the current one is moving
	waitForAnims();

	TStacks stacksOfMine = tacticianInterface->cb->battleGetStacks(CBattleCallback::ONLY_MINE);
	vstd::erase_if (stacksOfMine, &immobile);
	if (stacksOfMine.empty())
	{
		bEndTacticPhase();
		return;
	}

	auto it = vstd::find(stacksOfMine, current);
	if (it != stacksOfMine.end() && ++it != stacksOfMine.end())
		stackActivated(*it);
	else
		stackActivated(stacksOfMine.front());

}

std::string formatDmgRange(std::pair<ui32, ui32> dmgRange)
{
	if (dmgRange.first != dmgRange.second)
		return (boost::format("%d - %d") % dmgRange.first % dmgRange.second).str();
	else
		return (boost::format("%d") % dmgRange.first).str();
}

bool CBattleInterface::canStackMoveHere(const CStack * activeStack, BattleHex myNumber)
{
	std::vector<BattleHex> acc = curInt->cb->battleGetAvailableHexes(activeStack);
	BattleHex shiftedDest = myNumber.cloneInDirection(activeStack->destShiftDir(), false);

	if (vstd::contains(acc, myNumber))
		return true;
	else if (activeStack->doubleWide() && vstd::contains(acc, shiftedDest))
		return true;
	else
		return false;
}

void CBattleInterface::handleHex(BattleHex myNumber, int eventType)
{
	if (!myTurn || !battleActionsStarted) //we are not permit to do anything
		return;

	// This function handles mouse move over hexes and l-clicking on them.
	// First we decide what happens if player clicks on this hex and set appropriately
	// consoleMsg, cursorFrame/Type and prepare lambda realizeAction.
	//
	// Then, depending whether it was hover/click we either call the action or set tooltip/cursor.

	//used when hovering -> tooltip message and cursor to be set
	std::string consoleMsg;
	bool setCursor = true; //if we want to suppress setting cursor
	ECursor::ECursorTypes cursorType = ECursor::COMBAT;
	int cursorFrame = ECursor::COMBAT_POINTER; //TODO: is this line used?

	//used when l-clicking -> action to be called upon the click
	std::function<void()> realizeAction;

	//Get stack on the hex - first try to grab the alive one, if not found -> allow dead stacks.
	const CStack * shere = curInt->cb->battleGetStackByPos(myNumber, true);
	if(!shere)
		shere = curInt->cb->battleGetStackByPos(myNumber, false);

	if(!activeStack)
		return;

	bool ourStack = false;
	if (shere)
		ourStack = shere->owner == curInt->playerID;

	//stack changed, update selection border
	if (shere != mouseHoveredStack)
	{
		setHoveredStack(shere);
	}

	localActions.clear();
	illegalActions.clear();

	reorderPossibleActionsPriority(activeStack, shere ? MouseHoveredHexContext::OCCUPIED_HEX : MouseHoveredHexContext::UNOCCUPIED_HEX);
	const bool forcedAction = possibleActions.size() == 1;

	for (PossiblePlayerBattleAction action : possibleActions)
	{
		bool legalAction = false; //this action is legal and can be performed
		bool notLegal = false; //this action is not legal and should display message

		switch (action)
		{
			case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
				if (shere && ourStack)
					legalAction = true;
				break;
			case PossiblePlayerBattleAction::MOVE_TACTICS:
			case PossiblePlayerBattleAction::MOVE_STACK:
			{
				if (!(shere && shere->alive())) //we can walk on dead stacks
				{
					if(canStackMoveHere(activeStack, myNumber))
						legalAction = true;
				}
				break;
			}
			case PossiblePlayerBattleAction::ATTACK:
			case PossiblePlayerBattleAction::WALK_AND_ATTACK:
			case PossiblePlayerBattleAction::ATTACK_AND_RETURN:
			{
				if(curInt->cb->battleCanAttack(activeStack, shere, myNumber))
				{
					if (isTileAttackable(myNumber)) // move isTileAttackable to be part of battleCanAttack?
					{
						setBattleCursor(myNumber); // temporary - needed for following function :(
						BattleHex attackFromHex = fromWhichHexAttack(myNumber);

						if (attackFromHex >= 0) //we can be in this line when unreachable creature is L - clicked (as of revision 1308)
							legalAction = true;
					}
				}
			}
				break;
			case PossiblePlayerBattleAction::SHOOT:
				if(curInt->cb->battleCanShoot(activeStack, myNumber))
					legalAction = true;
				break;
			case PossiblePlayerBattleAction::ANY_LOCATION:
				if (myNumber > -1) //TODO: this should be checked for all actions
				{
					if(isCastingPossibleHere(activeStack, shere, myNumber))
						legalAction = true;
				}
				break;
			case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
				if(shere && isCastingPossibleHere(activeStack, shere, myNumber))
					legalAction = true;
				break;
			case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
			{
				if(shere && ourStack && shere != activeStack && shere->alive()) //only positive spells for other allied creatures
				{
					int spellID = curInt->cb->battleGetRandomStackSpell(CRandomGenerator::getDefault(), shere, CBattleInfoCallback::RANDOM_GENIE);
					if(spellID > -1)
					{
						legalAction = true;
					}
				}
			}
				break;
			case PossiblePlayerBattleAction::OBSTACLE:
				if(isCastingPossibleHere(activeStack, shere, myNumber))
					legalAction = true;
				break;
			case PossiblePlayerBattleAction::TELEPORT:
			{
				//todo: move to mechanics
				ui8 skill = 0;
				if (creatureCasting)
					skill = activeStack->getEffectLevel(SpellID(SpellID::TELEPORT).toSpell());
				else
					skill = getActiveHero()->getEffectLevel(SpellID(SpellID::TELEPORT).toSpell());
				//TODO: explicitely save power, skill
				if (curInt->cb->battleCanTeleportTo(selectedStack, myNumber, skill))
					legalAction = true;
				else
					notLegal = true;
			}
				break;
			case PossiblePlayerBattleAction::SACRIFICE: //choose our living stack to sacrifice
				if (shere && shere != selectedStack && ourStack && shere->alive())
					legalAction = true;
				else
					notLegal = true;
				break;
			case PossiblePlayerBattleAction::FREE_LOCATION:
				legalAction = true;
				if(!isCastingPossibleHere(activeStack, shere, myNumber))
				{
					legalAction = false;
					notLegal = true;
				}
				break;
			case PossiblePlayerBattleAction::CATAPULT:
				if (siegeController && siegeController->isCatapultAttackable(myNumber))
					legalAction = true;
				break;
			case PossiblePlayerBattleAction::HEAL:
				if (shere && ourStack && shere->canBeHealed())
					legalAction = true;
				break;
			case PossiblePlayerBattleAction::RISE_DEMONS:
				if (shere && ourStack && !shere->alive())
				{
					if (!(shere->hasBonusOfType(Bonus::UNDEAD)
						|| shere->hasBonusOfType(Bonus::NON_LIVING)
						|| shere->hasBonusOfType(Bonus::GARGOYLE)
						|| shere->summoned
						|| shere->isClone()
						|| shere->hasBonusOfType(Bonus::SIEGE_WEAPON)
						))
						legalAction = true;
				}
				break;
		}
		if (legalAction)
			localActions.push_back (action);
		else if (notLegal || forcedAction)
			illegalActions.push_back (action);
	}
	illegalAction = PossiblePlayerBattleAction::INVALID; //clear it in first place

	if (vstd::contains(localActions, selectedAction)) //try to use last selected action by default
		currentAction = selectedAction;
	else if (localActions.size()) //if not possible, select first available action (they are sorted by suggested priority)
		currentAction = localActions.front();
	else //no legal action possible
	{
		currentAction = PossiblePlayerBattleAction::INVALID; //don't allow to do anything

		if (vstd::contains(illegalActions, selectedAction))
			illegalAction = selectedAction;
		else if (illegalActions.size())
			illegalAction = illegalActions.front();
		else if (shere && ourStack && shere->alive()) //last possibility - display info about our creature
		{
			currentAction = PossiblePlayerBattleAction::CREATURE_INFO;
		}
		else
			illegalAction = PossiblePlayerBattleAction::INVALID; //we should never be here
	}

	bool isCastingPossible = false;
	bool secondaryTarget = false;

	if (currentAction > PossiblePlayerBattleAction::INVALID)
	{
		switch (currentAction) //display console message, realize selected action
		{
			case PossiblePlayerBattleAction::CHOOSE_TACTICS_STACK:
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[481]) % shere->getName()).str(); //Select %s
				realizeAction = [=](){ stackActivated(shere); };
				break;
			case PossiblePlayerBattleAction::MOVE_TACTICS:
			case PossiblePlayerBattleAction::MOVE_STACK:
				if (activeStack->hasBonusOfType(Bonus::FLYING))
				{
					cursorFrame = ECursor::COMBAT_FLY;
					consoleMsg = (boost::format(CGI->generaltexth->allTexts[295]) % activeStack->getName()).str(); //Fly %s here
				}
				else
				{
					cursorFrame = ECursor::COMBAT_MOVE;
					consoleMsg = (boost::format(CGI->generaltexth->allTexts[294]) % activeStack->getName()).str(); //Move %s here
				}

				realizeAction = [=]()
				{
					if(activeStack->doubleWide())
					{
						std::vector<BattleHex> acc = curInt->cb->battleGetAvailableHexes(activeStack);
						BattleHex shiftedDest = myNumber.cloneInDirection(activeStack->destShiftDir(), false);
						if(vstd::contains(acc, myNumber))
							giveCommand(EActionType::WALK, myNumber);
						else if(vstd::contains(acc, shiftedDest))
							giveCommand(EActionType::WALK, shiftedDest);
					}
					else
					{
						giveCommand(EActionType::WALK, myNumber);
					}
				};
				break;
			case PossiblePlayerBattleAction::ATTACK:
			case PossiblePlayerBattleAction::WALK_AND_ATTACK:
			case PossiblePlayerBattleAction::ATTACK_AND_RETURN: //TODO: allow to disable return
				{
					setBattleCursor(myNumber); //handle direction of cursor and attackable tile
					setCursor = false; //don't overwrite settings from the call above //TODO: what does it mean?

					bool returnAfterAttack = currentAction == PossiblePlayerBattleAction::ATTACK_AND_RETURN;

					realizeAction = [=]()
					{
						BattleHex attackFromHex = fromWhichHexAttack(myNumber);
						if(attackFromHex.isValid()) //we can be in this line when unreachable creature is L - clicked (as of revision 1308)
						{
							auto command = new BattleAction(BattleAction::makeMeleeAttack(activeStack, myNumber, attackFromHex, returnAfterAttack));
							sendCommand(command, activeStack);
						}
					};

					TDmgRange damage = curInt->cb->battleEstimateDamage(activeStack, shere);
					std::string estDmgText = formatDmgRange(std::make_pair((ui32)damage.first, (ui32)damage.second)); //calculating estimated dmg
					consoleMsg = (boost::format(CGI->generaltexth->allTexts[36]) % shere->getName() % estDmgText).str(); //Attack %s (%s damage)
				}
				break;
			case PossiblePlayerBattleAction::SHOOT:
			{
				if (curInt->cb->battleHasShootingPenalty(activeStack, myNumber))
					cursorFrame = ECursor::COMBAT_SHOOT_PENALTY;
				else
					cursorFrame = ECursor::COMBAT_SHOOT;

				realizeAction = [=](){giveCommand(EActionType::SHOOT, myNumber);};
				TDmgRange damage = curInt->cb->battleEstimateDamage(activeStack, shere);
				std::string estDmgText = formatDmgRange(std::make_pair((ui32)damage.first, (ui32)damage.second)); //calculating estimated dmg
				//printing - Shoot %s (%d shots left, %s damage)
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[296]) % shere->getName() % activeStack->shots.available() % estDmgText).str();
			}
				break;
			case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
				sp = CGI->spellh->objects[creatureCasting ? creatureSpellToCast : spellToCast->actionSubtype]; //necessary if creature has random Genie spell at same time
				consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[27]) % sp->name % shere->getName()); //Cast %s on %s
				switch (sp->id)
				{
					case SpellID::SACRIFICE:
					case SpellID::TELEPORT:
						selectedStack = shere; //remember first target
						secondaryTarget = true;
						break;
				}
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::ANY_LOCATION:
				sp = CGI->spellh->objects[creatureCasting ? creatureSpellToCast : spellToCast->actionSubtype]; //necessary if creature has random Genie spell at same time
				consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[26]) % sp->name); //Cast %s
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL: //we assume that teleport / sacrifice will never be available as random spell
				sp = nullptr;
				consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[301]) % shere->getName()); //Cast a spell on %
				creatureCasting = true;
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::TELEPORT:
				consoleMsg = CGI->generaltexth->allTexts[25]; //Teleport Here
				cursorFrame = ECursor::COMBAT_TELEPORT;
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::OBSTACLE:
				consoleMsg = CGI->generaltexth->allTexts[550];
				//TODO: remove obstacle cursor
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::SACRIFICE:
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[549]) % shere->getName()).str(); //sacrifice the %s
				cursorFrame = ECursor::COMBAT_SACRIFICE;
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::FREE_LOCATION:
				consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[26]) % sp->name); //Cast %s
				isCastingPossible = true;
				break;
			case PossiblePlayerBattleAction::HEAL:
				cursorFrame = ECursor::COMBAT_HEAL;
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[419]) % shere->getName()).str(); //Apply first aid to the %s
				realizeAction = [=](){ giveCommand(EActionType::STACK_HEAL, myNumber); }; //command healing
				break;
			case PossiblePlayerBattleAction::RISE_DEMONS:
				cursorType = ECursor::SPELLBOOK;
				realizeAction = [=]()
				{
					giveCommand(EActionType::DAEMON_SUMMONING, myNumber);
				};
				break;
			case PossiblePlayerBattleAction::CATAPULT:
				cursorFrame = ECursor::COMBAT_SHOOT_CATAPULT;
				realizeAction = [=](){ giveCommand(EActionType::CATAPULT, myNumber); };
				break;
			case PossiblePlayerBattleAction::CREATURE_INFO:
			{
				cursorFrame = ECursor::COMBAT_QUERY;
				consoleMsg = (boost::format(CGI->generaltexth->allTexts[297]) % shere->getName()).str();
				realizeAction = [=](){ GH.pushIntT<CStackWindow>(shere, false); };
				break;
			}
		}
	}
	else //no possible valid action, display message
	{
		switch (illegalAction)
		{
			case PossiblePlayerBattleAction::AIMED_SPELL_CREATURE:
			case PossiblePlayerBattleAction::RANDOM_GENIE_SPELL:
				cursorFrame = ECursor::COMBAT_BLOCKED;
				consoleMsg = CGI->generaltexth->allTexts[23];
				break;
			case PossiblePlayerBattleAction::TELEPORT:
				cursorFrame = ECursor::COMBAT_BLOCKED;
				consoleMsg = CGI->generaltexth->allTexts[24]; //Invalid Teleport Destination
				break;
			case PossiblePlayerBattleAction::SACRIFICE:
				consoleMsg = CGI->generaltexth->allTexts[543]; //choose army to sacrifice
				break;
			case PossiblePlayerBattleAction::FREE_LOCATION:
				cursorFrame = ECursor::COMBAT_BLOCKED;
				consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[181]) % sp->name); //No room to place %s here
				break;
			default:
				if (myNumber == -1)
					CCS->curh->changeGraphic(ECursor::COMBAT, ECursor::COMBAT_POINTER); //set neutral cursor over menu etc.
				else
					cursorFrame = ECursor::COMBAT_BLOCKED;
				break;
		}
	}

	if (isCastingPossible) //common part
	{
		switch (currentAction) //don't use that with teleport / sacrifice
		{
			case PossiblePlayerBattleAction::TELEPORT: //FIXME: more generic solution?
			case PossiblePlayerBattleAction::SACRIFICE:
				break;
			default:
				cursorType = ECursor::SPELLBOOK;
				cursorFrame = 0;
				if (consoleMsg.empty() && sp)
					consoleMsg = boost::str(boost::format(CGI->generaltexth->allTexts[26]) % sp->name); //Cast %s
				break;
		}

		realizeAction = [=]()
		{
			if(secondaryTarget) //select that target now
			{

				possibleActions.clear();
				switch (sp->id.toEnum())
				{
					case SpellID::TELEPORT: //don't cast spell yet, only select target
						spellToCast->aimToUnit(shere);
						possibleActions.push_back(PossiblePlayerBattleAction::TELEPORT);
						break;
					case SpellID::SACRIFICE:
						spellToCast->aimToHex(myNumber);
						possibleActions.push_back(PossiblePlayerBattleAction::SACRIFICE);
						break;
				}
			}
			else
			{
				if (creatureCasting)
				{
					if (sp)
					{
						giveCommand(EActionType::MONSTER_SPELL, myNumber, creatureSpellToCast);
					}
					else //unknown random spell
					{
						giveCommand(EActionType::MONSTER_SPELL, myNumber);
					}
				}
				else
				{
					assert(sp);
					switch (sp->id.toEnum())
					{
					case SpellID::SACRIFICE:
						spellToCast->aimToUnit(shere);//victim
						break;
					default:
						spellToCast->aimToHex(myNumber);
						break;
					}
					curInt->cb->battleMakeAction(spellToCast.get());
					endCastingSpell();
				}
				selectedStack = nullptr;
			}
		};
	}

	{
		if (eventType == MOVE)
		{
			if (setCursor)
				CCS->curh->changeGraphic(cursorType, cursorFrame);
			this->console->alterText(consoleMsg);
			this->console->whoSetAlter = 0;
		}
		if (eventType == LCLICK && realizeAction)
		{
			//opening creature window shouldn't affect myTurn...
			if ((currentAction != PossiblePlayerBattleAction::CREATURE_INFO) && !secondaryTarget)
			{
				myTurn = false; //tends to crash with empty calls
			}
			realizeAction();
			if (!secondaryTarget) //do not replace teleport or sacrifice cursor
				CCS->curh->changeGraphic(ECursor::COMBAT, ECursor::COMBAT_POINTER);
			this->console->alterText("");
		}
	}
}

bool CBattleInterface::isCastingPossibleHere(const CStack *sactive, const CStack *shere, BattleHex myNumber)
{
	creatureCasting = stackCanCastSpell && !spellDestSelectMode; //TODO: allow creatures to cast aimed spells

	bool isCastingPossible = true;

	int spellID = -1;
	if (creatureCasting)
	{
		if (creatureSpellToCast > -1 && (shere != sactive)) //can't cast on itself
			spellID = creatureSpellToCast; //TODO: merge with SpellTocast?
	}
	else //hero casting
	{
		spellID = spellToCast->actionSubtype;
	}


	sp = nullptr;
	if (spellID >= 0)
		sp = CGI->spellh->objects[spellID];

	if (sp)
	{
		const spells::Caster *caster = creatureCasting ? static_cast<const spells::Caster *>(sactive) : static_cast<const spells::Caster *>(curInt->cb->battleGetMyHero());
		if (caster == nullptr)
		{
			isCastingPossible = false;//just in case
		}
		else
		{
			const spells::Mode mode = creatureCasting ? spells::Mode::CREATURE_ACTIVE : spells::Mode::HERO;

			spells::Target target;
			target.emplace_back(myNumber);

			spells::BattleCast cast(curInt->cb.get(), caster, mode, sp);

			auto m = sp->battleMechanics(&cast);
			spells::detail::ProblemImpl problem; //todo: display problem in status bar

			isCastingPossible = m->canBeCastAt(target, problem);
		}
	}
	else
		isCastingPossible = false;
	if (!myNumber.isAvailable() && !shere) //empty tile outside battlefield (or in the unavailable border column)
			isCastingPossible = false;

	return isCastingPossible;
}

BattleHex CBattleInterface::fromWhichHexAttack(BattleHex myNumber)
{
	//TODO far too much repeating code
	BattleHex destHex;
	switch(CCS->curh->frame)
	{
	case 12: //from bottom right
		{
			bool doubleWide = activeStack->doubleWide();
			destHex = myNumber + ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH+1 ) +
				(activeStack->side == BattleSide::ATTACKER && doubleWide ? 1 : 0);
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(activeStack->side == BattleSide::ATTACKER)
			{
				if (vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 7: //from bottom left
		{
			destHex = myNumber + ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH-1 : GameConstants::BFIELD_WIDTH );
			if (vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(activeStack->side == BattleSide::ATTACKER)
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 8: //from left
		{
			if(activeStack->doubleWide() && activeStack->side == BattleSide::DEFENDER)
			{
				std::vector<BattleHex> acc = curInt->cb->battleGetAvailableHexes(activeStack);
				if (vstd::contains(acc, myNumber))
					return myNumber - 1;
				else
					return myNumber - 2;
			}
			else
			{
				return myNumber - 1;
			}
			break;
		}
	case 9: //from top left
		{
			destHex = myNumber - ((myNumber/GameConstants::BFIELD_WIDTH) % 2 ? GameConstants::BFIELD_WIDTH + 1 : GameConstants::BFIELD_WIDTH);
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(activeStack->side == BattleSide::ATTACKER)
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 10: //from top right
		{
			bool doubleWide = activeStack->doubleWide();
			destHex = myNumber - ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH-1 ) +
				(activeStack->side == BattleSide::ATTACKER && doubleWide ? 1 : 0);
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(activeStack->side == BattleSide::ATTACKER)
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 11: //from right
		{
			if(activeStack->doubleWide() && activeStack->side == BattleSide::ATTACKER)
			{
				std::vector<BattleHex> acc = curInt->cb->battleGetAvailableHexes(activeStack);
				if(vstd::contains(acc, myNumber))
					return myNumber + 1;
				else
					return myNumber + 2;
			}
			else
			{
				return myNumber + 1;
			}
			break;
		}
	case 13: //from bottom
		{
			destHex = myNumber + ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH+1 );
			if(vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(activeStack->side == BattleSide::ATTACKER)
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	case 14: //from top
		{
			destHex = myNumber - ( (myNumber/GameConstants::BFIELD_WIDTH)%2 ? GameConstants::BFIELD_WIDTH : GameConstants::BFIELD_WIDTH-1 );
			if (vstd::contains(occupyableHexes, destHex))
				return destHex;
			else if(activeStack->side == BattleSide::ATTACKER)
			{
				if(vstd::contains(occupyableHexes, destHex+1))
					return destHex+1;
			}
			else //if we are defender
			{
				if(vstd::contains(occupyableHexes, destHex-1))
					return destHex-1;
			}
			break;
		}
	}
	return -1;
}

Rect CBattleInterface::hexPosition(BattleHex hex) const
{
	int x = 14 + ((hex.getY())%2==0 ? 22 : 0) + 44*hex.getX() + pos.x;
	int y = 86 + 42 *hex.getY() + pos.y;
	int w = cellShade->w;
	int h = cellShade->h;
	return Rect(x, y, w, h);
}

void CBattleInterface::obstaclePlaced(const CObstacleInstance & oi)
{
	//so when multiple obstacles are added, they show up one after another
	waitForAnims();

	//soundBase::soundID sound; // FIXME(v.markovtsev): soundh->playSound() is commented in the end => warning

	std::string defname;

	switch(oi.obstacleType)
	{
	case CObstacleInstance::SPELL_CREATED:
		{
			auto &spellObstacle = dynamic_cast<const SpellCreatedObstacle&>(oi);
			defname = spellObstacle.appearAnimation;
			//TODO: sound
			//soundBase::QUIKSAND
			//soundBase::LANDMINE
			//soundBase::FORCEFLD
			//soundBase::fireWall
		}
		break;
	default:
		logGlobal->error("I don't know how to animate appearing obstacle of type %d", (int)oi.obstacleType);
		return;
	}

	auto animation = std::make_shared<CAnimation>(defname);
	animation->preload();

	auto first = animation->getImage(0, 0);
	if(!first)
		return;

	//we assume here that effect graphics have the same size as the usual obstacle image
	// -> if we know how to blit obstacle, let's blit the effect in the same place
	Point whereTo = getObstaclePosition(first, oi);
	addNewAnim(new CEffectAnimation(this, animation, whereTo.x, whereTo.y));

	//TODO we need to wait after playing sound till it's finished, otherwise it overlaps and sounds really bad
	//CCS->soundh->playSound(sound);
}

const CGHeroInstance *CBattleInterface::currentHero() const
{
	if (attackingHeroInstance->tempOwner == curInt->playerID)
		return attackingHeroInstance;
	else
		return defendingHeroInstance;
}

InfoAboutHero CBattleInterface::enemyHero() const
{
	InfoAboutHero ret;
	if (attackingHeroInstance->tempOwner == curInt->playerID)
		curInt->cb->getHeroInfo(defendingHeroInstance, ret);
	else
		curInt->cb->getHeroInfo(attackingHeroInstance, ret);

	return ret;
}

void CBattleInterface::requestAutofightingAIToTakeAction()
{
	assert(curInt->isAutoFightOn);

	boost::thread aiThread([&]()
	{
		auto ba = make_unique<BattleAction>(curInt->autofightingAI->activeStack(activeStack));

		if(curInt->cb->battleIsFinished())
		{
			return; // battle finished with spellcast
		}

		if (curInt->isAutoFightOn)
		{
			if (tacticsMode)
			{
				// Always end tactics mode. Player interface is blocked currently, so it's not possible that
				// the AI can take any action except end tactics phase (AI actions won't be triggered)
				//TODO implement the possibility that the AI will be triggered for further actions
				//TODO any solution to merge tactics phase & normal phase in the way it is handled by the player and battle interface?
				setActiveStack(nullptr);
				blockUI(true);
				tacticsMode = false;
			}
			else
			{
				givenCommand.setn(ba.release());
			}
		}
		else
		{
			boost::unique_lock<boost::recursive_mutex> un(*CPlayerInterface::pim);
			activateStack();
		}
	});

	aiThread.detach();
}

void CBattleInterface::showAll(SDL_Surface *to)
{
	show(to);
}

void CBattleInterface::show(SDL_Surface *to)
{
	assert(to);

	SDL_Rect buf;
	SDL_GetClipRect(to, &buf);
	SDL_SetClipRect(to, &pos);

	++animCount;

	showBackground(to);
	showBattlefieldObjects(to);
	projectilesController->showProjectiles(to);

	if(battleActionsStarted)
		updateBattleAnimations();

	SDL_SetClipRect(to, &buf); //restoring previous clip_rect

	showInterface(to);

	//activation of next stack
	if (pendingAnims.empty() && stackToActivate != nullptr && battleActionsStarted) //FIXME: watch for recursive infinite loop here when working with this file, this needs rework anyway...
	{
		activateStack();

		//we may have changed active interface (another side in hot-seat),
		// so we can't continue drawing with old setting.
		show(to);
	}
}

void CBattleInterface::showBackground(SDL_Surface *to)
{
	if (activeStack != nullptr && creAnims[activeStack->ID]->isIdle()) //show everything with range
	{
		// FIXME: any *real* reason to keep this separate? Speed difference can't be that big
		blitAt(backgroundWithHexes, pos.x, pos.y, to);
	}
	else
	{
		showBackgroundImage(to);
		showAbsoluteObstacles(to);
	}
	showHighlightedHexes(to);
}

void CBattleInterface::showBackgroundImage(SDL_Surface *to)
{
	blitAt(background, pos.x, pos.y, to);
	if (settings["battle"]["cellBorders"].Bool())
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(cellBorders, nullptr, to, &pos);
	}
}

void CBattleInterface::showAbsoluteObstacles(SDL_Surface * to)
{
	//Blit absolute obstacles
	for(auto & oi : curInt->cb->battleGetAllObstacles())
	{
		if(oi->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			auto img = getObstacleImage(*oi);
			if(img)
				img->draw(to, pos.x + oi->getInfo().width, pos.y + oi->getInfo().height);
		}
	}

	if ( siegeController )
		siegeController->showAbsoluteObstacles(to);
}

void CBattleInterface::showHighlightedHexes(SDL_Surface *to)
{
	bool delayedBlit = false; //workaround for blitting enemy stack hex without mouse shadow with stack range on
	if(activeStack && settings["battle"]["stackRange"].Bool())
	{
		std::set<BattleHex> set = curInt->cb->battleGetAttackedHexes(activeStack, currentlyHoveredHex, attackingHex);
		for(BattleHex hex : set)
			if(hex != currentlyHoveredHex)
				showHighlightedHex(to, hex);

		// display the movement shadow of the stack at b (i.e. stack under mouse)
		const CStack * const shere = curInt->cb->battleGetStackByPos(currentlyHoveredHex, false);
		if(shere && shere != activeStack && shere->alive())
		{
			std::vector<BattleHex> v = curInt->cb->battleGetAvailableHexes(shere, true, nullptr);
			for(BattleHex hex : v)
			{
				if(hex != currentlyHoveredHex)
					showHighlightedHex(to, hex);
				else if(!settings["battle"]["mouseShadow"].Bool())
					delayedBlit = true; //blit at the end of method to avoid graphic artifacts
				else
					showHighlightedHex(to, hex, true); //blit now and blit 2nd time later for darker shadow - avoids graphic artifacts
			}
		}
	}

	for(int b=0; b<GameConstants::BFIELD_SIZE; ++b)
	{
		if(bfield[b]->strictHovered && bfield[b]->hovered)
		{
			if(previouslyHoveredHex == -1)
				previouslyHoveredHex = b; //something to start with
			if(currentlyHoveredHex == -1)
				currentlyHoveredHex = b; //something to start with

			if(currentlyHoveredHex != b) //repair hover info
			{
				previouslyHoveredHex = currentlyHoveredHex;
				currentlyHoveredHex = b;
			}
			if(settings["battle"]["mouseShadow"].Bool() || delayedBlit)
			{
				const spells::Caster *caster = nullptr;
				const CSpell *spell = nullptr;

				spells::Mode mode = spells::Mode::HERO;

				if(spellToCast)//hero casts spell
				{
					spell = SpellID(spellToCast->actionSubtype).toSpell();
					caster = getActiveHero();
				}
				else if(creatureSpellToCast >= 0 && stackCanCastSpell && creatureCasting)//stack casts spell
				{
					spell = SpellID(creatureSpellToCast).toSpell();
					caster = activeStack;
					mode = spells::Mode::CREATURE_ACTIVE;
				}

				if(caster && spell) //when casting spell
				{
					// printing shaded hex(es)
					spells::BattleCast event(curInt->cb.get(), caster, mode, spell);
					auto shaded = spell->battleMechanics(&event)->rangeInHexes(currentlyHoveredHex);

					for(BattleHex shadedHex : shaded)
					{
						if((shadedHex.getX() != 0) && (shadedHex.getX() != GameConstants::BFIELD_WIDTH - 1))
							showHighlightedHex(to, shadedHex, true);
					}
				}
				else if(active || delayedBlit) //always highlight pointed hex, keep this condition last in this method for correct behavior
				{
					if(currentlyHoveredHex.getX() != 0
					 && currentlyHoveredHex.getX() != GameConstants::BFIELD_WIDTH - 1)
						showHighlightedHex(to, currentlyHoveredHex, true); //keep true for OH3 behavior: hovered hex frame "thinner"
				}
			}
		}
	}
}

void CBattleInterface::showHighlightedHex(SDL_Surface *to, BattleHex hex, bool darkBorder)
{
	int x = 14 + (hex.getY() % 2 == 0 ? 22 : 0) + 44 *(hex.getX()) + pos.x;
	int y = 86 + 42 *hex.getY() + pos.y;
	SDL_Rect temp_rect = genRect (cellShade->h, cellShade->w, x, y);
	CSDL_Ext::blit8bppAlphaTo24bpp (cellShade, nullptr, to, &temp_rect);
	if(!darkBorder && settings["battle"]["cellBorders"].Bool())
		CSDL_Ext::blit8bppAlphaTo24bpp(cellBorder, nullptr, to, &temp_rect); //redraw border to make it light green instead of shaded
}

void CBattleInterface::showBattlefieldObjects(SDL_Surface *to)
{
	auto showHexEntry = [&](BattleObjectsByHex::HexData & hex)
	{
		if (siegeController)
			siegeController->showPiecesOfWall(to, hex.walls);
		showObstacles(to, hex.obstacles);
		showAliveStacks(to, hex.alive);
		showBattleEffects(to, hex.effects);
	};

	BattleObjectsByHex objects = sortObjectsByHex();

	// dead stacks should be blit first
	showStacks(to, objects.beforeAll.dead);
	for (auto & data : objects.hex)
		showStacks(to, data.dead);
	showStacks(to, objects.afterAll.dead);

	// display objects that must be blit before anything else (e.g. topmost walls)
	showHexEntry(objects.beforeAll);

	// show heroes after "beforeAll" - e.g. topmost wall in siege
	if (attackingHero)
		attackingHero->show(to);
	if (defendingHero)
		defendingHero->show(to);

	// actual blit of most of objects, hex by hex
	// NOTE: row-by-row blitting may be a better approach
	for (auto &data : objects.hex)
		showHexEntry(data);

	// objects that must be blit *after* everything else - e.g. bottom tower or some spell effects
	showHexEntry(objects.afterAll);
}

void CBattleInterface::showAliveStacks(SDL_Surface *to, std::vector<const CStack *> stacks)
{
	BattleHex currentActionTarget;
	if(curInt->curAction)
	{
		auto target = curInt->curAction->getTarget(curInt->cb.get());
		if(!target.empty())
			currentActionTarget = target.at(0).hexValue;
	}

	auto isAmountBoxVisible = [&](const CStack *stack) -> bool
	{
		if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON) && stack->getCount() == 1) //do not show box for singular war machines, stacked war machines with box shown are supported as extension feature
			return false;

		if(stack->getCount() == 0) //hide box when target is going to die anyway - do not display "0 creatures"
			return false;

		for(auto anim : pendingAnims) //no matter what other conditions below are, hide box when creature is playing hit animation
		{
			auto hitAnimation = dynamic_cast<CDefenceAnimation*>(anim.first);
			if(hitAnimation && (hitAnimation->stack->ID == stack->ID)) //we process only "current creature" as other creatures will be processed reliably on their own iteration
				return false;
		}

		if(curInt->curAction)
		{
			if(curInt->curAction->stackNumber == stack->ID) //stack is currently taking action (is not a target of another creature's action etc)
			{
				if(curInt->curAction->actionType == EActionType::WALK || curInt->curAction->actionType == EActionType::SHOOT) //hide when stack walks or shoots
					return false;

				else if(curInt->curAction->actionType == EActionType::WALK_AND_ATTACK && currentActionTarget != stack->getPosition()) //when attacking, hide until walk phase finished
					return false;
			}

			if(curInt->curAction->actionType == EActionType::SHOOT && currentActionTarget == stack->getPosition()) //hide if we are ranged attack target
				return false;
		}

		return true;
	};

	auto getEffectsPositivness = [&](const std::vector<si32> & activeSpells) -> int
	{
		int pos = 0;
		for (const auto & spellId : activeSpells)
		{
			pos += CGI->spellh->objects.at(spellId)->positiveness;
		}
		return pos;
	};

	auto getAmountBoxBackground = [&](int positivness) -> SDL_Surface *
	{
		if (positivness > 0)
			return amountPositive;
		if (positivness < 0)
			return amountNegative;
		return amountEffNeutral;
	};

	showStacks(to, stacks); // Actual display of all stacks

	for (auto & stack : stacks)
	{
		assert(stack);
		//printing amount
		if (isAmountBoxVisible(stack))
		{
			const int sideShift = stack->side == BattleSide::ATTACKER ? 1 : -1;
			const int reverseSideShift = stack->side == BattleSide::ATTACKER ? -1 : 1;
			const BattleHex nextPos = stack->getPosition() + sideShift;
			const bool edge = stack->getPosition() % GameConstants::BFIELD_WIDTH == (stack->side == BattleSide::ATTACKER ? GameConstants::BFIELD_WIDTH - 2 : 1);
			const bool moveInside = !edge && !stackCountOutsideHexes[nextPos];
			int xAdd = (stack->side == BattleSide::ATTACKER ? 220 : 202) +
					   (stack->doubleWide() ? 44 : 0) * sideShift +
					   (moveInside ? amountNormal->w + 10 : 0) * reverseSideShift;
			int yAdd = 260 + ((stack->side == BattleSide::ATTACKER || moveInside) ? 0 : -15);

			//blitting amount background box
			SDL_Surface *amountBG = amountNormal;
			std::vector<si32> activeSpells = stack->activeSpells();
			if (!activeSpells.empty())
				amountBG = getAmountBoxBackground(getEffectsPositivness(activeSpells));

			SDL_Rect temp_rect = genRect(amountBG->h, amountBG->w, creAnims[stack->ID]->pos.x + xAdd, creAnims[stack->ID]->pos.y + yAdd);
			SDL_BlitSurface(amountBG, nullptr, to, &temp_rect);

			//blitting amount
			Point textPos(creAnims[stack->ID]->pos.x + xAdd + amountNormal->w/2,
						  creAnims[stack->ID]->pos.y + yAdd + amountNormal->h/2);
			graphics->fonts[FONT_TINY]->renderTextCenter(to, makeNumberShort(stack->getCount()), Colors::WHITE, textPos);
		}
	}
}

void CBattleInterface::showStacks(SDL_Surface *to, std::vector<const CStack *> stacks)
{
	for (const CStack *stack : stacks)
	{
		creAnims[stack->ID]->nextFrame(to, creDir[stack->ID]); // do actual blit
		creAnims[stack->ID]->incrementFrame(float(GH.mainFPSmng->getElapsedMilliseconds()) / 1000);
	}
}

void CBattleInterface::showObstacles(SDL_Surface * to, std::vector<std::shared_ptr<const CObstacleInstance>> & obstacles)
{
	for(auto & obstacle : obstacles)
	{
		auto img = getObstacleImage(*obstacle);
		if(img)
		{
			Point p = getObstaclePosition(img, *obstacle);
			img->draw(to, p.x, p.y);
		}
	}
}

void CBattleInterface::showBattleEffects(SDL_Surface *to, const std::vector<const BattleEffect *> &battleEffects)
{
	for (auto & elem : battleEffects)
	{
		int currentFrame = static_cast<int>(floor(elem->currentFrame));
		currentFrame %= elem->animation->size();

		auto img = elem->animation->getImage(currentFrame);

		SDL_Rect temp_rect = genRect(img->height(), img->width(), elem->x, elem->y);

		img->draw(to, &temp_rect, nullptr);
	}
}

void CBattleInterface::showInterface(SDL_Surface *to)
{
	blitAt(menu, pos.x, 556 + pos.y, to);

	if (tacticsMode)
	{
		btactNext->showAll(to);
		btactEnd->showAll(to);
	}
	else
	{
		console->showAll(to);
		bConsoleUp->showAll(to);
		bConsoleDown->showAll(to);
	}

	//showing buttons
	bOptions->showAll(to);
	bSurrender->showAll(to);
	bFlee->showAll(to);
	bAutofight->showAll(to);
	bSpell->showAll(to);
	bWait->showAll(to);
	bDefence->showAll(to);

	//showing in-game console
	LOCPLINT->cingconsole->show(to);

	Rect posWithQueue = Rect(pos.x, pos.y, 800, 600);

	if (settings["battle"]["showQueue"].Bool())
	{
		if (!queue->embedded)
		{
			posWithQueue.y -= queue->pos.h;
			posWithQueue.h += queue->pos.h;
		}

		queue->showAll(to);
	}

	//printing border around interface
	if (screen->w != 800 || screen->h !=600)
	{
		CMessage::drawBorder(curInt->playerID,to,posWithQueue.w + 28, posWithQueue.h + 28, posWithQueue.x-14, posWithQueue.y-15);
	}
}

BattleObjectsByHex CBattleInterface::sortObjectsByHex()
{
	auto getCurrentPosition = [&](const CStack *stack) -> BattleHex
	{
		for (auto & anim : pendingAnims)
		{
			// certainly ugly workaround but fixes quite annoying bug
			// stack position will be updated only *after* movement is finished
			// before this - stack is always at its initial position. Thus we need to find
			// its current position. Which can be found only in this class
			if (CMovementAnimation *move = dynamic_cast<CMovementAnimation*>(anim.first))
			{
				if (move->stack == stack)
					return move->nextHex;
			}
		}
		return stack->getPosition();
	};

	BattleObjectsByHex sorted;

	auto stacks = curInt->cb->battleGetStacksIf([](const CStack *s)
	{
		return !s->isTurret();
	});

	// Sort creatures
	for (auto & stack : stacks)
	{
		if (creAnims.find(stack->ID) == creAnims.end()) //e.g. for summoned but not yet handled stacks
			continue;

		if (stack->initialPosition < 0) // turret shooters are handled separately
			continue;

		//FIXME: hack to ignore ghost stacks
		if ((creAnims[stack->ID]->getType() == CCreatureAnim::DEAD || creAnims[stack->ID]->getType() == CCreatureAnim::HOLDING) && stack->isGhost())
			;//ignore
		else if (!creAnims[stack->ID]->isDead())
		{
			if (!creAnims[stack->ID]->isMoving())
				sorted.hex[stack->getPosition()].alive.push_back(stack);
			else
			{
				// flying creature - just blit them over everyone else
				if (stack->hasBonusOfType(Bonus::FLYING))
					sorted.afterAll.alive.push_back(stack);
				else//try to find current location
					sorted.hex[getCurrentPosition(stack)].alive.push_back(stack);
			}
		}
		else
			sorted.hex[stack->getPosition()].dead.push_back(stack);
	}

	// Sort battle effects (spells)
	for (auto & battleEffect : battleEffects)
	{
		if (battleEffect.position.isValid())
			sorted.hex[battleEffect.position].effects.push_back(&battleEffect);
		else
			sorted.afterAll.effects.push_back(&battleEffect);
	}

	// Sort obstacles
	{
		std::map<BattleHex, std::shared_ptr<const CObstacleInstance>> backgroundObstacles;
		for (auto &obstacle : curInt->cb->battleGetAllObstacles()) {
			if (obstacle->obstacleType != CObstacleInstance::ABSOLUTE_OBSTACLE
				&& obstacle->obstacleType != CObstacleInstance::MOAT) {
				backgroundObstacles[obstacle->pos] = obstacle;
			}
		}
		for (auto &op : backgroundObstacles)
		{
			sorted.beforeAll.obstacles.push_back(op.second);
		}
	}
	// Sort wall parts
	if (siegeController)
		siegeController->sortObjectsByHex(sorted);

	return sorted;
}

void CBattleInterface::updateBattleAnimations()
{
	//handle animations
	for (auto & elem : pendingAnims)
	{
		if (!elem.first) //this animation should be deleted
			continue;

		if (!elem.second)
		{
			elem.second = elem.first->init();
		}
		if (elem.second && elem.first)
			elem.first->nextFrame();
	}

	//delete anims
	int preSize = static_cast<int>(pendingAnims.size());
	for (auto it = pendingAnims.begin(); it != pendingAnims.end(); ++it)
	{
		if (it->first == nullptr)
		{
			pendingAnims.erase(it);
			it = pendingAnims.begin();
			break;
		}
	}

	if (preSize > 0 && pendingAnims.empty())
	{
		//anims ended
		blockUI(activeStack == nullptr);

		animsAreDisplayed.setn(false);
	}
}

std::shared_ptr<IImage> CBattleInterface::getObstacleImage(const CObstacleInstance & oi)
{
	int frameIndex = (animCount+1) *25 / getAnimSpeed();
	std::shared_ptr<CAnimation> animation;

	if(oi.obstacleType == CObstacleInstance::USUAL || oi.obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
	{
		animation = obstacleAnimations[oi.uniqueID];
	}
	else if(oi.obstacleType == CObstacleInstance::SPELL_CREATED)
	{
		const SpellCreatedObstacle * spellObstacle = dynamic_cast<const SpellCreatedObstacle *>(&oi);
		if(!spellObstacle)
			return std::shared_ptr<IImage>();

		std::string animationName = spellObstacle->animation;

		auto cacheIter = animationsCache.find(animationName);

		if(cacheIter == animationsCache.end())
		{
			logAnim->trace("Creating obstacle animation %s", animationName);

			animation = std::make_shared<CAnimation>(animationName);
			animation->preload();
			animationsCache[animationName] = animation;
		}
		else
		{
			animation = cacheIter->second;
		}
	}

	if(animation)
	{
		frameIndex %= animation->size(0);
		return animation->getImage(frameIndex, 0);
	}

	return nullptr;
}

Point CBattleInterface::getObstaclePosition(std::shared_ptr<IImage> image, const CObstacleInstance & obstacle)
{
	int offset = obstacle.getAnimationYOffset(image->height());

	Rect r = hexPosition(obstacle.pos);
	r.y += 42 - image->height() + offset;

	return r.topLeft();
}

void CBattleInterface::redrawBackgroundWithHexes(const CStack *activeStack)
{
	attackableHexes.clear();
	if (activeStack)
		occupyableHexes = curInt->cb->battleGetAvailableHexes(activeStack, true, &attackableHexes);

	auto fillStackCountOutsideHexes = [&]()
	{
		auto accessibility = curInt->cb->getAccesibility();

		for(int i = 0; i < accessibility.size(); i++)
			stackCountOutsideHexes.at(i) = (accessibility[i] == EAccessibility::ACCESSIBLE);
	};

	fillStackCountOutsideHexes();

	//prepare background graphic with hexes and shaded hexes
	blitAt(background, 0, 0, backgroundWithHexes);

	//draw absolute obstacles (cliffs and so on)
	for(auto & oi : curInt->cb->battleGetAllObstacles())
	{
		if(oi->obstacleType == CObstacleInstance::ABSOLUTE_OBSTACLE)
		{
			auto img = getObstacleImage(*oi);
			if(img)
				img->draw(backgroundWithHexes, oi->getInfo().width, oi->getInfo().height);
		}
	}

	if (settings["battle"]["stackRange"].Bool())
	{
		std::vector<BattleHex> hexesToShade = occupyableHexes;
		hexesToShade.insert(hexesToShade.end(), attackableHexes.begin(), attackableHexes.end());
		for (BattleHex hex : hexesToShade)
		{
			int i = hex.getY(); //row
			int j = hex.getX()-1; //column
			int x = 58 + (i%2==0 ? 22 : 0) + 44*j;
			int y = 86 + 42 *i;
			SDL_Rect temp_rect = genRect(cellShade->h, cellShade->w, x, y);
			CSDL_Ext::blit8bppAlphaTo24bpp(cellShade, nullptr, backgroundWithHexes, &temp_rect);
		}
	}

	if(settings["battle"]["cellBorders"].Bool())
		CSDL_Ext::blit8bppAlphaTo24bpp(cellBorders, nullptr, backgroundWithHexes, nullptr);
}

