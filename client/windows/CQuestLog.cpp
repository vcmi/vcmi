/*
 * CQuestLog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CQuestLog.h"

#include "../CPlayerInterface.h"

#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../CPlayerInterface.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/Slider.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../adventureMap/CMinimap.h"
#include "../render/Canvas.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/gameState/QuestInfo.h"
#include "../../lib/mapObjects/CQuest.h"
#include "../../lib/texts/CGeneralTextHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

struct QuestInfo;

VCMI_LIB_NAMESPACE_END

class CAdvmapInterface;

void CQuestLabel::clickPressed(const Point & cursorPosition)
{
	callback();
}

void CQuestLabel::showAll(Canvas & to)
{
	CMultiLineLabel::showAll (to);
}

CQuestIcon::CQuestIcon (const AnimationPath &defname, int index, int x, int y) :
	CAnimImage(defname, index, 0, x, y)
{
	addUsedEvents(LCLICK);
}

void CQuestIcon::clickPressed(const Point & cursorPosition)
{
	callback();
}

void CQuestIcon::showAll(Canvas & to)
{
	CanvasClipRectGuard guard(to, parent->pos);
	CAnimImage::showAll(to);
}

CQuestMinimap::CQuestMinimap(const Rect & position)
	: CMinimap(position),
	currentQuest(nullptr)
{
}

void CQuestMinimap::addQuestMarks (const QuestInfo * q)
{
	OBJECT_CONSTRUCTION;
	icons.clear();

	int3 tile = q->getPosition(GAME->interface()->cb.get());

	Point offset = tileToPixels(tile);

	onMapViewMoved(Rect(), tile.z);

	auto pic = std::make_shared<CQuestIcon>(AnimationPath::builtin("VwSymbol.def"), 3, offset.x, offset.y);

	pic->moveBy (Point ( -pic->pos.w/2, -pic->pos.h/2));
	pic->callback = std::bind (&CQuestMinimap::iconClicked, this);

	icons.push_back(pic);
}

void CQuestMinimap::update()
{
	CMinimap::update();
	if(currentQuest)
		addQuestMarks(currentQuest);
}

void CQuestMinimap::iconClicked()
{
	if(currentQuest->obj.hasValue())
		adventureInt->centerOnTile(currentQuest->getObject(GAME->interface()->cb.get())->visitablePos());
	//moveAdvMapSelection();
}

void CQuestMinimap::showAll(Canvas & to)
{
	CIntObject::showAll(to); // blitting IntObject directly to hide radar
//	for (auto pic : icons)
//		pic->showAll(to);
}

CQuestLog::CQuestLog (const std::vector<QuestInfo> & Quests)
	: CWindowObject(PLAYER_COLORED | BORDERED, ImagePath::builtin("questDialog")),
	questIndex(0),
	currentQuest(nullptr),
	hideComplete(false),
	quests(Quests)
{
	OBJECT_CONSTRUCTION;

	minimap = std::make_shared<CQuestMinimap>(Rect(12, 12, 169, 169));
	// TextBox have it's own 4 pixel padding from top at least for English. To achieve 10px from both left and top only add 6px margin
	description = std::make_shared<CTextBox>("", Rect(205, 18, 385, DESCRIPTION_HEIGHT_MAX), CSlider::BROWN, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE);
	ok = std::make_shared<CButton>(Point(539, 398), AnimationPath::builtin("IOKAY.DEF"), LIBRARY->generaltexth->zelp[445], std::bind(&CQuestLog::close, this), EShortcut::GLOBAL_RETURN);
	// Both button and label are shifted to -2px by x and y to not make them actually look like they're on same line with quests list and ok button
	hideCompleteButton = std::make_shared<CToggleButton>(Point(10, 396), AnimationPath::builtin("sysopchk.def"), CButton::tooltipLocalized("vcmi.questLog.hideComplete"), std::bind(&CQuestLog::toggleComplete, this, _1));
	hideCompleteLabel = std::make_shared<CLabel>(46, 398, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.questLog.hideComplete.hover"));
	slider = std::make_shared<CSlider>(Point(166, 195), 191, std::bind(&CQuestLog::sliderMoved, this, _1), QUEST_COUNT, 0, 0, Orientation::VERTICAL, CSlider::BROWN);
	slider->setPanningStep(32);

	recreateLabelList();
	recreateQuestList(0);
}

void CQuestLog::recreateLabelList()
{
	OBJECT_CONSTRUCTION;
	labels.clear();

	bool completeMissing = true;
	int currentLabel = 0;
	for (int i = 0; i < quests.size(); ++i)
	{
		auto questPtr = quests[i].getQuest(GAME->interface()->cb.get());
		auto questObject = quests[i].getObject(GAME->interface()->cb.get());

		// Quests without mision don't have text for them and can't be displayed
		if (quests[i].getQuest(GAME->interface()->cb.get())->mission == Rewardable::Limiter{})
			continue;

		if (questPtr->isCompleted)
		{
			completeMissing = false;
			if (hideComplete)
				continue;
		}

		MetaString text;
		questPtr->getRolloverText(GAME->interface()->cb.get(), text, false);
		if (quests[i].obj.hasValue())
		{
			if (auto seersHut = dynamic_cast<const CGSeerHut *>(questObject))
			{
				MetaString toSeer;
				toSeer.appendRawString(LIBRARY->generaltexth->allTexts[347]);
				toSeer.replaceRawString(seersHut->seerName);
				text.replaceRawString(toSeer.toString());
			}
			else
				text.replaceRawString(questObject->getObjectName()); //get name of the object
		}
		auto label = std::make_shared<CQuestLabel>(Rect(13, 195, 149,31), FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, text.toString());
		label->disable();

		label->callback = std::bind(&CQuestLog::selectQuest, this, i, currentLabel);
		labels.push_back(label);

		// Select latest active quest
		if(!questPtr->isCompleted)
			selectQuest(i, currentLabel);

		currentLabel = static_cast<int>(labels.size());
	}

	if (completeMissing) // We can't use block(completeMissing) because if false button state reset to NORMAL
		hideCompleteButton->block(true);

	slider->setAmount(currentLabel);
	if (currentLabel > QUEST_COUNT)
	{
		slider->block(false);
		slider->scrollToMax();
	}
	else
	{
		slider->block(true);
		slider->scrollToMin();
	}
}

void CQuestLog::showAll(Canvas & to)
{
	CWindowObject::showAll(to);
	if(questIndex >= 0 && questIndex < labels.size())
	{
		//TODO: use child object to selection rect
		Rect rect = Rect::createAround(labels[questIndex]->pos, 1);
		rect.x -= 2; // Adjustment needed as we want selection box on top of border in graphics
		rect.w += 2;
		to.drawBorder(rect, Colors::METALLIC_GOLD);
	}
}

void CQuestLog::recreateQuestList (int newpos)
{
	for (int i = 0; i < labels.size(); ++i)
	{
		labels[i]->pos = Rect (pos.x + 14, pos.y + 195 + (i-newpos) * 32, 151, 31);
		if (i >= newpos && i < newpos + QUEST_COUNT)
			labels[i]->enable();
		else
			labels[i]->disable();
	}
	minimap->update();
}

void CQuestLog::selectQuest(int which, int labelId)
{
	questIndex = labelId;
	currentQuest = &quests[which];
	minimap->currentQuest = currentQuest;

	MetaString text;
	std::vector<Component> components;
	currentQuest->getQuest(GAME->interface()->cb.get())->getVisitText(GAME->interface()->cb.get(), text, components, true);
	if(description->slider)
		description->slider->scrollToMin(); // scroll text to start position
	description->setText(text.toString()); //TODO: use special log entry text

	componentsBox.reset();

	int componentsSize = static_cast<int>(components.size());
	int descriptionHeight = DESCRIPTION_HEIGHT_MAX;
	if(componentsSize)
	{
		CComponent::ESize imageSize = CComponent::large;
		if (componentsSize > 4)
		{
			imageSize = CComponent::small; // Only small icons can be used for resources as 4+ icons take too much space
			descriptionHeight -= 155;
		}
		else
			descriptionHeight -= 130;
		/*switch (currentQuest->quest->missionType)
		{
			case CQuest::MISSION_ARMY:
			{
				if (componentsSize > 4)
					descriptionHeight -= 195;
				else
					descriptionHeight -= 100;

				break;
			}
			case CQuest::MISSION_ART:
			{
				if (componentsSize > 4)
					descriptionHeight -= 190;
				else
					descriptionHeight -= 90;

				break;
			}
			case CQuest::MISSION_PRIMARY_STAT:
			case CQuest::MISSION_RESOURCES:
			{
				if (componentsSize > 4)
				{
					imageSize = CComponent::small; // Only small icons can be used for resources as 4+ icons take too much space
					descriptionHeight -= 140;
				}
				else
					descriptionHeight -= 125;

				break;
			}
			default:
				descriptionHeight -= 115;
				break;
		}*/

		OBJECT_CONSTRUCTION;

		std::vector<std::shared_ptr<CComponent>> comps;
		for(auto & component : components)
		{
			auto c = std::make_shared<CComponent>(component, imageSize);
			comps.push_back(c);
		}

		componentsBox = std::make_shared<CComponentBox>(comps, Rect(202, 20+descriptionHeight+15, 391, DESCRIPTION_HEIGHT_MAX-(20+descriptionHeight)));
	}
	description->resize(Point(385, descriptionHeight));

	minimap->update();
	redraw();
}

void CQuestLog::sliderMoved(int newpos)
{
	recreateQuestList(newpos); //move components
	redraw();
}

void CQuestLog::toggleComplete(bool on)
{
	hideComplete = on;
	recreateLabelList();
	recreateQuestList(0);
	redraw();
}
