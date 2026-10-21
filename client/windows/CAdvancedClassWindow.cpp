/*
 * CAdvancedClassWindow.cpp, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 */
#include "StdInc.h"
#include "CAdvancedClassWindow.h"

#include <vcmi/spells/Spell.h>

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "render/Canvas.h"

#include "../../lib/CBonusTypeHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/bonuses/Bonus.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/mapObjects/AdvancedClass.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/spells/CSpell.h"
#include "../../lib/texts/CGeneralTextHandler.h"

namespace
{
constexpr int WINDOW_WIDTH = 760;
constexpr int WINDOW_HEIGHT = 560;
constexpr int LIST_WIDTH = 202;
constexpr int LIST_ROW_HEIGHT = 48;
constexpr int LIST_VISIBLE_ROWS = 8;

enum class CardState
{
	LOCKED,
	AVAILABLE,
	ACQUIRED,
	SELECTED
};

std::string translated(const std::string & id)
{
	return GAME->translator().translate(id);
}

std::string selectionProblemText(AdvancedClassSelectionProblem problem)
{
	if(problem == AdvancedClassSelectionProblem::NONE)
		return {};
	return translated("advancedClasses.heroWindow.problem." + std::string(advancedClassSelectionProblemName(problem)));
}

class AdvancedClassCard final : public CIntObject
{
	std::shared_ptr<CLabel> name;
	std::shared_ptr<CLabel> status;
	std::function<void()> callback;
	CardState state;
	bool hovered = false;

	ColorRGBA borderColor() const
	{
		if(state == CardState::ACQUIRED)
			return ColorRGBA(90, 190, 100, 255);
		if(state == CardState::AVAILABLE)
			return ColorRGBA(125, 190, 110, 255);
		if(state == CardState::SELECTED)
			return ColorRGBA(230, 185, 70, 255);
		return ColorRGBA(100, 90, 80, 255);
	}

public:
	AdvancedClassCard(Rect position, std::string nameText, std::string statusText, CardState state_, std::function<void()> callback_)
		: CIntObject(LCLICK | HOVER)
		, callback(std::move(callback_))
		, state(state_)
	{
		OBJECT_CONSTRUCTION;
		pos += position.topLeft();
		pos.w = position.w;
		pos.h = position.h;
		name = std::make_shared<CLabel>(pos.w / 2, 8, FONT_SMALL, ETextAlignment::CENTER,
			state == CardState::LOCKED ? ColorRGBA(145, 140, 130, 255) : Colors::WHITE, std::move(nameText), pos.w - 8);
		status = std::make_shared<CLabel>(pos.w / 2, pos.h - 13, FONT_SMALL, ETextAlignment::CENTER,
			state == CardState::LOCKED ? ColorRGBA(150, 145, 135, 255) : borderColor(), std::move(statusText), pos.w - 8);
	}

	void clickPressed(const Point &) override
	{
		if(callback)
			callback();
	}

	void hover(bool on) override
	{
		hovered = on;
		redraw();
	}

	void showAll(Canvas & to) override
	{
		const auto fill = hovered ? ColorRGBA(30, 30, 20, 150)
			: (state == CardState::LOCKED ? ColorRGBA(0, 0, 0, 145) : ColorRGBA(0, 0, 0, 100));
		to.drawColorBlended(pos, fill);
		to.drawBorder(pos, borderColor(), state == CardState::SELECTED ? 2 : 1);
		CIntObject::showAll(to);
	}
};
}

CAdvancedClassWindow::CAdvancedClassWindow(const CGHeroInstance * hero_)
	: CWindowObject(BORDERED)
	, hero(hero_)
{
	OBJECT_CONSTRUCTION;
	pos.w = WINDOW_WIDTH;
	pos.h = WINDOW_HEIGHT;
	updateShadow();
	center();

	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, pos.w, pos.h));
	candidateBackground = std::make_shared<TransparentFilledRectangle>(Rect(10, 58, LIST_WIDTH, 430), ColorRGBA(0, 0, 0, 55), ColorRGBA(128, 100, 75, 255));
	treeBackground = std::make_shared<TransparentFilledRectangle>(Rect(222, 58, 528, 244), ColorRGBA(0, 0, 0, 55), ColorRGBA(128, 100, 75, 255));
	detailBackground = std::make_shared<TransparentFilledRectangle>(Rect(222, 312, 528, 176), ColorRGBA(0, 0, 0, 75), ColorRGBA(128, 100, 75, 255));

	title = std::make_shared<CLabel>(pos.w / 2, 18, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, translated("advancedClasses.heroWindow.title"));
	candidateHeader = std::make_shared<CLabel>(111, 42, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, translated("advancedClasses.heroWindow.choices"));
	treeHeader = std::make_shared<CLabel>(486, 42, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, translated("advancedClasses.heroWindow.progression"));

	const int nextTier = hero->advancedClass.nextTier();
	for(const auto & definition : LIBRARY->advancedClassHandler->getAll())
	{
		if(definition->valid && definition->tier == nextTier && definition->follows(hero->advancedClass))
			candidates.push_back(definition.get());
	}
	std::ranges::sort(candidates, [this](const auto * lhs, const auto * rhs)
	{
		const bool lhsAvailable = LIBRARY->advancedClassHandler->canSelect(*hero, lhs->id);
		const bool rhsAvailable = LIBRARY->advancedClassHandler->canSelect(*hero, rhs->id);
		if(lhsAvailable != rhsAvailable)
			return lhsAvailable;
		return translated(lhs->nameTextId) < translated(rhs->nameTextId);
	});

	if(!candidates.empty())
		selected = candidates.front();
	else if(nextTier > 1 && nextTier <= 5)
		selected = LIBRARY->advancedClassHandler->find(hero->advancedClass.classes.at(std::min(nextTier, 5) - 2));

	auto createCandidate = [this](size_t index) -> std::shared_ptr<CIntObject>
	{
		if(index >= candidates.size())
			return nullptr;

		const auto * definition = candidates.at(index);
		const bool available = LIBRARY->advancedClassHandler->canSelect(*hero, definition->id);
		const auto status = boost::str(boost::format(translated("advancedClasses.heroWindow.cardRequirements"))
			% definition->requiredLevel % definition->requiredPoints);
		return std::make_shared<AdvancedClassCard>(Rect(0, 0, 184, LIST_ROW_HEIGHT), translated(definition->nameTextId), status,
			definition == selected ? CardState::SELECTED : (available ? CardState::AVAILABLE : CardState::LOCKED),
			[this, definition](){ selectDefinition(definition); });
	};
	const int candidateSlider = candidates.size() > LIST_VISIBLE_ROWS ? 1 : 0;
	candidateList = std::make_shared<CListBox>(createCandidate, Point(15, 66), Point(0, LIST_ROW_HEIGHT), LIST_VISIBLE_ROWS,
		candidates.size(), 0, candidateSlider, Rect(184, 0, LIST_VISIBLE_ROWS * LIST_ROW_HEIGHT, 16));
	candidateList->pos.w = LIST_WIDTH - 20;
	candidateList->pos.h = LIST_VISIBLE_ROWS * LIST_ROW_HEIGHT;
	candidateList->setRedrawParent(true);

	detailText = std::make_shared<CTextBox>("", Rect(232, 322, 508, 156), CSlider::BROWN, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
	selectButton = std::make_shared<CButton>(Point(584, 506), AnimationPath::builtin("settingsWindow/button80"), CButton::tooltipLocalized("advancedClasses.heroWindow.select"), [this](){ confirmSelection(); }, EShortcut::GLOBAL_ACCEPT);
	selectButton->setTextOverlay(translated("advancedClasses.heroWindow.select"), FONT_SMALL, Colors::YELLOW);
	closeButton = std::make_shared<CButton>(Point(680, 506), AnimationPath::builtin("IOKAY"), CButton::tooltip(), [this](){ close(); }, EShortcut::GLOBAL_CANCEL);

	rebuildTree();
	updateDetails();
}

void CAdvancedClassWindow::selectDefinition(const AdvancedClassDefinition * definition)
{
	selected = definition;
	candidateList->reset();
	rebuildTree();
	updateDetails();
	ENGINE->windows().totalRedraw();
}

void CAdvancedClassWindow::rebuildTree()
{
	OBJECT_CONSTRUCTION;
	treeWidgets.clear();
	if(!selected)
		return;

	const auto * root = LIBRARY->advancedClassHandler->getRootClass(*selected);
	if(!root)
		return;

	std::array<std::vector<const AdvancedClassDefinition *>, 4> tiers;
	for(const auto & definition : LIBRARY->advancedClassHandler->getAll())
		if(definition->valid && LIBRARY->advancedClassHandler->getRootClass(*definition) == root)
			tiers.at(definition->tier - 1).push_back(definition.get());

	for(auto & tier : tiers)
		std::ranges::sort(tier, [](const auto * lhs, const auto * rhs){ return lhs->id < rhs->id; });

	for(size_t tierIndex = 1; tierIndex < tiers.size(); ++tierIndex)
	{
		for(size_t row = 0; row < tiers[tierIndex].size(); ++row)
		{
			const auto * definition = tiers[tierIndex][row];
			const int childY = tiers[tierIndex].size() == 1 ? 179 : 138 + static_cast<int>(row) * 82;
			for(const auto & parentId : definition->parents)
			{
				auto parent = std::ranges::find(tiers[tierIndex - 1], parentId, &AdvancedClassDefinition::id);
				if(parent == tiers[tierIndex - 1].end())
					continue;
				const int parentRow = static_cast<int>(std::distance(tiers[tierIndex - 1].begin(), parent));
				const int parentY = tiers[tierIndex - 1].size() == 1 ? 179 : 138 + parentRow * 82;
				treeWidgets.push_back(std::make_shared<SimpleLine>(Point(218 + static_cast<int>(tierIndex) * 129, parentY),
					Point(231 + static_cast<int>(tierIndex) * 129, childY), ColorRGBA(128, 100, 75, 255)));
			}
		}
	}

	for(size_t tierIndex = 0; tierIndex < tiers.size(); ++tierIndex)
	{
		const int x = 231 + static_cast<int>(tierIndex) * 129;
		treeWidgets.push_back(std::make_shared<CLabel>(x + 57, 69, FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW,
			boost::str(boost::format(translated("advancedClasses.heroWindow.tier")) % (tierIndex + 1))));
		const int count = static_cast<int>(tiers[tierIndex].size());
		for(int row = 0; row < count; ++row)
		{
			const auto * definition = tiers[tierIndex][row];
			const int y = count == 1 ? 153 : 112 + row * 82;
			const bool acquired = hero->advancedClass.classes.at(tierIndex) == definition->id;
			const bool available = LIBRARY->advancedClassHandler->canSelect(*hero, definition->id);
			const auto state = definition == selected ? CardState::SELECTED : (acquired ? CardState::ACQUIRED : (available ? CardState::AVAILABLE : CardState::LOCKED));
			std::string status = acquired ? translated("advancedClasses.heroWindow.acquired")
				: (available ? translated("advancedClasses.heroWindow.available") : translated("advancedClasses.heroWindow.locked"));
			treeWidgets.push_back(std::make_shared<AdvancedClassCard>(Rect(x, y, 116, 52), translated(definition->nameTextId), status, state,
				[this, definition](){ selectDefinition(definition); }));
		}
	}
}

void CAdvancedClassWindow::updateDetails()
{
	if(!selected)
	{
		detailText->setText(translated("advancedClasses.heroWindow.noChoices"));
		selectButton->block(true);
		return;
	}

	const int points = LIBRARY->advancedClassHandler->pointsFor(*hero, *selected);
	std::string text = "{" + translated(selected->nameTextId) + "}\n";
	if(!selected->descriptionTextId.empty())
		text += translated(selected->descriptionTextId) + "\n\n";
	text += boost::str(boost::format(translated("advancedClasses.heroWindow.requirementSummary"))
		% hero->level % selected->requiredLevel % points % selected->requiredPoints);

	if(!selected->bonuses.empty())
	{
		text += "\n\n{" + translated("advancedClasses.heroWindow.rewards") + "}";
		const auto * bonusHandler = dynamic_cast<const CBonusTypeHandler *>(LIBRARY->getBth());
		for(const auto & bonus : selected->bonuses)
		{
			std::string description = bonusHandler ? bonusHandler->bonusToString(bonus) : std::string();
			if(description.empty())
				description = translated("advancedClasses.heroWindow.unspecifiedBonus");
			text += "\n- " + description;
		}
	}

	if(selected->abilityConfigured)
	{
		text += "\n\n{" + translated("advancedClasses.heroWindow.ability") + "}\n";
		text += boost::str(boost::format(translated("advancedClasses.heroWindow.abilitySummary"))
			% selected->ability.toSpell()->getNameTranslated() % selected->abilityCooldown);
	}

	const auto problem = LIBRARY->advancedClassHandler->selectionProblem(*hero, selected->id);
	const bool canSelect = problem == AdvancedClassSelectionProblem::NONE;
	if(!canSelect)
		text += "\n\n{" + translated("advancedClasses.heroWindow.status") + "} " + selectionProblemText(problem);

	detailText->setText(text);
	selectButton->block(!canSelect);
}

void CAdvancedClassWindow::confirmSelection()
{
	if(!selected || !LIBRARY->advancedClassHandler->canSelect(*hero, selected->id))
		return;
	const auto question = boost::str(boost::format(translated("advancedClasses.heroWindow.confirm")) % translated(selected->nameTextId));
	GAME->interface()->showYesNoDialog(question, [this, classId = selected->id]()
	{
		GAME->interface()->cb->selectAdvancedClass(hero, classId);
		close();
	}, nullptr);
}
