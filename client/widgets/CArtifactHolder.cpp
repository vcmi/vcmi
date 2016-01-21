#include "StdInc.h"
#include "CArtifactHolder.h"

#include "../gui/CGuiHandler.h"
#include "../gui/CCursorHandler.h"

#include "Buttons.h"
#include "CComponent.h"

#include "../windows/CHeroWindow.h"
#include "../windows/CSpellWindow.h"
#include "../windows/GUIClasses.h"
#include "../CPlayerInterface.h"
#include "../CGameInfo.h"

#include "../../CCallback.h"

#include "../../lib/CArtHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/CGeneralTextHandler.h"

#include "../../lib/mapObjects/CGHeroInstance.h"

/*
 * CArtifactHolder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CArtPlace::CArtPlace(Point position, const CArtifactInstance * Art):
	locked(false), picked(false), marked(false), ourArt(Art)
{
	pos += position;
	pos.w = pos.h = 44;
	createImage();
}

void CArtPlace::createImage()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	int graphic = 0;
	if (ourArt)
		graphic = ourArt->artType->iconIndex;
	if (locked)
		graphic = ArtifactID::ART_LOCK;

	image = new CAnimImage("artifact", graphic);
	if (!ourArt)
		image->disable();

	selection = new CAnimImage("artifact", ArtifactID::ART_SELECTION);
	selection->disable();
}

void CArtPlace::lockSlot(bool on)
{
	if (locked == on)
		return;

	locked = on;

	if (on)
		image->setFrame(ArtifactID::ART_LOCK);
	else
		image->setFrame(ourArt->artType->iconIndex);
}

void CArtPlace::pickSlot(bool on)
{
	if (picked == on)
		return;

	picked = on;
	if (on)
		image->disable();
	else
		image->enable();
}

void CArtPlace::selectSlot(bool on)
{
	if (marked == on)
		return;

	marked = on;
	if (on)
		selection->enable();
	else
		selection->disable();
}

void CArtPlace::clickLeft(tribool down, bool previousState)
{
	//LRClickableAreaWTextComp::clickLeft(down);
	bool inBackpack = slotID >= GameConstants::BACKPACK_START,
		srcInBackpack = ourOwner->commonInfo->src.slotID >= GameConstants::BACKPACK_START,
		srcInSameHero = ourOwner->commonInfo->src.AOH == ourOwner;

	if(ourOwner->highlightModeCallback && ourArt)
	{
		if(down)
		{
			if(!ourArt->artType->isTradable()) //War Machine or Spellbook
			{
				LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[21]); //This item can't be traded.
			}
			else
			{
				ourOwner->unmarkSlots(false);
				selectSlot(true);
				ourOwner->highlightModeCallback(this);
			}
		}
		return;
	}

	// If clicked on spellbook, open it only if no artifact is held at the moment.
	if(ourArt && !down && previousState && !ourOwner->commonInfo->src.AOH)
	{
		if(ourArt->artType->id == ArtifactID::SPELLBOOK)
		{
			auto   spellWindow = new CSpellWindow(genRect(595, 620, (screen->w - 620)/2, (screen->h - 595)/2), ourOwner->curHero, LOCPLINT, LOCPLINT->battleInt);
			GH.pushInt(spellWindow);
		}
	}

	if (!down && previousState)
	{
		if(ourArt && ourArt->artType->id == ArtifactID::SPELLBOOK)
			return; //this is handled separately

		if(!ourOwner->commonInfo->src.AOH) //nothing has been clicked
		{
			if(ourArt  //to prevent selecting empty slots (bugfix to what GrayFace reported)
				&&  ourOwner->curHero->tempOwner == LOCPLINT->playerID)//can't take art from another player
			{
				if(ourArt->artType->id == ArtifactID::CATAPULT) //catapult cannot be highlighted
				{
					std::vector<CComponent *> catapult(1, new CComponent(CComponent::artifact, 3, 0));
					LOCPLINT->showInfoDialog(CGI->generaltexth->allTexts[312], catapult); //The Catapult must be equipped.
					return;
				}
				select();
			}
		}
		else if(ourArt == ourOwner->commonInfo->src.art) //restore previously picked artifact
		{
			deselect();
		}
		else //perform artifact transition
		{
			if(inBackpack) // Backpack destination.
			{
				if(srcInBackpack && slotID == ourOwner->commonInfo->src.slotID + 1) //next slot (our is not visible, so visually same as "old" place) to the art -> make nothing, return artifact to slot
				{
					deselect();
				}
				else
				{
					const CArtifact * const cur = ourOwner->commonInfo->src.art->artType;

					switch(cur->id)
					{
					case ArtifactID::CATAPULT:
						//should not happen, catapult cannot be selected
						assert(cur->id != ArtifactID::CATAPULT);
						break;
					case ArtifactID::BALLISTA: case ArtifactID::AMMO_CART: case ArtifactID::FIRST_AID_TENT: //war machines cannot go to backpack
						LOCPLINT->showInfoDialog(boost::str(boost::format(CGI->generaltexth->allTexts[153]) % cur->Name()));
						break;
					default:
						setMeAsDest();
						vstd::amin(ourOwner->commonInfo->dst.slotID, ArtifactPosition(
							ourOwner->curHero->artifactsInBackpack.size() + GameConstants::BACKPACK_START));
						if(srcInBackpack && srcInSameHero)
						{
							if(!ourArt								//cannot move from backpack to AFTER backpack -> combined with vstd::amin above it will guarantee that dest is at most the last artifact
								|| ourOwner->commonInfo->src.slotID < ourOwner->commonInfo->dst.slotID) //rearranging arts in backpack after taking src artifact, the dest id will be shifted
								vstd::advance(ourOwner->commonInfo->dst.slotID, -1);
						}
						if(srcInSameHero && ourOwner->commonInfo->dst.slotID == ourOwner->commonInfo->src.slotID) //we came to src == dst
							deselect();
						else
							ourOwner->realizeCurrentTransaction();
						break;
					}
				}
			}
			//check if swap is possible
			else if (fitsHere(ourOwner->commonInfo->src.art) &&
				(!ourArt || ourOwner->curHero->tempOwner == LOCPLINT->playerID))
			{
				setMeAsDest();
//
// 				// Special case when the dest artifact can't be fit into the src slot.
// 				//CGI->arth->unequipArtifact(ourOwner->curHero->artifWorn, slotID);
// 				const CArtifactsOfHero* srcAOH = ourOwner->commonInfo->src.AOH;
// 				ui16 srcSlotID = ourOwner->commonInfo->src.slotID;
// 				if (ourArt && srcSlotID < 19 && !ourArt->canBePutAt(ArtifactLocation(srcAOH->curHero, srcSlotID)))
// 				{
// 					// Put dest artifact into owner's backpack.
// 					ourOwner->commonInfo->src.AOH = ourOwner;
// 					ourOwner->commonInfo->src.slotID = ourOwner->curHero->artifacts.size() + 19;
// 				}

				ourOwner->realizeCurrentTransaction();
			}
		}
	}
}

bool CArtPlace::askToAssemble(const CArtifactInstance *art, ArtifactPosition slot,
                              const CGHeroInstance *hero)
{
	std::vector<const CArtifact *> assemblyPossibilities = art->assemblyPossibilities(hero);

	// If the artifact can be assembled, display dialog.
	for(const CArtifact *combination : assemblyPossibilities)
	{
		LOCPLINT->showArtifactAssemblyDialog(
			art->artType->id,
			combination->id,
			true,
			std::bind(&CCallback::assembleArtifacts, LOCPLINT->cb.get(), hero, slot, true, combination->id),
			0);

		if(assemblyPossibilities.size() > 2)
		{
			logGlobal->warnStream() << boost::format(
				"More than one possibility of assembling on %s... taking only first")
				% art->artType->Name();
		}
		return true;
	}
	return false;
}

void CArtPlace::clickRight(tribool down, bool previousState)
{
	if(down && ourArt && !locked && text.size() && !picked)  //if there is no description or it's a lock, do nothing ;]
	{
		if (slotID < GameConstants::BACKPACK_START)
		{
			if(ourOwner->allowedAssembling)
			{
				std::vector<const CArtifact *> assemblyPossibilities = ourArt->assemblyPossibilities(ourOwner->curHero);

				// If the artifact can be assembled, display dialog.
				if (askToAssemble(ourArt, slotID, ourOwner->curHero))
				{
					return;
				}

				// Otherwise if the artifact can be diasassembled, display dialog.
				if(ourArt->canBeDisassembled())
				{
					LOCPLINT->showArtifactAssemblyDialog(
						ourArt->artType->id,
						0,
						false,
						std::bind(&CCallback::assembleArtifacts, LOCPLINT->cb.get(), ourOwner->curHero, slotID, false, ArtifactID()),
						0);
					return;
				}
			}
		}

		// Lastly just show the artifact description.
		LRClickableAreaWTextComp::clickRight(down, previousState);
	}
}

/**
 * Selects artifact slot so that the containing artifact looks like it's picked up.
 */
void CArtPlace::select ()
{
	if (locked)
		return;

	selectSlot(true);
	pickSlot(true);
	if(ourArt->canBeDisassembled() && slotID < GameConstants::BACKPACK_START) //worn combined artifact -> locks have to disappear
	{
		for(int i = 0; i < GameConstants::BACKPACK_START; i++)
		{
			CArtPlace * ap = ourOwner->getArtPlace(i);
			if(nullptr != ap)//getArtPlace may return null
				ap->pickSlot(ourArt->isPart(ap->ourArt));
		}
	}

	CCS->curh->dragAndDropCursor(new CAnimImage("artifact", ourArt->artType->iconIndex));
	ourOwner->commonInfo->src.setTo(this, false);
	ourOwner->markPossibleSlots(ourArt);

	if(slotID >= GameConstants::BACKPACK_START)
		ourOwner->scrollBackpack(0); //will update slots

	ourOwner->updateParentWindow();
	ourOwner->safeRedraw();
}

/**
 * Deselects the artifact slot.
 */
void CArtPlace::deselect ()
{
	pickSlot(false);
	if(ourArt && ourArt->canBeDisassembled()) //combined art returned to its slot -> restore locks
	{
		for(int i = 0; i < GameConstants::BACKPACK_START; i++)
		{
			auto place = ourOwner->getArtPlace(i);

			if(nullptr != place)//getArtPlace may return null
				place->pickSlot(false);
		}
	}

	CCS->curh->dragAndDropCursor(nullptr);
	ourOwner->unmarkSlots();
	ourOwner->commonInfo->src.clear();
	if(slotID >= GameConstants::BACKPACK_START)
		ourOwner->scrollBackpack(0); //will update slots


	ourOwner->updateParentWindow();
	ourOwner->safeRedraw();
}

void CArtPlace::showAll(SDL_Surface * to)
{
	if (ourArt && !picked && ourArt == ourOwner->curHero->getArt(slotID, false)) //last condition is needed for disassembling -> artifact may be gone, but we don't know yet TODO: real, nice solution
	{
		CIntObject::showAll(to);
	}

	if(marked && active)
	{
		// Draw vertical bars.
		for (int i = 0; i < pos.h; ++i)
		{
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x,             pos.y + i, 240, 220, 120);
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x + pos.w - 1, pos.y + i, 240, 220, 120);
		}

		// Draw horizontal bars.
		for (int i = 0; i < pos.w; ++i)
		{
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x + i, pos.y,             240, 220, 120);
			CSDL_Ext::SDL_PutPixelWithoutRefresh(to, pos.x + i, pos.y + pos.h - 1, 240, 220, 120);
		}
	}
}

bool CArtPlace::fitsHere(const CArtifactInstance * art) const
{
	// You can place 'no artifact' anywhere.
	if(!art)
		return true;

	// Anything but War Machines can be placed in backpack.
	if (slotID >= GameConstants::BACKPACK_START)
		return !CGI->arth->isBigArtifact(art->artType->id);

	return art->canBePutAt(ArtifactLocation(ourOwner->curHero, slotID), true);
}

void CArtPlace::setMeAsDest(bool backpackAsVoid /*= true*/)
{
	ourOwner->commonInfo->dst.setTo(this, backpackAsVoid);
}

void CArtPlace::setArtifact(const CArtifactInstance *art)
{
	baseType = -1; //by default we don't store any component
	ourArt = art;
	if(!art)
	{
		image->disable();
		text = std::string();
		hoverText = CGI->generaltexth->allTexts[507];
		return;
	}

	image->enable();
	image->setFrame(locked ? ArtifactID::ART_LOCK : art->artType->iconIndex);

	text = art->getEffectiveDescription(ourOwner->curHero);

	if(art->artType->id == ArtifactID::SPELL_SCROLL)
	{
		int spellID = art->getGivenSpellID();
		if(spellID >= 0)
		{
			//add spell component info (used to provide a pic in r-click popup)
			baseType = CComponent::spell;
			type = spellID;
			bonusValue = 0;
		}
	}
	else
	{
		baseType = CComponent::artifact;
		type = art->artType->id;
		bonusValue = 0;
	}

	if (locked) // Locks should appear as empty.
		hoverText = CGI->generaltexth->allTexts[507];
	else
		hoverText = boost::str(boost::format(CGI->generaltexth->heroscrn[1]) % ourArt->artType->Name());
}

void CArtifactsOfHero::SCommonPart::reset()
{
	src.clear();
	dst.clear();
	CCS->curh->dragAndDropCursor(nullptr);
}

void CArtifactsOfHero::setHero(const CGHeroInstance * hero)
{
	curHero = hero;
	if (curHero->artifactsInBackpack.size() > 0)
		backpackPos %= curHero->artifactsInBackpack.size();
	else
		backpackPos = 0;

	// Fill the slots for worn artifacts and backpack.

	for(auto p : artWorn)
	{
		setSlotData(p.second, p.first);
	}

	scrollBackpack(0);
}

void CArtifactsOfHero::dispose()
{
	CCS->curh->dragAndDropCursor(nullptr);
}

void CArtifactsOfHero::scrollBackpack(int dir)
{
	int artsInBackpack = curHero->artifactsInBackpack.size();
	backpackPos += dir;
	if(backpackPos < 0)// No guarantee of modulus behavior with negative operands -> we keep it positive
		backpackPos += artsInBackpack;

	if(artsInBackpack)
		backpackPos %= artsInBackpack;

	std::multiset<const CArtifactInstance *> toOmit = artifactsOnAltar;
	if(commonInfo->src.art) //if we picked an art from backapck, its slot has to be omitted
		toOmit.insert(commonInfo->src.art);

	int omitedSoFar = 0;

	//set new data
	size_t s = 0;
	for( ; s < artsInBackpack; ++s)
	{

		if (s < artsInBackpack)
		{
			auto slotID = ArtifactPosition(GameConstants::BACKPACK_START + (s + backpackPos)%artsInBackpack);
			const CArtifactInstance *art = curHero->getArt(slotID);
			assert(art);
			if(!vstd::contains(toOmit, art))
			{
				if(s - omitedSoFar < backpack.size())
					setSlotData(backpack[s-omitedSoFar], slotID);
			}
			else
			{
				toOmit -= art;
				omitedSoFar++;
				continue;
			}
		}
	}
	for( ; s - omitedSoFar < backpack.size(); s++)
		eraseSlotData(backpack[s-omitedSoFar], ArtifactPosition(GameConstants::BACKPACK_START + s));

	//in artifact merchant selling artifacts we may have highlight on one of backpack artifacts -> market needs update, cause artifact under highlight changed
	if(highlightModeCallback)
	{
		for(auto & elem : backpack)
		{
			if(elem->marked)
			{
				highlightModeCallback(elem);
				break;
			}
		}
	}

	//blocking scrolling if there is not enough artifacts to scroll
	bool scrollingPossible = artsInBackpack - omitedSoFar > backpack.size();
	leftArtRoll->block(!scrollingPossible);
	rightArtRoll->block(!scrollingPossible);

	safeRedraw();

}

/**
 * Marks possible slots where a given artifact can be placed, except backpack.
 *
 * @param art Artifact checked against.
 */
void CArtifactsOfHero::markPossibleSlots(const CArtifactInstance* art)
{
	for(CArtifactsOfHero *aoh : commonInfo->participants)
		for(auto p : aoh->artWorn)
			p.second->selectSlot(art->canBePutAt(ArtifactLocation(aoh->curHero, p.second->slotID), true));

	safeRedraw();
}

/**
 * Unamarks all slots.
 */
void CArtifactsOfHero::unmarkSlots(bool withRedraw /*= true*/)
{
	if(commonInfo)
		for(CArtifactsOfHero *aoh : commonInfo->participants)
			aoh->unmarkLocalSlots(false);
	else
		unmarkLocalSlots(false);\

	if(withRedraw)
		safeRedraw();
}

void CArtifactsOfHero::unmarkLocalSlots(bool withRedraw /*= true*/)
{
	for(auto p : artWorn)
		p.second->selectSlot(false);
	for(CArtPlace *place : backpack)
		place->selectSlot(false);

	if(withRedraw)
		safeRedraw();
}

/**
 * Assigns an artifacts to an artifact place depending on it's new slot ID.
 */
void CArtifactsOfHero::setSlotData(CArtPlace* artPlace, ArtifactPosition slotID)
{
	if(!artPlace && slotID >= GameConstants::BACKPACK_START) //spurious call from artifactMoved in attempt to update hidden backpack slot
	{
		return;
	}

	artPlace->pickSlot(false);
	artPlace->slotID = slotID;

	if(const ArtSlotInfo *asi = curHero->getSlot(slotID))
	{
		artPlace->lockSlot(asi->locked);
		artPlace->setArtifact(asi->artifact);
	}
	else
		artPlace->setArtifact(nullptr);
}

/**
 * Makes given artifact slot appear as empty with a certain slot ID.
 */
void CArtifactsOfHero::eraseSlotData (CArtPlace* artPlace, ArtifactPosition slotID)
{
	artPlace->pickSlot(false);
	artPlace->slotID = slotID;
	artPlace->setArtifact(nullptr);
}

CArtifactsOfHero::CArtifactsOfHero(std::map<ArtifactPosition, CArtPlace *> ArtWorn, std::vector<CArtPlace *> Backpack,
	CButton *leftScroll, CButton *rightScroll, bool createCommonPart):

	curHero(nullptr),
	artWorn(ArtWorn), backpack(Backpack),
	backpackPos(0), commonInfo(nullptr), updateState(false),
	leftArtRoll(leftScroll), rightArtRoll(rightScroll),
	allowedAssembling(true), highlightModeCallback(nullptr)
{
	if(createCommonPart)
	{
		commonInfo = new CArtifactsOfHero::SCommonPart;
		commonInfo->participants.insert(this);
	}

	// Init slots for worn artifacts.
	for (auto p : artWorn)
	{
		p.second->ourOwner = this;
		eraseSlotData(p.second, p.first);
	}

	// Init slots for the backpack.
	for(size_t s=0; s<backpack.size(); ++s)
	{
		backpack[s]->ourOwner = this;
		eraseSlotData(backpack[s], ArtifactPosition(GameConstants::BACKPACK_START + s));
	}

	leftArtRoll->addCallback(std::bind(&CArtifactsOfHero::scrollBackpack,this,-1));
	rightArtRoll->addCallback(std::bind(&CArtifactsOfHero::scrollBackpack,this,+1));
}

CArtifactsOfHero::CArtifactsOfHero(const Point& position, bool createCommonPart /*= false*/)
 : curHero(nullptr), backpackPos(0), commonInfo(nullptr), updateState(false), allowedAssembling(true), highlightModeCallback(nullptr)
{
	if(createCommonPart)
	{
		commonInfo = new CArtifactsOfHero::SCommonPart;
		commonInfo->participants.insert(this);
	}

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos += position;

	std::vector<Point> slotPos =
	{
		Point(509,30),  Point(567,240), Point(509,80),  //0-2
		Point(383,68),  Point(564,183), Point(509,130), //3-5
		Point(431,68),  Point(610,183), Point(515,295), //6-8
		Point(383,143), Point(399,194), Point(415,245), //9-11
		Point(431,296), Point(564,30),  Point(610,30), //12-14
		Point(610,76),  Point(610,122), Point(610,310), //15-17
		Point(381,296) //18
	};

	// Create slots for worn artifacts.
	for (size_t g = 0; g < GameConstants::BACKPACK_START ; g++)
	{
		artWorn[ArtifactPosition(g)] = new CArtPlace(slotPos[g]);
		artWorn[ArtifactPosition(g)]->ourOwner = this;
		eraseSlotData(artWorn[ArtifactPosition(g)], ArtifactPosition(g));
	}

	// Create slots for the backpack.
	for(size_t s=0; s<5; ++s)
	{
		auto add = new CArtPlace(Point(403 + 46 * s, 365));

		add->ourOwner = this;
		eraseSlotData(add, ArtifactPosition(GameConstants::BACKPACK_START + s));

		backpack.push_back(add);
	}

	leftArtRoll =  new CButton(Point(379, 364), "hsbtns3.def", CButton::tooltip(), [&]{ scrollBackpack(-1);}, SDLK_LEFT);
	rightArtRoll = new CButton(Point(632, 364), "hsbtns5.def", CButton::tooltip(), [&]{ scrollBackpack(+1);}, SDLK_RIGHT);
}

CArtifactsOfHero::~CArtifactsOfHero()
{
	dispose();
}

void CArtifactsOfHero::updateParentWindow()
{
	if (CHeroWindow* chw = dynamic_cast<CHeroWindow*>(GH.topInt()))
	{
		if(updateState)
			chw->curHero = curHero;
		else
			chw->update(curHero, true);
	}
	else if(CExchangeWindow* cew = dynamic_cast<CExchangeWindow*>(GH.topInt()))
	{

		//use our copy of hero to draw window
		if(cew->heroInst[0]->id == curHero->id)
			cew->heroInst[0] = curHero;
		else
			cew->heroInst[1] = curHero;

		if(!updateState)
		{
			cew->deactivate();
// 			for(int g=0; g<ARRAY_COUNT(cew->heroInst); ++g)
// 			{
// 				if(cew->heroInst[g] == curHero)
// 				{
// 					cew->artifs[g]->setHero(curHero);
// 				}
// 			}


			cew->prepareBackground();
			cew->redraw();
			cew->activate();
		}
	}
}

void CArtifactsOfHero::safeRedraw()
{
	if (active)
	{
		if(parent)
			parent->redraw();
		else
			redraw();
	}
}

void CArtifactsOfHero::realizeCurrentTransaction()
{
	assert(commonInfo->src.AOH);
	assert(commonInfo->dst.AOH);
	LOCPLINT->cb->swapArtifacts(ArtifactLocation(commonInfo->src.AOH->curHero, commonInfo->src.slotID),
								ArtifactLocation(commonInfo->dst.AOH->curHero, commonInfo->dst.slotID));
}

void CArtifactsOfHero::artifactMoved(const ArtifactLocation &src, const ArtifactLocation &dst)
{
	bool isCurHeroSrc = src.isHolder(curHero),
		isCurHeroDst = dst.isHolder(curHero);
	if(isCurHeroSrc && src.slot >= GameConstants::BACKPACK_START)
		updateSlot(src.slot);
	if(isCurHeroDst && dst.slot >= GameConstants::BACKPACK_START)
		updateSlot(dst.slot);
	if(isCurHeroSrc  ||  isCurHeroDst) //we need to update all slots, artifact might be combined and affect more slots
		updateWornSlots(false);

	if (!src.isHolder(curHero) && !isCurHeroDst)
		return;

	if(commonInfo->src == src) //artifact was taken from us
	{
		assert(commonInfo->dst == dst  //expected movement from slot ot slot
			||  dst.slot == dst.getHolderArtSet()->artifactsInBackpack.size() + GameConstants::BACKPACK_START //artifact moved back to backpack (eg. to make place for art we are moving)
			|| dst.getHolderArtSet()->bearerType() != ArtBearer::HERO);
		commonInfo->reset();
		unmarkSlots();
	}
	else if(commonInfo->dst == src) //the dest artifact was moved -> we are picking it
	{
		assert(dst.slot >= GameConstants::BACKPACK_START);
		commonInfo->reset();

		CArtPlace *ap = nullptr;
		for(CArtifactsOfHero *aoh : commonInfo->participants)
		{
			if(dst.isHolder(aoh->curHero))
			{
				commonInfo->src.AOH = aoh;
				if((ap = aoh->getArtPlace(dst.slot)))//getArtPlace may return null
					break;
			}
		}

		if(ap)
		{
			ap->select();
		}
		else
		{
			commonInfo->src.art = dst.getArt();
			commonInfo->src.slotID = dst.slot;
			assert(commonInfo->src.AOH);
			CCS->curh->dragAndDropCursor(new CAnimImage("artifact", dst.getArt()->artType->iconIndex));
			markPossibleSlots(dst.getArt());
		}
	}
	else if(src.slot >= GameConstants::BACKPACK_START &&
	        src.slot <  commonInfo->src.slotID &&
			    src.isHolder(commonInfo->src.AOH->curHero)) //artifact taken from before currently picked one
	{
		//int fixedSlot = src.hero->getArtPos(commonInfo->src.art);
		vstd::advance(commonInfo->src.slotID, -1);
		assert(commonInfo->src.valid());
	}
	else
	{
		//when moving one artifact onto another it leads to two art movements: dst->backapck; src->dst
		// however after first movement we pick the art from backpack and the second movement coming when
		// we have a different artifact may look surprising... but it's valid.
	}

	updateParentWindow();
	int shift = 0;
// 	if(dst.slot >= Arts::BACKPACK_START && dst.slot - Arts::BACKPACK_START < backpackPos)
// 		shift++;
//
	if(src.slot < GameConstants::BACKPACK_START  &&  dst.slot - GameConstants::BACKPACK_START < backpackPos)
		shift++;
	if(dst.slot < GameConstants::BACKPACK_START  &&  src.slot - GameConstants::BACKPACK_START < backpackPos)
		shift--;

	if( (isCurHeroSrc && src.slot >= GameConstants::BACKPACK_START)
	 || (isCurHeroDst && dst.slot >= GameConstants::BACKPACK_START) )
		scrollBackpack(shift); //update backpack slots
}

void CArtifactsOfHero::artifactRemoved(const ArtifactLocation &al)
{
	if(al.isHolder(curHero))
	{
		if(al.slot < GameConstants::BACKPACK_START)
			updateWornSlots(0);
		else
			scrollBackpack(0); //update backpack slots
	}
}

CArtPlace * CArtifactsOfHero::getArtPlace(int slot)
{
	if(slot < GameConstants::BACKPACK_START)
	{
		if(artWorn.find(ArtifactPosition(slot)) == artWorn.end())
		{
			logGlobal->errorStream() << "CArtifactsOfHero::getArtPlace: invalid slot " << slot;
			return nullptr;
		}

		return artWorn[ArtifactPosition(slot)];
	}
	else
	{
		for(CArtPlace *ap : backpack)
			if(ap->slotID == slot)
				return ap;
		return nullptr;
	}
}

void CArtifactsOfHero::artifactAssembled(const ArtifactLocation &al)
{
	if(al.isHolder(curHero))
		updateWornSlots();
}

void CArtifactsOfHero::artifactDisassembled(const ArtifactLocation &al)
{
	if(al.isHolder(curHero))
		updateWornSlots();
}

void CArtifactsOfHero::updateWornSlots(bool redrawParent /*= true*/)
{
	for(auto p : artWorn)
		updateSlot(p.first);

	if(redrawParent)
		updateParentWindow();
}

const CGHeroInstance * CArtifactsOfHero::getHero() const
{
	return curHero;
}

void CArtifactsOfHero::updateSlot(ArtifactPosition slotID)
{
	setSlotData(getArtPlace(slotID), slotID);
}

CArtifactHolder::CArtifactHolder()
{
}

void CWindowWithArtifacts::artifactRemoved(const ArtifactLocation &artLoc)
{
	for(CArtifactsOfHero *aoh : artSets)
		aoh->artifactRemoved(artLoc);
}

void CWindowWithArtifacts::artifactMoved(const ArtifactLocation &artLoc, const ArtifactLocation &destLoc)
{
	CArtifactsOfHero *destaoh = nullptr;
	for(CArtifactsOfHero *aoh : artSets)
	{
		aoh->artifactMoved(artLoc, destLoc);
		aoh->redraw();
		if(destLoc.isHolder(aoh->getHero()))
			destaoh = aoh;
	}

	//Make sure the status bar is updated so it does not display old text
	if(destaoh != nullptr && destaoh->getArtPlace(destLoc.slot) != nullptr)
	{
		destaoh->getArtPlace(destLoc.slot)->hover(true);
	}
}

void CWindowWithArtifacts::artifactDisassembled(const ArtifactLocation &artLoc)
{
	for(CArtifactsOfHero *aoh : artSets)
		aoh->artifactDisassembled(artLoc);
}

void CWindowWithArtifacts::artifactAssembled(const ArtifactLocation &artLoc)
{
	for(CArtifactsOfHero *aoh : artSets)
		aoh->artifactAssembled(artLoc);
}

void CArtifactsOfHero::SCommonPart::Artpos::clear()
{
	slotID = ArtifactPosition::PRE_FIRST;
	AOH = nullptr;
	art = nullptr;
}

CArtifactsOfHero::SCommonPart::Artpos::Artpos()
{
	clear();
}

void CArtifactsOfHero::SCommonPart::Artpos::setTo(const CArtPlace *place, bool dontTakeBackpack)
{
	slotID = place->slotID;
	AOH = place->ourOwner;

	if(slotID >= 19 && dontTakeBackpack)
		art = nullptr;
	else
		art = place->ourArt;
}

bool CArtifactsOfHero::SCommonPart::Artpos::operator==(const ArtifactLocation &al) const
{
	if(!AOH)
		return false;
	bool ret = al.isHolder(AOH->curHero)  &&  al.slot == slotID;

	//assert(al.getArt() == art);
	return ret;
}

bool CArtifactsOfHero::SCommonPart::Artpos::valid()
{
	assert(AOH && art);
	return art == AOH->curHero->getArt(slotID);
}
