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

#include "../../lib/CConfigHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/gameState/ScenarioEventJournalEntry.h"
#include "../../lib/gameState/QuestInfo.h"
#include "../../lib/mapObjects/Quest.h"
#include "../../lib/texts/CGeneralTextHandler.h"

struct QuestInfo;

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

void CQuestMinimap::setQuest(const QuestInfo * q)
{
	currentQuest = q;
	markerTiles = q ? q->getMarkerTiles(GAME->interface()->cb.get()) : std::vector<int3>{};
	update();
}

void CQuestMinimap::placeMarks()
{
	OBJECT_CONSTRUCTION;
	icons.clear();

	if(markerTiles.empty())
		return;

	// The minimap shows a single level; follow the source object's level and draw
	// only the markers on it.
	const int level = markerTiles.front().z;
	onMapViewMoved(Rect(), level);

	for(const int3 & tile : markerTiles)
	{
		if(tile.z != level)
			continue;

		Point offset = tileToPixels(tile);
		auto pic = std::make_shared<CQuestIcon>(AnimationPath::builtin("VwSymbol.def"), 3, offset.x, offset.y);
		pic->moveBy(Point(-pic->pos.w/2, -pic->pos.h/2));
		pic->callback = std::bind(&CQuestMinimap::iconClicked, this);
		icons.push_back(pic);
	}
}

void CQuestMinimap::update()
{
	CMinimap::update();
	placeMarks();
}

void CQuestMinimap::iconClicked()
{
	if(currentQuest->hasObjectInstance())
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
	quests(Quests)
{
	OBJECT_CONSTRUCTION;

	minimap = std::make_shared<CQuestMinimap>(Rect(12, 12, 169, 169));
	// TextBox have it's own 4 pixel padding from top at least for English. To achieve 10px from both left and top only add 6px margin
	description = std::make_shared<CTextBox>("", Rect(205, DESCRIPTION_TOP, 385, DESCRIPTION_HEIGHT_MAX), CSlider::BROWN, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE);
	ok = std::make_shared<CButton>(Point(539, 398), AnimationPath::builtin("IOKAY.DEF"), LIBRARY->generaltexth->zelp[445], std::bind(&CQuestLog::close, this), EShortcut::GLOBAL_RETURN);
	auto questsTab = std::make_shared<CToggleButton>(Point(193, 18), AnimationPath::builtin("settingsWindow/button190"), CButton::tooltip(), nullptr);
	auto eventsTab = std::make_shared<CToggleButton>(Point(411, 18), AnimationPath::builtin("settingsWindow/button190"), CButton::tooltip(), nullptr);
	questsTab->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.adventureMap.journal.quests"), FONT_SMALL, Colors::YELLOW);
	eventsTab->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.adventureMap.journal.events"), FONT_SMALL, Colors::YELLOW);
	eventsTab->block(GAME->interface()->cb->getMyScenarioEventJournal().empty());
	journalTabs = std::make_shared<CToggleGroup>([this](int tab)
	{
		if(tab == 1)
		{
			close();
			GAME->interface()->showScenarioEventJournal();
		}
	});
	journalTabs->addToggle(0, questsTab);
	journalTabs->addToggle(1, eventsTab);
	journalTabs->setSelected(0);
	slider = std::make_shared<CSlider>(Point(166, 195), 191, std::bind(&CQuestLog::sliderMoved, this, _1), QUEST_COUNT, 0, 0, Orientation::VERTICAL, CSlider::BROWN);
	slider->setPanningStep(32);

	recreateLabelList();
	recreateQuestList(0);
}

void CQuestLog::recreateLabelList()
{
	OBJECT_CONSTRUCTION;
	labels.clear();

	int currentLabel = 0;
	for (int i = 0; i < quests.size(); ++i)
	{
		auto questPtr = quests[i].getQuest(GAME->interface()->cb.get());
		auto questObject = quests[i].getObject(GAME->interface()->cb.get());

		// Quests without mision don't have text for them and can't be displayed
		if (quests[i].getQuest(GAME->interface()->cb.get())->mission == Rewardable::Limiter{})
			continue;

		MetaString text;
		questPtr->getQuestlogText(GAME->interface()->cb.get(), text, false);
		if (quests[i].hasObjectInstance())
		{
			const auto * source = questObject ? questObject->asQuestSource() : nullptr;
			std::string giver = source ? source->getQuestGiverName() : "";
			if (!giver.empty())
			{
				MetaString toSeer;
				toSeer.appendRawString(LIBRARY->generaltexth->allTexts[347]);
				toSeer.replaceRawString(giver);
				text.replaceRawString(toSeer.toString());
			}
			else if(questObject)
				text.replaceRawString(questObject->getObjectName()); //get name of the object
		}
		auto label = std::make_shared<CQuestLabel>(Rect(13, 195, 149,31), FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, text.toString());
		label->disable();

		label->callback = std::bind(&CQuestLog::selectQuest, this, i, currentLabel);
		labels.push_back(label);

		// Select latest quest
		selectQuest(i, currentLabel);

		currentLabel = static_cast<int>(labels.size());
	}

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
	minimap->setQuest(currentQuest);

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
			case Quest::MISSION_ARMY:
			{
				if (componentsSize > 4)
					descriptionHeight -= 195;
				else
					descriptionHeight -= 100;

				break;
			}
			case Quest::MISSION_ART:
			{
				if (componentsSize > 4)
					descriptionHeight -= 190;
				else
					descriptionHeight -= 90;

				break;
			}
			case Quest::MISSION_PRIMARY_STAT:
			case Quest::MISSION_RESOURCES:
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

		componentsBox = std::make_shared<CComponentBox>(comps, Rect(202, DESCRIPTION_TOP + descriptionHeight + 15, 391, 115));
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
