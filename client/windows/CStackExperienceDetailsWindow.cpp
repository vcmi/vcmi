/*
 * CStackExperienceDetailsWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CStackExperienceDetailsWindow.h"

#include <vcmi/spells/Spell.h>

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../render/CAnimation.h"
#include "../render/CanvasImage.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../render/ImageLocator.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/Images.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"

#include "../../lib/CBonusTypeHandler.h"
#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/bonuses/Propagators.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/TextOperations.h"

namespace
{
int getPreviewExperienceForRank(const std::vector<ui32> & rankThresholds, int rank)
{
	if(rank <= 0 || rankThresholds.empty())
		return 0;

	const int thresholdIndex = std::min(rank - 1, static_cast<int>(rankThresholds.size()) - 1);
	return static_cast<int>(rankThresholds[thresholdIndex]) + 1;
}

int getPreviewExperienceForBonusColumn(const std::vector<ui32> & rankThresholds, int column)
{
	// Table columns are 0..10. Bonus values are defined for ranks 1..10.
	// Column 0 is baseline; columns 1..10 map to bonus ranks 1..10.
	const int maxBonusRank = std::max(0, std::min(10, static_cast<int>(rankThresholds.size()) - 1));
	const int bonusRank = std::clamp(column, 0, maxBonusRank);
	return getPreviewExperienceForRank(rankThresholds, bonusRank);
}

}

int CStackWindow::StackExperienceDetailsWindow::getStackExperienceTierFromCreatureLevel(int creatureLevel)
{
	const int maxTier = static_cast<int>(LIBRARY->creh->expRanks.size()) - 1;
	if(maxTier <= 0)
	{
		static bool warningPrinted = false;
		if(!warningPrinted)
		{
			logGlobal->warn("StackExperienceDetailsWindow: no valid stack experience tiers loaded, defaulting to tier 0");
			warningPrinted = true;
		}
		return 0;
	}

	// Creature level/tier selects which exp-rank threshold table is used.
	// Stack experience rank itself is separate (0..10 columns in the dialog).
	// Keep mapping consistent with CStackInstance::getExpRank():
	// creature levels outside 1..7 use fallback tier 0.
	if(!vstd::iswithin(creatureLevel, 1, 7))
		return 0;

	return std::clamp(creatureLevel, 1, std::min(7, maxTier));
}

int CStackWindow::StackExperienceDetailsWindow::calculateDynamicTableRowCount(const CStackInstance * stack, const CCreature * creature)
{
	// Matches asset generator semantics: image suffix N means
	// table renders N data rows + 1 header row.
	if(!stack || !creature || !GAME->interface())
		return 7;

	struct BonusKey
	{
		BonusType type;
		int subtype;
		bool operator<(const BonusKey & other) const
		{
			return std::tie(type, subtype) < std::tie(other.type, other.subtype);
		}
	};

	int tier = StackExperienceDetailsWindow::getStackExperienceTierFromCreatureLevel(creature->getLevel());
	const auto & rankThresholds = LIBRARY->creh->expRanks[tier];
	auto gameCallback = GAME->interface()->cb.get();

	std::set<BonusKey> uniqueBonuses;
	for(int column = 0; column < MAX_RANKS; ++column)
	{
		const int previewExperience = getPreviewExperienceForBonusColumn(rankThresholds, column);
		CStackInstance preview(gameCallback, creature->getId(), std::max(1, stack->getCount()), true);
		const TExpType totalExperience = static_cast<TExpType>(previewExperience) * static_cast<TExpType>(preview.getCount());
		preview.giveTotalStackExperience(totalExperience);

		auto bonuses = preview.getBonuses(Selector::sourceTypeSel(BonusSource::STACK_EXPERIENCE));
		for(const auto & bonus : *bonuses)
			uniqueBonuses.insert({bonus->type, bonus->subtype.getNum()});
	}

	const int maxRowsWithoutHeader = 7; // keep dialog within 800x600
	const int rowsWithoutHeader = std::clamp(1 + static_cast<int>(uniqueBonuses.size()), 1, maxRowsWithoutHeader); // Experience + bonus rows
	return rowsWithoutHeader;
}

ImagePath CStackWindow::StackExperienceDetailsWindow::getDialogBackground(int rowCount)
{
	const int clampedRowCount = std::clamp(rowCount, 1, 7);
	return ImagePath::builtin("stackExperienceDialogRows" + std::to_string(clampedRowCount));
}

CStackWindow::StackExperienceDetailsWindow::StackExperienceDetailsWindow(const CStackInstance * stack, const CCreature * creatureType)
	: CWindowObject(PLAYER_COLORED_BORDERED_STATUSBAR, getDialogBackground(calculateDynamicTableRowCount(stack, creatureType)))
	, sourceStack(stack)
	, creature(creatureType)
{
	OBJECT_CONSTRUCTION;
	statusbar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(8, pos.h - 26, pos.w - 16, 19), 8, pos.h - 26));

	const int creatureSideMargin = 25;
	const int tableSideMargin = 35;
	const int yOffset = 16;
	const int headerTop = 1 + yOffset;
	const int detailsTop = 68 + yOffset;
	const int tableTop = 218 + yOffset;
	constexpr int tableBaseRowHeight = 35;

	title = std::make_shared<CLabel>(pos.w / 2, headerTop, FONT_BIG, ETextAlignment::TOPCENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.stackExperience.windowTitle"));

	int tier = getStackExperienceTierFromCreatureLevel(this->creature->getLevel());
	const auto & rankThresholds = LIBRARY->creh->expRanks[tier];

	const int expMax = static_cast<int>(rankThresholds.back());
	const int currentRank = std::clamp(sourceStack->getExpRank(), 0, MAX_RANKS - 1);
	const int maxExpPerBattle = static_cast<int>(LIBRARY->creh->maxExpPerBattle[tier]) * expMax / 100;
	const int nextRankExp = (currentRank < MAX_RANKS - 1 && currentRank < static_cast<int>(rankThresholds.size()))
		? std::max(0, static_cast<int>(rankThresholds[currentRank] - sourceStack->getAverageExperience()))
		: 0;
	int expMin = std::max(LIBRARY->creh->expRanks[tier][std::max(currentRank - 1, 0)], static_cast<ui32>(1));
	const int maxNewRecruits = std::max(0, static_cast<int>(sourceStack->getTotalExperience() / expMin - sourceStack->getCount()));
	const int upgradeMultiplier = static_cast<int>(LIBRARY->creh->expAfterUpgrade);
	expMin = LIBRARY->creh->expRanks[tier][9];
	const int expAfterRank10 = static_cast<int>(LIBRARY->creh->expRanks[tier][10] - expMin);
	const int maxRecruitsAtRank10 = std::max(0, static_cast<int>((sourceStack->getCount() * expAfterRank10) / expMin));

	const std::string unitHeader = this->creature->getNamePluralTranslated();

	stackSummary = std::make_shared<CLabel>(pos.w / 2, headerTop + 46, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, unitHeader);

	const int creatureFrameX = creatureSideMargin;
	const int creatureFrameY = detailsTop;
	creatureAnimation = std::make_shared<CCreaturePic>(creatureFrameX + 1, creatureFrameY + 1, this->creature, true, true);
	creatureAnimation->setAmount(sourceStack->getCount());

	const int infoLeftX = creatureSideMargin + 116; // shifted 28px left relative to previous layout
	const int infoColumnGap = 14;
	const int infoLabelWidth = 221;
	const int infoValueWidth = 76;
	const int infoSectionGap = 10; // 2px visual gap between split blocks (+/-4px frame padding)
	const int infoColumnWidth = infoLabelWidth + infoSectionGap + infoValueWidth;
	const int infoRightX = infoLeftX + infoColumnWidth + infoColumnGap;
	const int infoTop = detailsTop + 4;
	const int infoFieldHeight = 22;
	const int infoFieldGap = 2;
	const int infoRowStep = infoFieldHeight + infoFieldGap;
	const int infoLongFieldHeight = infoFieldHeight * 2 + infoFieldGap + 8;

	auto addInfo = [&](int row, int column, const std::string & key, const std::string & value)
	{
		const int baseX = column == 0 ? infoLeftX : infoRightX;
		const int rowY = infoTop + row * infoRowStep;
		labels.push_back(std::make_shared<CLabel>(baseX + 2, rowY + infoFieldHeight / 2, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, key + ":"));
		labels.push_back(std::make_shared<CLabel>(baseX + infoLabelWidth + infoSectionGap + 2, rowY + infoFieldHeight / 2, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, value));
	};

	addInfo(0, 0, LIBRARY->generaltexth->translate("vcmi.stackExperience.popup.rank"), std::to_string(currentRank));
	addInfo(0, 1, LIBRARY->generaltexth->translate("vcmi.stackExperience.popup.experiencePoints"), std::to_string(sourceStack->getAverageExperience()));
	addInfo(1, 0, LIBRARY->generaltexth->translate("vcmi.stackExperience.popup.maxPerBattle"), std::to_string(maxExpPerBattle));
	addInfo(1, 1, LIBRARY->generaltexth->translate("vcmi.stackExperience.popup.nextRank"), std::to_string(nextRankExp));
	addInfo(2, 0, LIBRARY->generaltexth->translate("vcmi.stackExperience.popup.upgradeMultiplier"), boost::str(boost::format("%d%%") % upgradeMultiplier));
	addInfo(2, 1, LIBRARY->generaltexth->translate("vcmi.stackExperience.popup.experienceAfterRank10"), std::to_string(expAfterRank10));

	auto addLongInfo = [&](int column, const std::string & key, const std::string & value)
	{
		const int baseX = column == 0 ? infoLeftX : infoRightX;
		const int rowY = infoTop + 3 * infoRowStep;
		labels.push_back(std::make_shared<CMultiLineLabel>(
			Rect(baseX + 2, rowY + 1, infoLabelWidth - 2, infoLongFieldHeight - 2),
			FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE,
			key + ":"));
		labels.push_back(std::make_shared<CLabel>(
			baseX + infoLabelWidth + infoSectionGap + 2,
			rowY + infoLongFieldHeight / 2,
			FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE,
			value));
	};

	addLongInfo(0, LIBRARY->generaltexth->translate("vcmi.stackExperience.popup.maxRecruits"), std::to_string(maxNewRecruits));
	addLongInfo(1, LIBRARY->generaltexth->translate("vcmi.stackExperience.popup.maxRecruitsRank10"), std::to_string(maxRecruitsAtRank10));

	auto makeTooltipAndPopupText = [](const std::string & descriptionText)
	{
		std::string tooltip = descriptionText;
		std::string popup = descriptionText;
		boost::replace_all(tooltip, "\n\n", ": ");
		boost::replace_all(tooltip, "\n", ": ");
		boost::replace_all(popup, "\n", "\n\n");
		return std::pair<std::string, std::string>{tooltip, popup};
	};

	const auto [experienceTooltip, experiencePopup] = makeTooltipAndPopupText(LIBRARY->generaltexth->translate("vcmi.stackExperience.desc.experience"));

	std::vector<NumericRow> rows = {
		{LIBRARY->generaltexth->translate("vcmi.stackExperience.table.experience"), [&rankThresholds](const CStackInstance & stackInst)
			{
				const int rank = std::clamp(stackInst.getExpRank(), 0, MAX_RANKS - 1);
				return static_cast<int>(rankThresholds[std::min(rank, static_cast<int>(rankThresholds.size()) - 1)]);
		},
		[](const CStackInstance &){ return true; },
		ImagePath::builtin("stackExperienceIconExperience"), true, -1, experienceTooltip, experiencePopup, false, false, false, false, true},
	};

	struct BonusKey
	{
		BonusType type;
		BonusSubtypeID subtype;

		bool operator<(const BonusKey & other) const
		{
			if(type != other.type)
				return type < other.type;
			return subtype.getNum() < other.subtype.getNum();
		}
	};

	auto getBonusKey = [](const std::shared_ptr<const Bonus> & bonus)
	{
		return BonusKey{bonus->type, bonus->subtype};
	};

	auto makeStackExpSelector = [](const BonusKey & key)
	{
		return Selector::sourceTypeSel(BonusSource::STACK_EXPERIENCE).And(Selector::typeSubtype(key.type, key.subtype));
	};

	std::map<BonusKey, std::shared_ptr<const Bonus>> dynamicBonuses;
	std::set<std::string> loggedDuplicateBonusKeys;
	for(int column = 0; column < MAX_RANKS; ++column)
	{
		auto gameCallback = GAME->interface() ? GAME->interface()->cb.get() : nullptr;
		const int previewExperience = getPreviewExperienceForBonusColumn(rankThresholds, column);
		CStackInstance preview(gameCallback, this->creature->getId(), std::max(1, sourceStack->getCount()), true);
		const TExpType totalExperience = static_cast<TExpType>(previewExperience) * static_cast<TExpType>(preview.getCount());
		preview.giveTotalStackExperience(totalExperience);
		auto bonuses = preview.getBonuses(Selector::sourceTypeSel(BonusSource::STACK_EXPERIENCE));
		for(const auto & bonus : *bonuses)
		{
			const auto key = getBonusKey(bonus);
			auto [it, inserted] = dynamicBonuses.emplace(key, bonus);
			// Debug noise
			if(!inserted)
			{
				const auto & kept = it->second;
				const std::string duplicateSignature = boost::str(boost::format("%s|%d|%d|%d|%d|%d|%d|%d|%d")
					% (this->creature ? this->creature->getJsonKey() : "<null>")
					% static_cast<int>(key.type)
					% key.subtype.getNum()
					% kept->val
					% static_cast<int>(kept->valType)
					% bonus->val
					% static_cast<int>(bonus->valType)
					% kept->subtype.getNum()
					% bonus->subtype.getNum());
				if(loggedDuplicateBonusKeys.insert(duplicateSignature).second)
				{
					logGlobal->warn(
						"StackExperienceDetailsWindow: duplicate normalized bonus key detected for creature %s (first seen at column %d); "
						"key(type=%d, subtype=%d). Kept bonus(type=%d, subtype=%d, val=%d, valType=%d, source=%d), "
						"dropped bonus(type=%d, subtype=%d, val=%d, valType=%d, source=%d)",
						this->creature ? this->creature->getJsonKey().c_str() : "<null>",
						column,
						static_cast<int>(key.type),
						key.subtype.getNum(),
						static_cast<int>(kept->type),
						kept->subtype.getNum(),
						kept->val,
						static_cast<int>(kept->valType),
						static_cast<int>(kept->source),
						static_cast<int>(bonus->type),
						bonus->subtype.getNum(),
						bonus->val,
						static_cast<int>(bonus->valType),
						static_cast<int>(bonus->source));
				}
			}
		}
	}

	auto addBonusRow = [&](const BonusKey & key, const std::shared_ptr<const Bonus> & bonus, const std::string & label, const std::string & descriptionText, int iconFrame = -1, std::optional<ImagePath> iconOverride = std::nullopt, bool percent = false, bool binary = false, bool showSign = true, bool alwaysVisible = false, std::optional<std::function<int(const CStackInstance &)>> valueGetterOverride = std::nullopt, std::optional<CSelector> selectorOverride = std::nullopt)
	{
		const auto selector = selectorOverride.value_or(makeStackExpSelector(key));
		const ImagePath iconPath = iconOverride.value_or(sourceStack->bonusToGraphics(std::const_pointer_cast<Bonus>(bonus)));
		const bool allowPresenceFallback = bonus->val == 0;
		auto [tooltip, popup] = makeTooltipAndPopupText(descriptionText);
		rows.push_back({label, [selector, binary, valueGetterOverride](const CStackInstance & stackInst)
			{
				if(valueGetterOverride.has_value())
					return (*valueGetterOverride)(stackInst);

				if(binary)
					return stackInst.hasBonus(selector) ? 1 : 0;

				return stackInst.valOfBonuses(selector);
			},
			[selector](const CStackInstance & stackInst)
			{
				return stackInst.hasBonus(selector);
			},
			iconPath, true, iconFrame, tooltip, popup, percent, binary, allowPresenceFallback, showSign, alwaysVisible});
	};

	auto getBonusDisplayName = [&](const std::shared_ptr<const Bonus> & bonus)
	{
		std::string rowLabel;
		if(!bonus->description.empty())
			rowLabel = bonus->description.toString();
		else
		{
			auto mutableBonus = std::const_pointer_cast<Bonus>(bonus);
			rowLabel = sourceStack->bonusToString(mutableBonus);
		}

		const auto lineBreak = rowLabel.find('\n');
		if(lineBreak != std::string::npos)
			rowLabel = rowLabel.substr(0, lineBreak);

		if(rowLabel.empty())
		{
			if(const auto * bonusTypeHandler = dynamic_cast<const CBonusTypeHandler *>(LIBRARY->getBth()))
				rowLabel = bonusTypeHandler->bonusToString(bonus->type);
			else
				rowLabel = "Bonus";
		}

		return rowLabel;
	};
	auto getBonusTooltipText = [&](const std::shared_ptr<const Bonus> & bonus)
	{
		if(!bonus->description.empty())
			return bonus->description.toString();

		auto tooltip = sourceStack->bonusToString(std::const_pointer_cast<Bonus>(bonus));
		if(!tooltip.empty())
			return tooltip;

		if(const auto * bonusTypeHandler = dynamic_cast<const CBonusTypeHandler *>(LIBRARY->getBth()))
			return bonusTypeHandler->bonusToString(bonus->type);

		return tooltip;
	};

	
	auto isPercentBonus = [](const std::shared_ptr<const Bonus> & bonus)
	{
		const std::string descriptionText = bonus->description.toString();
		if(descriptionText.find('%') != std::string::npos)
			return true;

		return bonus->valType == BonusValueType::PERCENT_TO_BASE
			|| bonus->valType == BonusValueType::PERCENT_TO_ALL
			|| bonus->type == BonusType::MAGIC_RESISTANCE;
	};

	struct PreferredRowPresentation
	{
		std::string label;
		ImagePath icon;
		std::string tooltip;
		std::optional<std::function<int(const CStackInstance &)>> valueGetterOverride;
	};

	auto getPreferredPresentation = [&](const BonusKey & key) -> std::optional<PreferredRowPresentation>
	{
		if(key.type == BonusType::PRIMARY_SKILL && key.subtype == BonusSubtypeID(PrimarySkill::ATTACK))
			return PreferredRowPresentation{LIBRARY->generaltexth->allTexts[190], ImagePath::builtin("stackExperienceIconAttack"), LIBRARY->generaltexth->translate("vcmi.stackExperience.desc.attack"), std::nullopt};
		if(key.type == BonusType::PRIMARY_SKILL && key.subtype == BonusSubtypeID(PrimarySkill::DEFENSE))
			return PreferredRowPresentation{LIBRARY->generaltexth->allTexts[391], ImagePath::builtin("stackExperienceIconDefense"), LIBRARY->generaltexth->translate("vcmi.stackExperience.desc.defense"), std::nullopt};
		if(key.type == BonusType::CREATURE_DAMAGE && key.subtype == BonusSubtypeID(BonusCustomSubtype::creatureDamageMin))
			return PreferredRowPresentation{LIBRARY->generaltexth->translate("vcmi.stackExperience.table.minDamage"), ImagePath::builtin("stackExperienceIconMinDamage"), LIBRARY->generaltexth->translate("vcmi.stackExperience.desc.minDamage"), std::nullopt};
		if(key.type == BonusType::CREATURE_DAMAGE && key.subtype == BonusSubtypeID(BonusCustomSubtype::creatureDamageMax))
			return PreferredRowPresentation{LIBRARY->generaltexth->translate("vcmi.stackExperience.table.maxDamage"), ImagePath::builtin("stackExperienceIconMaxDamage"), LIBRARY->generaltexth->translate("vcmi.stackExperience.desc.maxDamage"), std::nullopt};
		if(key.type == BonusType::STACK_HEALTH)
		{
			auto valueOverride = [selector = makeStackExpSelector(key)](const CStackInstance & stackInst)
			{
				int result = 0;
				auto bonuses = stackInst.getBonuses(selector);
				for(const auto & b : *bonuses)
					result += b->val;
				return result;
			};
			return PreferredRowPresentation{LIBRARY->generaltexth->allTexts[388], ImagePath::builtin("stackExperienceIconHealth"), LIBRARY->generaltexth->translate("vcmi.stackExperience.desc.health"), valueOverride};
		}
		if(key.type == BonusType::STACKS_SPEED)
			return PreferredRowPresentation{LIBRARY->generaltexth->allTexts[193], ImagePath::builtin("stackExperienceIconSpeed"), LIBRARY->generaltexth->translate("vcmi.stackExperience.desc.speed"), std::nullopt};
		if(key.type == BonusType::SHOTS)
			return PreferredRowPresentation{LIBRARY->generaltexth->allTexts[198], ImagePath::builtin("stackExperienceIconShots"), LIBRARY->generaltexth->translate("vcmi.stackExperience.desc.shots"), std::nullopt};
		if(key.type == BonusType::CASTS)
			return PreferredRowPresentation{LIBRARY->generaltexth->allTexts[399], ImagePath::builtin("stackExperienceIconCasts"), LIBRARY->generaltexth->translate("vcmi.stackExperience.desc.casts"), std::nullopt};

		return std::nullopt;
	};

	std::set<BonusType> handledSubtypeAgnosticTypes;
	for(const auto & [key, bonus] : dynamicBonuses)
	{
		const bool percentValue = isPercentBonus(bonus);
		const bool binaryValue = false;
		const bool subtypeAgnosticPreferred = key.type == BonusType::CASTS || key.type == BonusType::MAGIC_RESISTANCE || key.type == BonusType::STACKS_SPEED || key.type == BonusType::SHOTS || key.type == BonusType::STACK_HEALTH;
		if(subtypeAgnosticPreferred && handledSubtypeAgnosticTypes.count(key.type))
			continue;
		if(subtypeAgnosticPreferred)
			handledSubtypeAgnosticTypes.insert(key.type);
		const auto selectorOverride = subtypeAgnosticPreferred
			? std::optional<CSelector>(Selector::sourceTypeSel(BonusSource::STACK_EXPERIENCE).And(Selector::type()(key.type)))
			: std::nullopt;
		if(const auto preferredPresentation = getPreferredPresentation(key))
		{
			addBonusRow(key, bonus, preferredPresentation->label, preferredPresentation->tooltip, -1, preferredPresentation->icon, percentValue, binaryValue, true, false, preferredPresentation->valueGetterOverride, selectorOverride);
			continue;
		}
		addBonusRow(key, bonus, getBonusDisplayName(bonus), getBonusTooltipText(bonus), -1, std::nullopt, percentValue, binaryValue, true, false, std::nullopt, selectorOverride);
	}

	preparedRows.reserve(rows.size());
	for(const auto & row : rows)
	{
		PreparedRow prepared;
		prepared.title = row.title;
		prepared.icon = row.icon;
		prepared.hasIcon = row.hasIcon;
		prepared.iconFrame = row.iconFrame;
		prepared.tooltipText = row.tooltipText;
		prepared.popupText = row.popupText;
		prepared.percent = row.percent;
		prepared.showSign = row.showSign;
		prepared.alwaysVisible = row.alwaysVisible;

		bool hasNumericValue = false;
		bool hasActiveBonus = false;
		bool onlyBinary = true;
		std::array<int, MAX_RANKS> numericValues{};
		std::array<int, MAX_RANKS> presenceValues{};
		for(int column = 0; column < MAX_RANKS; ++column)
		{
			int value = 0;
			bool isActive = false;
			if(row.icon == ImagePath::builtin("stackExperienceIconExperience"))
			{
				value = column == 0 ? 0 : static_cast<int>(rankThresholds[std::min(column - 1, static_cast<int>(rankThresholds.size()) - 1)]);
			}
			else
			{
				const int previewExperience = getPreviewExperienceForBonusColumn(rankThresholds, column);
				auto gameCallback = GAME->interface() ? GAME->interface()->cb.get() : nullptr;
				CStackInstance preview(gameCallback, this->creature->getId(), std::max(1, sourceStack->getCount()), true);
				const TExpType totalExperience = static_cast<TExpType>(previewExperience) * static_cast<TExpType>(preview.getCount());
				preview.giveTotalStackExperience(totalExperience);
				value = row.valueGetter(preview);
				isActive = row.presenceGetter(preview);
			}
			numericValues[column] = value;
			presenceValues[column] = isActive ? 1 : 0;
			hasNumericValue = hasNumericValue || value != 0;
			hasActiveBonus = hasActiveBonus || isActive;
			onlyBinary = onlyBinary && (value == 0 || value == 1);
		}

		if(hasNumericValue || row.alwaysVisible)
		{
			prepared.values = numericValues;
			prepared.binary = row.binary && onlyBinary;
			preparedRows.push_back(std::move(prepared));
		}
		else if(hasActiveBonus && row.allowPresenceFallback)
		{
			// Boolean stack-experience entries are loaded as presence-only bonuses
			// (CCreatureHandler::loadStackExperience bool). Show activation by rank.
			prepared.values = presenceValues;
			prepared.binary = true;
			preparedRows.push_back(std::move(prepared));
		}
	}

	std::set<std::string> uniqueRowSignatures;
	preparedRows.erase(std::remove_if(preparedRows.begin(), preparedRows.end(), [&](const PreparedRow & row)
	{
		std::string signature = row.title + "|" + row.icon.getOriginalName() + "|" + (row.binary ? "1" : "0");
		for(int value : row.values)
			signature += "|" + std::to_string(value);

		if(uniqueRowSignatures.count(signature))
			return true;

		uniqueRowSignatures.insert(signature);
		return false;
	}), preparedRows.end());

	auto getFirstVisibleColumn = [](const PreparedRow & row)
	{
		for(int column = 0; column < MAX_RANKS; ++column)
			if(row.values[column] != 0)
				return column;
		return MAX_RANKS;
	};

	const auto experienceIcon = ImagePath::builtin("stackExperienceIconExperience");
	std::stable_sort(preparedRows.begin(), preparedRows.end(), [&](const PreparedRow & lhs, const PreparedRow & rhs)
	{
		const bool lhsIsExperience = lhs.icon == experienceIcon;
		const bool rhsIsExperience = rhs.icon == experienceIcon;
		if(lhsIsExperience != rhsIsExperience)
			return lhsIsExperience;

		const int lhsActivationColumn = getFirstVisibleColumn(lhs);
		const int rhsActivationColumn = getFirstVisibleColumn(rhs);
		if(lhsActivationColumn != rhsActivationColumn)
			return lhsActivationColumn < rhsActivationColumn;

		return lhs.title < rhs.title;
	});

	const int rowNameWidth = 36;
	const int colWidth = (pos.w - 2 * tableSideMargin - rowNameWidth) / MAX_RANKS;
	const int rowHeight = tableBaseRowHeight;
	const int tableWidth = rowNameWidth + colWidth * MAX_RANKS;
	this->tableTop = tableTop;
	this->tableRowHeight = rowHeight;
	this->tableSideMargin = tableSideMargin;
	this->tableRowNameWidth = rowNameWidth;
	this->tableColWidth = colWidth;

	for(int rank = 0; rank < MAX_RANKS; ++rank)
	{
		const int iconX = tableSideMargin + rowNameWidth + rank * colWidth + (colWidth - 32) / 2;
		const int iconY = tableTop + (rowHeight - 32) / 2;
		auto rankIcon = std::make_shared<CAnimImage>(AnimationPath::builtin("stackWindow/levels"), rank, 0, iconX, iconY);
		rankIcon->setScale(Point(32, 32));
		labels.push_back(rankIcon);
	}

	constexpr int maxVisibleBonusRows = 7;
	const int totalBonusRows = static_cast<int>(preparedRows.size());
	visibleBonusRows = std::min(maxVisibleBonusRows, totalBonusRows);
	const int tableRowsVisible = visibleBonusRows + 1; // header + visible bonus rows
	currentRankFrame = std::make_shared<GraphicalPrimitiveCanvas>(Rect(tableSideMargin, tableTop, tableWidth, rowHeight * tableRowsVisible));
	currentRankFrame->addRectangle(Point(rowNameWidth + currentRank * colWidth + 1, 0), Point(colWidth, rowHeight * tableRowsVisible), Colors::BRIGHT_YELLOW);
	currentRankFrame->addRectangle(Point(rowNameWidth + currentRank * colWidth + 2, 1), Point(colWidth - 2, rowHeight * tableRowsVisible - 2), Colors::BRIGHT_YELLOW);

	if(totalBonusRows > maxVisibleBonusRows)
	{
		tableSlider = std::make_shared<CSlider>(Point(pos.w - 25 - 9, tableTop + rowHeight), rowHeight * visibleBonusRows, [this](int){ rebuildTableRows(); setRedrawParent(true); redraw(); }, visibleBonusRows, totalBonusRows, 0, Orientation::VERTICAL, CSlider::BROWN);
		tableSlider->setPanningStep(rowHeight);
		tableSlider->setScrollBounds(Rect(-pos.w + tableSlider->pos.w, 0, pos.w, pos.h));
	}
	else
	{
		tableSlider.reset();
	}
	rebuildTableRows();

	const int centerX = pos.w / 2;
	constexpr int closeButtonBottomMargin = 40;
	constexpr int closeButtonWidth = 64;
	constexpr int closeButtonHeight = 32;
	closeButton = std::make_shared<CButton>(Point(centerX - closeButtonWidth / 2, pos.h - closeButtonBottomMargin - closeButtonHeight), AnimationPath::builtin("IOKAY.DEF"), CButton::tooltip(LIBRARY->generaltexth->translate("vcmi.stackExperience.closeDetails")), [this](){ close(); }, EShortcut::GLOBAL_ACCEPT);
	closeButton->setBorderColor(Colors::METALLIC_GOLD);
}

void CStackWindow::StackExperienceDetailsWindow::rebuildTableRows()
{
	for(const auto & widget : tableBackgroundWidgets)
		removeChild(widget.get());
	for(const auto & widget : tableRowWidgets)
		removeChild(widget.get());

	tableBackgroundWidgets.clear();
	tableRowWidgets.clear();
	const int firstRow = tableSlider ? tableSlider->getValue() : 0;
	const int currentRank = std::clamp(sourceStack->getExpRank(), 0, MAX_RANKS - 1);
	auto withStackExperiencePlaceholders = [](std::string text, int value)
	{
		boost::replace_all(text, "${val}", std::to_string(value));
		return text;
	};

	for(int localRow = 0; localRow < visibleBonusRows; ++localRow)
	{
		const int rowIndex = firstRow + localRow;
		if(rowIndex >= static_cast<int>(preparedRows.size()))
			break;
		const int rowY = tableTop + (localRow + 1) * tableRowHeight + tableRowHeight / 2;

		if(preparedRows[rowIndex].hasIcon)
		{
			const int x = tableSideMargin + (tableRowNameWidth - 32) / 2;
			const int y = tableTop + (localRow + 1) * tableRowHeight + (tableRowHeight - 32) / 2;
			if(preparedRows[rowIndex].iconFrame >= 0)
			{
				auto iconImage = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("PSKIL42"), EImageBlitMode::COLORKEY)->getImage(preparedRows[rowIndex].iconFrame);
				iconImage->scaleTo(Point(32, 32), EScalingAlgorithm::BILINEAR);
				auto rowIcon = std::make_shared<CPicture>(iconImage, Point(x, y));
				tableRowWidgets.push_back(rowIcon);
				addChild(rowIcon.get(), true);
			}
			else
			{
				auto rowIcon = std::make_shared<CPicture>(preparedRows[rowIndex].icon, x, y);
					rowIcon->scaleTo(Point(32, 32));
					tableRowWidgets.push_back(rowIcon);
					addChild(rowIcon.get(), true);
				}

				const bool inactiveAtCurrentRank = preparedRows[rowIndex].values[currentRank] == 0 && std::any_of(preparedRows[rowIndex].values.begin() + currentRank + 1, preparedRows[rowIndex].values.end(), [](int value){ return value != 0; });
				const bool isExperienceRow = (rowIndex == 0);
				if(inactiveAtCurrentRank && !isExperienceRow)
				{
					auto overlayImage = ENGINE->renderHandler().loadImage(ImageLocator(ImagePath::builtin("stackExperienceIconInactiveOverlay"), EImageBlitMode::COLORKEY));
					auto inactiveOverlay = std::make_shared<CPicture>(overlayImage, Point(x, y));
					tableRowWidgets.push_back(inactiveOverlay);
					addChild(inactiveOverlay.get(), true);
				}
		}
		if(preparedRows[rowIndex].hasIcon)
		{
			const int currentValue = preparedRows[rowIndex].values[currentRank];
			std::string hoverText = withStackExperiencePlaceholders(preparedRows[rowIndex].tooltipText, currentValue);
			std::string popupText = withStackExperiencePlaceholders(preparedRows[rowIndex].popupText, currentValue);
			if(hoverText.empty())
				hoverText = !popupText.empty() ? popupText : preparedRows[rowIndex].title;
			if(popupText.empty())
				popupText = hoverText;

			auto iconRClick = std::make_shared<LRClickableAreaWText>(
				Rect(tableSideMargin + (tableRowNameWidth - 32) / 2, tableTop + (localRow + 1) * tableRowHeight + (tableRowHeight - 32) / 2, 32, 32),
				hoverText,
				popupText);
			tableRowWidgets.push_back(iconRClick);
			addChild(iconRClick.get(), true);
		}
		else
		{
			auto rowTitle = std::make_shared<CLabel>(tableSideMargin + 6, rowY, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, preparedRows[rowIndex].title);
			tableRowWidgets.push_back(rowTitle);
			addChild(rowTitle.get(), true);
		}
		for(int rank = 0; rank < MAX_RANKS; ++rank)
		{
			const int value = preparedRows[rowIndex].values[rank];
			std::string valueText;
			if(preparedRows[rowIndex].binary)
				valueText = value != 0 ? LIBRARY->generaltexth->translate("vcmi.stackExperience.table.yes") : LIBRARY->generaltexth->translate("vcmi.stackExperience.table.no");
			else
				valueText = (preparedRows[rowIndex].showSign && value > 0 ? "+" : "") + std::to_string(value) + (preparedRows[rowIndex].percent ? "%" : "");
			auto valueLabel = std::make_shared<CLabel>(tableSideMargin + tableRowNameWidth + rank * tableColWidth + tableColWidth / 2, rowY, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, valueText);
			tableRowWidgets.push_back(valueLabel);
			addChild(valueLabel.get(), true);
		}
	}
}
