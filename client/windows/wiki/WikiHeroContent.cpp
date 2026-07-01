/*
 * WikiHeroContent.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "WikiHeroContent.h"
#include "WikiCommon.h"

#include "../../widgets/CViewport.h"
#include "../../widgets/Images.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/GraphicalPrimitiveCanvas.h"
#include "../../render/Canvas.h"
#include "../../render/Colors.h"
#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../CHeroOverview.h"
#include "../CCreatureWindow.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/CSkillHandler.h"
#include "../../../lib/spells/CSpellHandler.h"
#include "../../../lib/entities/hero/CHero.h"
#include "../../../lib/entities/hero/CHeroClass.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/entities/hero/CHeroHandler.h"
#include "../../../lib/entities/hero/EHeroGender.h"
#include "../../../lib/entities/artifact/CArtifact.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/constants/Enumerations.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "WikiWindow.h"
#include "../InfoWindows.h"

// ─────────────────────────────────────────────────────────────────────────────
// Layout constants
// ─────────────────────────────────────────────────────────────────────────────

static constexpr int MARGIN = 4;
static constexpr int GAP    = 10;
static constexpr int CELL_L = 4;
static constexpr int CELL_T = 2;

// ─────────────────────────────────────────────────────────────────────────────
// buildHeroContent
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::shared_ptr<CIntObject>> buildHeroContent(
	CViewport & viewport,
	const CHero * hero,
	int viewportWidth,
	bool blueStyle,
	WikiHeroNavigateCallback navigateCallback,
	const CGHeroInstance * mapHero)
{
	std::vector<std::shared_ptr<CIntObject>> widgets;
	if(!hero) return widgets;

	OBJECT_CONSTRUCTION_TARGETED(viewport.content());
	const Rect clipRect = viewport.clipRect();
	const int W   = viewportWidth;
	int       curY = 12;

	const ColorRGBA valCol = blueStyle
		? ColorRGBA(160, 210, 255, 255)
		: Colors::YELLOW;

	// ── 1. Title + class/gender subtitle ───────────────────────────────────
	widgets.push_back(std::make_shared<CLabel>(
		W / 2, curY,
		FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,
		(mapHero && !mapHero->nameCustomTextId.empty()) ? mapHero->getNameTranslated() : hero->getNameTranslated()));
	curY += 26;

	{
		// Gender: ♂/♀ Unicode glyphs if supported by font, else translatable fallback
		const std::string genderSpan = wikiGenderSpan(hero->gender == EHeroGender::FEMALE);

		const std::string className = hero->heroClass ? hero->heroClass->getNameTranslated() : "";

		// Alignment as colored inline span using {#RRGGBB|text} syntax
		std::string alignSpan;
		if(hero->heroClass)
		{
			switch(hero->heroClass->getAlignment())
			{
				case EAlignment::GOOD:    alignSpan = "{#00C800|" + LIBRARY->generaltexth->translate("vcmi.wiki.alignment.good")    + "}"; break;
				case EAlignment::EVIL:    alignSpan = "{#DC3232|" + LIBRARY->generaltexth->translate("vcmi.wiki.alignment.evil")    + "}"; break;
				case EAlignment::NEUTRAL: alignSpan = "{#FFFFFF|" + LIBRARY->generaltexth->translate("vcmi.wiki.alignment.neutral") + "}"; break;
				default: break;
			}
		}

		// Compose "{yellow|ClassName} · {color|♂/♀} · {color|Alignment}"
		std::string subtitle = className.empty() ? genderSpan : (className + " \xc2\xb7 " + genderSpan);
		if(!alignSpan.empty())
			subtitle += " \xc2\xb7 " + alignSpan;

		widgets.push_back(std::make_shared<CLabel>(
			W / 2, curY,
			FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, subtitle));
		curY += 18;
	}

	// ── 2. Large portrait  +  biography side-by-side ────────────────────────
	{
		const int portraitW = 58;  // PortraitsLarge native width
		const int portraitH = 68;  // clickable area height
		const int bioX = MARGIN + portraitW + GAP;
		const int bioW = W - bioX - MARGIN;

		const bool hasCustomData = mapHero && (
			!mapHero->nameCustomTextId.empty() ||
			!mapHero->biographyCustomTextId.empty() ||
			mapHero->customPortraitSource.isValid());

		const int portraitFrame = (mapHero && mapHero->customPortraitSource.isValid())
				? (int)mapHero->getPortraitSource().toHeroType()->getIconIndex()
			: hero->imageIndex;
		widgets.push_back(std::make_shared<CAnimImage>(
			AnimationPath::builtin("PortraitsLarge"),
			portraitFrame, 0,
			MARGIN, curY));

		if(!hasCustomData)
		{
			const HeroTypeID hId = hero->getId();
			widgets.push_back(std::make_shared<WikiClickable>(
				Rect(MARGIN, curY, portraitW, portraitH),
				nullptr,
				[hId](){ ENGINE->windows().createAndPushWindow<CHeroOverview>(hId); },
				blueStyle));
		}

		const std::string bio = (mapHero && !mapHero->biographyCustomTextId.empty())
			? mapHero->getBiographyTranslated()
			: hero->getBiographyTranslated();
		if(!bio.empty())
		{
			auto bioLabel = std::make_shared<CMultiLineLabel>(
				Rect(bioX, curY, bioW, 4000),
				FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, bio);
			bioLabel->pos.h = bioLabel->textSize.y;
			const int blockH = std::max(portraitH, (int)bioLabel->textSize.y);
			curY += blockH + GAP;
			widgets.push_back(std::move(bioLabel));
		}
		else
		{
			curY += portraitH + GAP;
		}
	}

	// ── 3. Primary skills table ─────────────────────────────────────────
	if(hero->heroClass)
	{
		curY += 16;
		const std::vector<std::string> psNames = {
			LIBRARY->generaltexth->jktexts[1],
			LIBRARY->generaltexth->jktexts[2],
			LIBRARY->generaltexth->jktexts[3],
			LIBRARY->generaltexth->jktexts[4],
		};
		const int nStats = static_cast<int>(psNames.size());
		const int tableW = W - MARGIN * 2;
		const int rowH   = 18;
		const int colLbl = tableW / 2;

		widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW, std::vector<int>{colLbl, tableW - colLbl}, 0, rowH, nStats, blueStyle));

		for(int i = 0; i < nStats; ++i)
		{
			const int ry = curY + i * rowH;
			widgets.push_back(std::make_shared<CLabel>(
				MARGIN + CELL_L, ry + CELL_T,
				FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
				psNames[i]));
			const int val = (i < (int)hero->heroClass->primarySkillInitial.size())
				? hero->heroClass->primarySkillInitial[i] : 0;
			widgets.push_back(std::make_shared<CLabel>(
				MARGIN + colLbl + CELL_L, ry + CELL_T,
				FONT_SMALL, ETextAlignment::TOPLEFT, valCol,
				std::to_string(val)));
		}
		curY += nStats * rowH + GAP;
	}

	// ── 4. Specialty ─────────────────────────────────────────────────────
	{
		const std::string specName = hero->getSpecialtyNameTranslated();
		const std::string specDesc = hero->getSpecialtyDescriptionTranslated();
		if(!specName.empty())
		{
			curY += 16;
			widgets.push_back(std::make_shared<CLabel>(
				W / 2, curY,
				FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
				LIBRARY->generaltexth->translate("vcmi.wiki.hero.specialty")));
			curY += 20;

			const int iconCol = 48;
			const int tableW  = W - MARGIN * 2;
			const ColorRGBA bdr = wikiBorderColor(blueStyle);
			auto rowBorder = std::make_shared<GraphicalPrimitiveCanvas>(
				Rect(MARGIN, curY, tableW, 50));
			rowBorder->addRectangle(Point(0, 0), Point(tableW, 50), bdr);
			widgets.push_back(rowBorder);

			widgets.push_back(std::make_shared<CAnimImage>(
				AnimationPath::builtin("UN44"),
				hero->imageIndex, 0,
				MARGIN + 2, curY + 2));
			widgets.push_back(std::make_shared<CLabel>(
				MARGIN + iconCol + CELL_L, curY + CELL_T,
				FONT_MEDIUM, ETextAlignment::TOPLEFT, Colors::YELLOW,
				specName));
			curY += 50;

			if(!specDesc.empty())
			{
				auto label = std::make_shared<CMultiLineLabel>(
					Rect(MARGIN, curY, W - MARGIN * 2, 4000),
					FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, specDesc);
				label->pos.h = label->textSize.y;
				curY += label->textSize.y + GAP;
				widgets.push_back(std::move(label));
			}
			else
			{
				curY += GAP;
			}
		}
	}

	// ── 5. Starting army table ───────────────────────────────────────────
	if(!hero->initialArmy.empty())
	{
		curY += 16;
		widgets.push_back(std::make_shared<CLabel>(
			W / 2, curY,
			FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.heroOverview.startingArmy")));
		curY += 20;

		struct ArmyRow { const CCreature * cr; ui32 minAmt; ui32 maxAmt; };
		std::vector<ArmyRow> armyRows;
		for(const auto & stack : hero->initialArmy)
		{
			if(stack.creature.getNum() < 0 || stack.creature.getNum() >= (int)LIBRARY->creh->objects.size())
				continue;
			const auto & cr = LIBRARY->creh->objects[stack.creature.getNum()];
			if(cr && cr->warMachine == ArtifactID::NONE)
				armyRows.push_back({cr.get(), stack.minAmount, stack.maxAmount});
		}

		if(!armyRows.empty())
		{
			const int iconW   = 36;
			const int tableW  = W - MARGIN * 2;
			const int nameW   = tableW - iconW - 60;
			const std::vector<int> cols = { iconW, nameW, 60 };
			const int headerH = 16;
			const int rowH    = 36;

			widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW, cols,
				headerH, rowH, static_cast<int>(armyRows.size()), blueStyle));

			widgets.push_back(std::make_shared<CLabel>(
				MARGIN + iconW + CELL_L, curY + CELL_T,
				FONT_TINY, ETextAlignment::TOPLEFT, Colors::YELLOW,
				LIBRARY->generaltexth->translate("core.genrltxt.42")));
			widgets.push_back(std::make_shared<CLabel>(
				MARGIN + iconW + nameW + CELL_L, curY + CELL_T,
				FONT_TINY, ETextAlignment::TOPLEFT, Colors::YELLOW,
				LIBRARY->generaltexth->translate("vcmi.wiki.hero.column.amount")));
			curY += headerH;

			for(const auto & ar : armyRows)
			{
				widgets.push_back(std::make_shared<CAnimImage>(
					AnimationPath::builtin("CPRSMALL"),
					ar.cr->getIconIndex(), 0,
					MARGIN + 2, curY + 2));
				widgets.push_back(std::make_shared<CLabel>(
					MARGIN + iconW + CELL_L, curY + rowH / 2 - 5,
					FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
					ar.cr->getNamePluralTranslated()));
				const std::string amt = (ar.minAmt == ar.maxAmt)
					? std::to_string(ar.minAmt)
					: std::to_string(ar.minAmt) + "-" + std::to_string(ar.maxAmt);
				widgets.push_back(std::make_shared<CLabel>(
					MARGIN + iconW + nameW + CELL_L, curY + rowH / 2 - 5,
					FONT_SMALL, ETextAlignment::TOPLEFT, valCol, amt));

				const CreatureID crId(ar.cr->getIndex());
				std::function<void()> lclick;
				if(navigateCallback)
				{
					const std::string crKey = ar.cr->getJsonKey();
					lclick = [navigateCallback, crKey](){ navigateCallback(WikiCategory::CREATURE, crKey); };
				}
				widgets.push_back(std::make_shared<WikiClickable>(
					Rect(MARGIN, curY, tableW, rowH),
					std::move(lclick),
					[crId](){
						ENGINE->windows().createAndPushWindow<CStackWindow>(
							LIBRARY->creh->objects[crId.getNum()].get(), true);
					},
					blueStyle, clipRect));
				curY += rowH;
			}
			curY += GAP;
		}
	}

	// ── 5.5. War machines ───────────────────────────────────────────────
	{
		// Like CHeroOverview: always show catapult + any war machines from initialArmy.
		struct WMRow { const CCreature * cr; };
		std::vector<WMRow> wmRows;

		// Slot 0: catapult is always present
		const CreatureID catapultId = CreatureID::CATAPULT;
		if(catapultId.getNum() >= 0 && catapultId.getNum() < (int)LIBRARY->creh->objects.size())
		{
			const auto & cr = LIBRARY->creh->objects[catapultId.getNum()];
			if(cr) wmRows.push_back({cr.get()});
		}

		// Additional war machines from hero's starting army
		for(const auto & stack : hero->initialArmy)
		{
			if(stack.creature.getNum() < 0 || stack.creature.getNum() >= (int)LIBRARY->creh->objects.size())
				continue;
			const auto & cr = LIBRARY->creh->objects[stack.creature.getNum()];
			if(cr && cr->warMachine != ArtifactID::NONE)
				wmRows.push_back({cr.get()});
		}

		if(!wmRows.empty())
		{
			curY += 16;
			widgets.push_back(std::make_shared<CLabel>(
				W / 2, curY,
				FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
				LIBRARY->generaltexth->translate("vcmi.heroOverview.warMachine")));
			curY += 20;

			const int iconW  = 36;
			const int tableW = W - MARGIN * 2;
			const int nameW  = tableW - iconW;
			const std::vector<int> cols = { iconW, nameW };
			const int rowH   = 36;

			widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW, cols,
				0, rowH, static_cast<int>(wmRows.size()), blueStyle));

			for(const auto & wr : wmRows)
			{
				widgets.push_back(std::make_shared<CAnimImage>(
					AnimationPath::builtin("CPRSMALL"),
					wr.cr->getIconIndex(), 0,
					MARGIN + 2, curY + 2));
				widgets.push_back(std::make_shared<CLabel>(
					MARGIN + iconW + CELL_L, curY + rowH / 2 - 5,
					FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
					wr.cr->getNameSingularTranslated()));

				const CCreature * crPtr = wr.cr;
				std::function<void()> lclick;
				if(navigateCallback)
				{
					const std::string crKey = crPtr->getJsonKey();
					lclick = [navigateCallback, crKey](){ navigateCallback(WikiCategory::CREATURE, crKey); };
				}
				widgets.push_back(std::make_shared<WikiClickable>(
					Rect(MARGIN, curY, tableW, rowH),
					std::move(lclick),
					[crPtr](){ ENGINE->windows().createAndPushWindow<CStackWindow>(crPtr, true); },
					blueStyle, clipRect));
				curY += rowH;
			}
			curY += GAP;
		}
	}

	// ── 6. Secondary skills ──────────────────────────────────────────────
	if(!hero->secSkillsInit.empty())
	{
		curY += 16;
		widgets.push_back(std::make_shared<CLabel>(
			W / 2, curY,
			FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.heroOverview.secondarySkills")));
		curY += 20;

		const int iconW  = 36;
		const int tableW = W - MARGIN * 2;
		const int nameW  = tableW - iconW - 80;
		const std::vector<int> cols = { iconW, nameW, 80 };
		const int headerH = 16;
		const int rowH    = 36;
		const int n       = static_cast<int>(hero->secSkillsInit.size());

		widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW, cols,
			headerH, rowH, n, blueStyle));
		widgets.push_back(std::make_shared<CLabel>(
			MARGIN + iconW + CELL_L, curY + CELL_T,
			FONT_TINY, ETextAlignment::TOPLEFT, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.wiki.hero.column.skill")));
		widgets.push_back(std::make_shared<CLabel>(
			MARGIN + iconW + nameW + CELL_L, curY + CELL_T,
			FONT_TINY, ETextAlignment::TOPLEFT, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.wiki.hero.column.level")));
		curY += headerH;

		for(const auto & [skillId, level] : hero->secSkillsInit)
		{
			if(skillId.getNum() < 0 || skillId.getNum() >= (int)LIBRARY->skillh->objects.size())
				{ curY += rowH; continue; }
			const auto & sk = LIBRARY->skillh->objects[skillId.getNum()];
			if(!sk) { curY += rowH; continue; }

			const size_t frame = static_cast<size_t>(sk->getIconIndex(static_cast<uint8_t>(level - 1)));
			widgets.push_back(std::make_shared<CAnimImage>(
				AnimationPath::builtin("SECSK32"), frame, 0,
				MARGIN + 2, curY + 2));
			widgets.push_back(std::make_shared<CLabel>(
				MARGIN + iconW + CELL_L, curY + rowH / 2 - 5,
				FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
				sk->getNameTranslated()));
			const std::string lvlStr = (level >= 1 && level <= 3)
				? LIBRARY->generaltexth->levels[level - 1]
				: std::to_string(level);
			widgets.push_back(std::make_shared<CLabel>(
				MARGIN + iconW + nameW + CELL_L, curY + rowH / 2 - 5,
				FONT_SMALL, ETextAlignment::TOPLEFT, valCol, lvlStr));

			const CSkill * skPtr = sk.get();
			const int skLvl = level;
			widgets.push_back(std::make_shared<WikiClickable>(
				Rect(MARGIN, curY, tableW, rowH),
				nullptr,
				[skPtr, skLvl](){
					CRClickPopup::createAndPush(skPtr->getDescriptionTranslated(skLvl));
				},
				blueStyle, clipRect));
			curY += rowH;
		}
		curY += GAP;
	}

	// ── 7. Starting spells (always shown) ──────────────────────────────
	{
		curY += 16;
		widgets.push_back(std::make_shared<CLabel>(
			W / 2, curY,
			FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.heroOverview.spells")));
		curY += 20;

		if(!hero->haveSpellBook)
		{
			const int tableW = W - MARGIN * 2;
			const int rowH   = 28;
			widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW, std::vector<int>{tableW}, 0, rowH, 1, blueStyle));
			widgets.push_back(std::make_shared<CLabel>(
				MARGIN + tableW / 2, curY + rowH / 2 - 5,
				FONT_SMALL, ETextAlignment::TOPCENTER, Colors::WHITE,
				LIBRARY->generaltexth->translate("vcmi.wiki.hero.noSpellBook")));
			curY += rowH + GAP;
		}
		else
		{
			std::vector<const CSpell *> validSpells;
			for(const SpellID & sid : hero->spells)
			{
				if(sid.getNum() < 0 || sid.getNum() >= (int)LIBRARY->spellh->objects.size()) continue;
				const auto & sp = LIBRARY->spellh->objects[sid.getNum()];
				if(sp) validSpells.push_back(sp.get());
			}

			if(!validSpells.empty())
			{
			const int iconW   = 36;
			const int tableW  = W - MARGIN * 2;
			const int nameW   = tableW - iconW;
			const std::vector<int> cols = { iconW, nameW };
			const int headerH = 16;
			const int rowH    = 36;

			widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW, cols,
				headerH, rowH, static_cast<int>(validSpells.size()), blueStyle));
			widgets.push_back(std::make_shared<CLabel>(
				MARGIN + iconW + CELL_L, curY + CELL_T,
				FONT_TINY, ETextAlignment::TOPLEFT, Colors::YELLOW,
				LIBRARY->generaltexth->translate("vcmi.wiki.hero.column.spell")));
			curY += headerH;

			for(const CSpell * sp : validSpells)
			{
				widgets.push_back(std::make_shared<CAnimImage>(
					AnimationPath::builtin("SPELLBON"),
					static_cast<size_t>(sp->getIconIndex()),
					Rect(MARGIN + 2, curY + 2, 32, 32), 0));
				widgets.push_back(std::make_shared<CLabel>(
					MARGIN + iconW + CELL_L, curY + rowH / 2 - 5,
					FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
					sp->getNameTranslated()));

				std::function<void()> lclick;
				if(navigateCallback)
				{
					const std::string spId = sp->getJsonKey();
					lclick = [navigateCallback, spId](){ navigateCallback(WikiCategory::SPELL, spId); };
				}
				const CSpell * spPtr = sp;
				widgets.push_back(std::make_shared<WikiClickable>(
					Rect(MARGIN, curY, tableW, rowH),
					std::move(lclick),
					[spPtr](){
						CRClickPopup::createAndPush(spPtr->getDescriptionTranslated(1));
					},
					blueStyle, clipRect));
				curY += rowH;
			}
				curY += GAP;
			}
			else
			{
				// Has spellbook but no starting spells
				curY += GAP;
			}
		}
	}

	return widgets;
}
