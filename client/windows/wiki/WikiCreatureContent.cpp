/*
 * WikiCreatureContent.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "WikiCreatureContent.h"
#include "WikiCommon.h"

#include "../../widgets/CViewport.h"
#include "../../widgets/Images.h"
#include "../../widgets/MiscWidgets.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/GraphicalPrimitiveCanvas.h"
#include "../../render/Canvas.h"
#include "../../render/Colors.h"
#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../CCreatureWindow.h"

#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/CBonusTypeHandler.h"
#include "../../../lib/CStack.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/ResourceSet.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../InfoWindows.h"

#include "../../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../../lib/mapObjectConstructors/DwellingInstanceConstructor.h"
#include "../../../lib/mapObjects/ObjectTemplate.h"

// ─────────────────────────────────────────────────────────────────────────────
// Layout constants
// ─────────────────────────────────────────────────────────────────────────────

static constexpr int MARGIN = 4;
static constexpr int GAP    = 10;
static constexpr int CELL_L = 4;
static constexpr int CELL_T = 2;

// ─────────────────────────────────────────────────────────────────────────────
// buildCreatureContent – main entry point
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::shared_ptr<CIntObject>> buildCreatureContent( // NOSONAR (complex by nature – many creature attributes)
	CViewport & viewport,
	const CCreature * creature,
	int viewportWidth,
	bool blueStyle,
	WikiCreatureNavigateCallback navigateCallback)
{
	std::vector<std::shared_ptr<CIntObject>> widgets;
	if(!creature) return widgets;

	OBJECT_CONSTRUCTION_TARGETED(viewport.content());
	const Rect clipRect = viewport.clipRect();

	const int W   = viewportWidth;
	int       curY = 12;

	// ── 1. Title ────────────────────────────────────────────────────────
	widgets.push_back(std::make_shared<CLabel>(
		W / 2, curY,
		FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,
		creature->getNameSingularTranslated()));
	curY += 28;

	// ── 2. Creature animation (CCreaturePic handles footset + faction background) ──
	{
		auto pic = std::make_shared<CCreaturePic>(0, curY, creature, true, true);
		const int picW = pic->pos.w;
		const int picH = pic->pos.h;
		const int centerX = (W - picW) / 2;
		pic->moveBy(Point(centerX, 0));
		widgets.push_back(pic);

		const CreatureID crId(creature->getIndex());
		widgets.push_back(std::make_shared<WikiClickable>(
			Rect(centerX, curY, picW, picH),
			nullptr,
			[crId](){
				ENGINE->windows().createAndPushWindow<CStackWindow>(
					LIBRARY->creh->objects[crId.getNum()].get(), true);
			},
			blueStyle, clipRect));

		curY += picH + GAP;
	}

	// ── 2.5. Description text ────────────────────────────────────────────
	{
		const std::string desc = creature->getDescriptionTranslated();
		if(!desc.empty())
		{
			auto label = std::make_shared<CMultiLineLabel>(
				Rect(MARGIN, curY, W - MARGIN * 2, 4000),
				FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, desc);
			label->pos.h = label->textSize.y;
			curY += label->textSize.y + GAP;
			widgets.push_back(std::move(label));
		}
	}

	// ── 3. Stats table ──────────────────────────────────────────────────
	{
		curY += 16;
		widgets.push_back(std::make_shared<CLabel>(
			W / 2, curY,
			FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.wiki.creature.section.stats")));
		curY += 20;

		auto tr = [](const char * key){ return LIBRARY->generaltexth->translate(key); };
		struct StatRow { std::string label; std::string value; };
		std::vector<StatRow> stats;

		// Use a hypothetical CStackInstance to evaluate limiters correctly (e.g. conditional rune bonuses)
		CStackInstance fakeStack(nullptr, creature->getId(), 1, true);
		const bool shooter = fakeStack.hasBonusOfType(BonusType::SHOOTER) && fakeStack.valOfBonuses(BonusType::SHOTS);

		stats.push_back({tr("vcmi.wiki.creature.stat.level"),   std::to_string(creature->getLevel())});
		stats.push_back({tr("core.genrltxt.190"),  std::to_string(fakeStack.getAttack(shooter))});
		stats.push_back({tr("core.genrltxt.191"), std::to_string(fakeStack.getDefense(shooter))});

		const int dmgMin = fakeStack.getMinDamage(shooter);
		const int dmgMax = fakeStack.getMaxDamage(shooter);
		if(dmgMin == dmgMax)
			stats.push_back({tr("core.genrltxt.199"), std::to_string(dmgMin)});
		else
			stats.push_back({tr("core.genrltxt.199"), std::to_string(dmgMin) + " - " + std::to_string(dmgMax)});

		stats.push_back({tr("core.genrltxt.193"),     std::to_string(fakeStack.getMovementRange())});
		stats.push_back({tr("core.help.439.help"), std::to_string(fakeStack.getMaxHealth())});
		stats.push_back({tr("core.genrltxt.194"),    std::to_string(creature->getGrowth())});

		const int baseShots = fakeStack.valOfBonuses(BonusType::SHOTS);
		if(baseShots > 0)
			stats.push_back({tr("vcmi.wiki.creature.stat.shots"),      std::to_string(baseShots)});
		const int baseCasts = fakeStack.valOfBonuses(BonusType::CASTS);
		if(baseCasts > 0)
			stats.push_back({tr("core.genrltxt.387"), std::to_string(baseCasts)});
		if(creature->getHorde() > 0)
			stats.push_back({tr("vcmi.wiki.creature.stat.hordegrowth"), std::to_string(creature->getHorde())});
		if(creature->isDoubleWide())
			stats.push_back({tr("vcmi.wiki.creature.stat.doublewide"), tr("vcmi.wiki.creature.stat.yes")});

		const int mapMin = creature->getAdvMapAmountMin();
		const int mapMax = creature->getAdvMapAmountMax();
		if(mapMin > 0 || mapMax > 0)
		{
			const std::string mapAmt = (mapMin == mapMax)
				? std::to_string(mapMin)
				: std::to_string(mapMin) + " – " + std::to_string(mapMax);
			stats.push_back({tr("vcmi.wiki.creature.stat.mapamount"), mapAmt});
		}

		stats.push_back({tr("vcmi.wiki.creature.stat.aivalue"),    std::to_string(creature->getAIValue())});
		stats.push_back({tr("vcmi.wiki.creature.stat.fightvalue"), std::to_string(creature->getFightValue())});

		const int tableW = W - MARGIN * 2;
		const int rowH   = 18;
		const int colLbl = tableW / 2;
		const int rowCnt = static_cast<int>(stats.size());

		widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW, std::vector<int>{colLbl, tableW - colLbl}, 0, rowH, rowCnt, blueStyle));

		for(int i = 0; i < rowCnt; ++i)
		{
			const int ry = curY + i * rowH;
			widgets.push_back(std::make_shared<CLabel>(
				MARGIN + CELL_L, ry + CELL_T,
				FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
				stats[i].label));
			widgets.push_back(std::make_shared<CLabel>(
				MARGIN + colLbl + CELL_L, ry + CELL_T,
				FONT_SMALL, ETextAlignment::TOPLEFT,
				blueStyle ? ColorRGBA(160, 210, 255, 255) : Colors::YELLOW,
				stats[i].value));
		}
		curY += rowCnt * rowH + GAP;
	}

	// ── 4. Cost row ─────────────────────────────────────────────────────
	{
		const TResources & cost = creature->getFullRecruitCost();
		const bool hasCost = cost.nonZero();
		if(hasCost)
		{
			curY += 16;
			widgets.push_back(std::make_shared<CLabel>(
				W / 2, curY,
				FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
				LIBRARY->generaltexth->translate("vcmi.wiki.creature.section.cost")));
			curY += 20;
			widgets.push_back(std::make_shared<WikiResourceCost>(cost, MARGIN, curY, W - MARGIN * 2));
			curY += 18 + GAP;
		}
	}

	// ── 5. Upgrades / related creatures ─────────────────────────────────
	{
		std::vector<const CCreature *> upgrades;
		for(const CreatureID uid : creature->upgrades)
		{
			if(uid.getNum() >= 0 && uid.getNum() < static_cast<int>(LIBRARY->creh->objects.size()))
			{
				if(const auto & up = LIBRARY->creh->objects[uid.getNum()])
					upgrades.push_back(up.get());
			}
		}

		std::vector<const CCreature *> bases;
		for(const auto & cr : LIBRARY->creh->objects)
		{
			if(cr && cr.get() != creature
				&& cr->upgrades.count(CreatureID(creature->getIndex())) > 0)
			{
				bases.push_back(cr.get());
			}
		}

		auto addRelatedTable = [&](const std::string & sectionLabel,
		                           const std::vector<const CCreature *> & list)
		{
			if(list.empty()) return;
			curY += 16;
			widgets.push_back(std::make_shared<CLabel>(
				W / 2, curY,
				FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW, sectionLabel));
			curY += 20;

			const int iconW  = 36;
			const int rowH   = 36;
			const int tableW = W - MARGIN * 2;
			const ColorRGBA bdr = wikiBorderColor(blueStyle);

			for(const CCreature * rel : list)
			{
				if(!rel) continue;

				auto rowBg = std::make_shared<GraphicalPrimitiveCanvas>(
					Rect(MARGIN, curY, tableW, rowH));
				rowBg->addRectangle(Point(0,0), Point(tableW, rowH), bdr);
				widgets.push_back(rowBg);

				widgets.push_back(std::make_shared<CAnimImage>(
					AnimationPath::builtin("CPRSMALL"),
					rel->getIconIndex(), 0,
					MARGIN + 2, curY + 2));

				widgets.push_back(std::make_shared<CLabel>(
					MARGIN + iconW + CELL_L, curY + rowH / 2 - 5,
					FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
					rel->getNameSingularTranslated()));

				const std::string relJsonKey = rel->getJsonKey();
				std::function<void()> lclick;
				if(navigateCallback)
					lclick = [navigateCallback, relJsonKey](){ navigateCallback(relJsonKey); };
				const CreatureID relId(rel->getIndex());
				widgets.push_back(std::make_shared<WikiClickable>(
					Rect(MARGIN, curY, tableW, rowH),
					std::move(lclick),
					[relId](){
						ENGINE->windows().createAndPushWindow<CStackWindow>(
							LIBRARY->creh->objects[relId.getNum()].get(), true);
					},
					blueStyle, clipRect));

				curY += rowH + 2;
			}
			curY += GAP - 2;
		};

		addRelatedTable(
			LIBRARY->generaltexth->translate("vcmi.wiki.creature.upgradeOf"),
			bases);
		addRelatedTable(
			LIBRARY->generaltexth->translate("vcmi.wiki.creature.upgradeTo"),
			upgrades);
	}

	// ── 7. Abilities (creature bonuses with icons) ───────────────────────
	{
		struct AbilityRow
		{
			std::string text;
			ImagePath   icon;
		};
		std::vector<AbilityRow> abilityRows;
		std::set<std::tuple<int, int, std::string>> seen;

		auto bonusList = creature->getBonuses(Selector::all);
		for(const auto & b : *bonusList)
		{
			if(b->hidden) continue;
			const std::string text = !b->description.empty()
				? b->description.toString()
				: LIBRARY->bth->bonusToString(b, creature);
			if(text.empty()) continue;
			const auto key = std::make_tuple(static_cast<int>(b->type), b->subtype.getNum(), text);
			if(!seen.insert(key).second) continue;
			const ImagePath icon = !b->customIconPath.empty()
				? b->customIconPath
				: LIBRARY->bth->bonusToGraphics(b);
			abilityRows.push_back(AbilityRow{text, icon});
		}

		if(!abilityRows.empty())
		{
			curY += 16;
			widgets.push_back(std::make_shared<CLabel>(
				W / 2, curY,
				FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
				LIBRARY->generaltexth->translate("vcmi.wiki.creature.section.abilities")));
			curY += 20;

			static constexpr int ICON_SIZE = 51;
			static constexpr int ICON_COL  = ICON_SIZE + 4;
			const int tableW = W - MARGIN * 2;
			const ColorRGBA bdr = wikiBorderColor(blueStyle);

			for(const auto & ab : abilityRows)
			{
				const bool hasIcon = !ab.icon.empty();
				const int  textX   = MARGIN + (hasIcon ? ICON_COL : 0) + CELL_L;
				const int  textW   = tableW - (hasIcon ? ICON_COL : 0) - CELL_L * 2;

				auto textLabel = std::make_shared<CMultiLineLabel>(
					Rect(textX, curY + CELL_T, textW, 4000),
					FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, ab.text);
				const int textH = textLabel->textSize.y;

				const int minH = hasIcon ? (ICON_SIZE + CELL_T * 2) : (CELL_T * 2 + 12);
				const int rowH = std::max(minH, textH + CELL_T * 2);

				textLabel->pos.h = rowH - CELL_T * 2;

				auto rowBorder = std::make_shared<GraphicalPrimitiveCanvas>(
					Rect(MARGIN, curY, tableW, rowH));
				rowBorder->addRectangle(Point(0, 0), Point(tableW, rowH), bdr);
				widgets.push_back(rowBorder);

				if(hasIcon)
					widgets.push_back(std::make_shared<CPicture>(
						ab.icon,
						MARGIN + 2,
						curY + (rowH - ICON_SIZE) / 2));

				widgets.push_back(textLabel);

				const std::string popupText = ab.text;
				widgets.push_back(std::make_shared<WikiClickable>(
					Rect(MARGIN, curY, tableW, rowH),
					nullptr,
					[popupText](){ CRClickPopup::createAndPush(popupText); },
					blueStyle, clipRect));

				curY += rowH + 2;
			}
			curY += GAP - 2;
		}
	}

	// ── Dwellings ─────────────────────────────────────────────────────────────
	{
		// Collect all dwelling handlers that produce this creature
		struct DwellingEntry
		{
			const DwellingInstanceConstructor * handler;
			AnimationPath icon;
		};
		std::vector<DwellingEntry> dwellings;

		static constexpr std::array<int, 4> dwellingObjTypes = {
			Obj::CREATURE_GENERATOR1, Obj::CREATURE_GENERATOR2,
			Obj::CREATURE_GENERATOR3, Obj::CREATURE_GENERATOR4
		};
		for(const int objTypeId : dwellingObjTypes)
		{
			const MapObjectID objType(objTypeId);
			for(const MapObjectSubID subtype : LIBRARY->objtypeh->knownSubObjects(objType))
			{
				auto baseHandler = LIBRARY->objtypeh->getHandlerFor(objType, subtype);
				auto handler = std::dynamic_pointer_cast<const DwellingInstanceConstructor>(baseHandler);
				if(!handler || !handler->producesCreature(creature))
					continue;

				AnimationPath icon = handler->getKingdomOverviewImage();
				if(icon.empty())
				{
					const auto templates = handler->getTemplates();
					if(!templates.empty())
						icon = templates[0]->animationFile;
				}
				dwellings.push_back({handler.get(), icon});
			}
		}

		if(!dwellings.empty())
		{
			curY += 16;
			widgets.push_back(std::make_shared<CLabel>(
				W / 2, curY,
				FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
				LIBRARY->generaltexth->translate("vcmi.wiki.creature.section.dwellings")));
			curY += 20;

			static constexpr int DW_ICON_W = 42;
			static constexpr int DW_ICON_H = 42;

			const int tableW = W - MARGIN * 2;
			const ColorRGBA bdr = wikiBorderColor(blueStyle);

			for(const auto & dw : dwellings)
			{
				const int nameW = tableW - DW_ICON_W - CELL_L * 2;

				auto nameLabel = std::make_shared<CMultiLineLabel>(
					Rect(MARGIN + DW_ICON_W + CELL_L, curY + CELL_T, nameW, 4000),
					FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
					dw.handler->getNameTranslated());
				const int textH = nameLabel->textSize.y;

				const int rowH = std::max(DW_ICON_H + CELL_T * 2, textH + CELL_T * 2);

				nameLabel->pos.h = rowH - CELL_T * 2;

				auto rowBg = std::make_shared<GraphicalPrimitiveCanvas>(
					Rect(MARGIN, curY, tableW, rowH));
				rowBg->addRectangle(Point(0, 0), Point(tableW, rowH), bdr);
				widgets.push_back(rowBg);

				// Dwelling icon
				if(!dw.icon.empty())
					widgets.push_back(std::make_shared<CAnimImage>(
						dw.icon, 0,
						Rect(MARGIN + 2, curY + (rowH - DW_ICON_H) / 2, DW_ICON_W - 4, DW_ICON_H),
						0, CShowableAnim::MAP_OBJECT_MODE));

				widgets.push_back(nameLabel);

				curY += rowH + 2;
			}
			curY += GAP - 2;
		}
	}

	return widgets;
}
