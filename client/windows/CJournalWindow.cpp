/*
 * CJournalWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CJournalWindow.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/Slider.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/gameState/QuestInfo.h"
#include "../../lib/gameState/ScenarioEventJournalEntry.h"
#include "../../lib/texts/CGeneralTextHandler.h"

JournalLabel::JournalLabel(const Rect & position, const std::string & text)
	: CMultiLineLabel(position, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, text)
{
}

void JournalLabel::clickPressed(const Point & cursorPosition)
{
	callback();
}

void JournalLabel::showAll(Canvas & to)
{
	CMultiLineLabel::showAll(to);
}

JournalWindow::JournalWindow(EJournalMode mode)
	: CWindowObject(PLAYER_COLORED | BORDERED, ImagePath::builtin("questDialog"))
	, mode(mode)
{
	OBJECT_CONSTRUCTION;

	description = std::make_shared<CTextBox>("", Rect(205, DESCRIPTION_TOP, 385, DESCRIPTION_HEIGHT), CSlider::BROWN, FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::WHITE);
	ok = std::make_shared<CButton>(Point(539, 398), AnimationPath::builtin("IOKAY.DEF"), LIBRARY->generaltexth->zelp[445], std::bind(&JournalWindow::close, this), EShortcut::GLOBAL_RETURN);
	auto questsTab = std::make_shared<CToggleButton>(Point(193, 18), AnimationPath::builtin("settingsWindow/button190"), CButton::tooltip(), nullptr);
	auto eventsTab = std::make_shared<CToggleButton>(Point(411, 18), AnimationPath::builtin("settingsWindow/button190"), CButton::tooltip(), nullptr);
	questsTab->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.adventureMap.journal.quests"), FONT_SMALL, Colors::YELLOW);
	eventsTab->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.adventureMap.journal.events"), FONT_SMALL, Colors::YELLOW);

	questsTab->block(!GAME->interface()->hasDisplayableQuests());
	eventsTab->block(!GAME->interface()->hasScenarioEventJournalEntries());

	journalTabs = std::make_shared<CToggleGroup>([this](int selectedMode)
	{
		if(selectedMode == static_cast<int>(this->mode))
			return;

		close();
		if(selectedMode == static_cast<int>(EJournalMode::QUESTS))
			GAME->interface()->showQuestLog();
		else
			GAME->interface()->showScenarioEventJournal();
	});
	journalTabs->addToggle(static_cast<int>(EJournalMode::QUESTS), questsTab);
	journalTabs->addToggle(static_cast<int>(EJournalMode::EVENTS), eventsTab);
	journalTabs->setSelected(static_cast<int>(mode));

	slider = std::make_shared<CSlider>(Point(166, 195), 191, std::bind(&JournalWindow::sliderMoved, this, _1), VISIBLE_ITEM_COUNT, 0, 0, Orientation::VERTICAL, CSlider::BROWN);
	slider->setPanningStep(32);
}

void JournalWindow::initializeItems()
{
	OBJECT_CONSTRUCTION;

	for(size_t i = 0; i < getItemCount(); ++i)
	{
		auto label = std::make_shared<JournalLabel>(Rect(13, 195, 149, 31), getItemText(i));
		label->callback = [this, i]()
		{
			selectItem(i);
			redraw();
		};
		labels.push_back(label);
	}

	const int firstVisible = std::max(0, static_cast<int>(getItemCount()) - VISIBLE_ITEM_COUNT);
	recreateItemList(firstVisible);
	selectItem(getItemCount() - 1);

	slider->setAmount(static_cast<int>(getItemCount()));
	if(getItemCount() > VISIBLE_ITEM_COUNT)
	{
		slider->block(false);
		slider->scrollTo(firstVisible);
	}
	else
	{
		slider->block(true);
		slider->scrollToMin();
	}
}

void JournalWindow::selectItem(size_t itemIndex)
{
	selectedLabel = static_cast<int>(itemIndex);
	onItemSelected(itemIndex);
}

void JournalWindow::setContent(const std::string & text, std::vector<std::shared_ptr<CComponent>> components, int componentAreaHeight)
{
	if(description->slider)
		description->slider->scrollToMin();
	description->setText(text);
	componentsBox.reset();

	int descriptionHeight = DESCRIPTION_HEIGHT;
	if(!components.empty())
	{
		descriptionHeight -= componentAreaHeight;
		OBJECT_CONSTRUCTION;
		componentsBox = std::make_shared<CComponentBox>(std::move(components), Rect(205, DESCRIPTION_TOP + descriptionHeight + 15, 385, 115));
	}
	description->resize(Point(385, descriptionHeight));
}

void JournalWindow::recreateItemList(int firstVisible)
{
	for(size_t i = 0; i < labels.size(); ++i)
	{
		labels[i]->pos = Rect(pos.x + 14, pos.y + 195 + (static_cast<int>(i) - firstVisible) * 32, 151, 31);
		if(static_cast<int>(i) >= firstVisible && static_cast<int>(i) < firstVisible + VISIBLE_ITEM_COUNT)
			labels[i]->enable();
		else
			labels[i]->disable();
	}
	updateMinimap();
}

void JournalWindow::sliderMoved(int newPosition)
{
	recreateItemList(newPosition);
	redraw();
}

void JournalWindow::showAll(Canvas & to)
{
	CWindowObject::showAll(to);
	if(selectedLabel < 0 || selectedLabel >= labels.size())
		return;

	// Keep the established quest-log rendering unchanged; event rows are clipped
	// when scrolled out because their previous positions may overlap the map view.
	if(mode == EJournalMode::EVENTS && labels[selectedLabel]->isDisabled())
		return;

	Rect selection = Rect::createAround(labels[selectedLabel]->pos, 1);
	selection.x -= 2;
	selection.w += 2;
	to.drawBorder(selection, Colors::METALLIC_GOLD);
}
