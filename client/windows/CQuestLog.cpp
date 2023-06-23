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

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"

#include "../gui/CGuiHandler.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/Slider.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../adventureMap/CMinimap.h"
#include "../render/Canvas.h"
#include "../renderSDL/SDL_Extensions.h"

#include "../../CCallback.h"
#include "../../lib/CArtHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/gameState/QuestInfo.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/MetaString.h"
#include "../../lib/mapObjects/CQuest.h"

VCMI_LIB_NAMESPACE_BEGIN

struct QuestInfo;

VCMI_LIB_NAMESPACE_END

class CAdvmapInterface;

void CQuestLabel::clickLeft(tribool down, bool previousState)
{
	if (down)
		callback();
}

void CQuestLabel::showAll(Canvas & to)
{
	CMultiLineLabel::showAll (to);
}

CQuestIcon::CQuestIcon (const std::string &defname, int index, int x, int y) :
	CAnimImage(defname, index, 0, x, y)
{
	addUsedEvents(LCLICK);
}

void CQuestIcon::clickLeft(tribool down, bool previousState)
{
	if (down)
		callback();
}

void CQuestIcon::showAll(Canvas & to)
{
	CSDL_Ext::CClipRectGuard guard(to.getInternalSurface(), parent->pos);
	CAnimImage::showAll(to);
}

CQuestMinimap::CQuestMinimap(const Rect & position)
	: CMinimap(position),
	currentQuest(nullptr)
{
}

void CQuestMinimap::addQuestMarks (const QuestInfo * q)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	icons.clear();

	int3 tile;
	if (q->obj)
		tile = q->obj->pos;
	else
		tile = q->tile;

	Point offset = tileToPixels(tile);

	onMapViewMoved(Rect(), tile.z);

	auto pic = std::make_shared<CQuestIcon>("VwSymbol.def", 3, offset.x, offset.y);

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
	if(currentQuest->obj)
		adventureInt->centerOnTile(currentQuest->obj->pos);
	//moveAdvMapSelection();
}

void CQuestMinimap::showAll(Canvas & to)
{
	CIntObject::showAll(to); // blitting IntObject directly to hide radar
//	for (auto pic : icons)
//		pic->showAll(to);
}

CQuestLog::CQuestLog (const std::vector<QuestInfo> & Quests)
	: CWindowObject(PLAYER_COLORED | BORDERED, "questDialog"),
	questIndex(0),
	currentQuest(nullptr),
	hideComplete(false),
	quests(Quests)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	minimap = std::make_shared<CQuestMinimap>(Rect(12, 12, 169, 169));
	// TextBox have it's own 4 pixel padding from top at least for English. To achieve 10px from both left and top only add 6px margin
	description = std::make_shared<CTextBox>("", Rect(205, 18, 385, DESCRIPTION_HEIGHT_MAX), CSlider::BROWN, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE);
	ok = std::make_shared<CButton>(Point(539, 398), "IOKAY.DEF", CGI->generaltexth->zelp[445], std::bind(&CQuestLog::close, this), EShortcut::GLOBAL_ACCEPT);
	// Both button and lable are shifted to -2px by x and y to not make them actually look like they're on same line with quests list and ok button
	hideCompleteButton = std::make_shared<CToggleButton>(Point(10, 396), "sysopchk.def", CButton::tooltipLocalized("vcmi.questLog.hideComplete"), std::bind(&CQuestLog::toggleComplete, this, _1));
	hideCompleteLabel = std::make_shared<CLabel>(46, 398, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE, CGI->generaltexth->translate("vcmi.questLog.hideComplete.hover"));
	slider = std::make_shared<CSlider>(Point(166, 195), 191, std::bind(&CQuestLog::sliderMoved, this, _1), QUEST_COUNT, 0, 0, Orientation::VERTICAL, CSlider::BROWN);
	slider->setPanningStep(32);

	recreateLabelList();
	recreateQuestList(0);
}

void CQuestLog::recreateLabelList()
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	labels.clear();

	bool completeMissing = true;
	int currentLabel = 0;
	for (int i = 0; i < quests.size(); ++i)
	{
		// Quests with MISSION_NONE type don't have text for them and can't be displayed
		if (quests[i].quest->missionType == CQuest::MISSION_NONE)
			continue;

		if (quests[i].quest->progress == CQuest::COMPLETE)
		{
			completeMissing = false;
			if (hideComplete)
				continue;
		}

		MetaString text;
		quests[i].quest->getRolloverText (text, false);
		if (quests[i].obj)
		{
			if (auto seersHut = dynamic_cast<const CGSeerHut *>(quests[i].obj))
			{
				MetaString toSeer;
				toSeer.appendRawString(VLC->generaltexth->allTexts[347]);
				toSeer.replaceRawString(seersHut->seerName);
				text.replaceRawString(toSeer.toString());
			}
			else
				text.replaceRawString(quests[i].obj->getObjectName()); //get name of the object
		}
		auto label = std::make_shared<CQuestLabel>(Rect(13, 195, 149,31), FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, text.toString());
		label->disable();

		label->callback = std::bind(&CQuestLog::selectQuest, this, i, currentLabel);
		labels.push_back(label);

		// Select latest active quest
		if (quests[i].quest->progress != CQuest::COMPLETE)
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
	currentQuest->quest->getVisitText (text, components, currentQuest->quest->isCustomFirst, true);
	if(description->slider)
		description->slider->scrollToMin(); // scroll text to start position
	description->setText(text.toString()); //TODO: use special log entry text

	componentsBox.reset();

	int componentsSize = static_cast<int>(components.size());
	int descriptionHeight = DESCRIPTION_HEIGHT_MAX;
	if(componentsSize)
	{
		descriptionHeight -= 15;
		CComponent::ESize imageSize = CComponent::large;
		switch (currentQuest->quest->missionType)
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
		}

		OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);

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
