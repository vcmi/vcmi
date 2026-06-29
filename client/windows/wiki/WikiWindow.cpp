/*
 * WikiWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "WikiWindow.h"
#include "WikiTownContent.h"
#include "WikiCreatureContent.h"
#include "WikiHeroContent.h"
#include "WikiArtifactContent.h"
#include "WikiModContent.h"
#include "WikiMarkdown.h"

#include "../../gui/Shortcut.h"
#include "../../gui/WindowHandler.h"
#include "../../GameEngine.h"
#include "../../widgets/Buttons.h"
#include "../../widgets/CViewport.h"
#include "../../widgets/GraphicalPrimitiveCanvas.h"
#include "../../widgets/Images.h"
#include "../../widgets/ObjectLists.h"
#include "../../widgets/Slider.h"
#include "../../widgets/TextControls.h"
#include "../../render/Canvas.h"
#include "../../render/Colors.h"
#include "../../render/IImage.h"
#include "../../render/IRenderHandler.h"
#include "../../widgets/CTextInput.h"
#include "../InfoWindows.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/entities/faction/CFaction.h"
#include "../../../lib/entities/faction/CTownHandler.h"
#include "../../../lib/entities/faction/CTown.h"
#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/entities/hero/CHeroHandler.h"
#include "../../../lib/entities/hero/CHeroClass.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/spells/CSpellHandler.h"
#include "../../../lib/spells/SpellSchoolHandler.h"
#include "../../../lib/CSkillHandler.h"
#include "../../../lib/TerrainHandler.h"
#include "../../../lib/modding/CModHandler.h"
#include "../../../lib/modding/ModDescription.h"
#include "../../../lib/texts/TextOperations.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../../../lib/json/JsonNode.h"
#include "../../../lib/CConfigHandler.h"
#include "../../../lib/json/JsonUtils.h"

// ============================================================================
// Built-in category string-id → WikiCategory mapping
// Used for both validation (reserved names) and link-callback resolution.
// ============================================================================

static const std::map<std::string, WikiCategory> BUILTIN_CATEGORY_MAP = // NOSONAR (std::map cannot be constexpr)
{
	{"glossary", WikiCategory::GLOSSARY},
	{"town",     WikiCategory::TOWN},
	{"hero",     WikiCategory::HERO},
	{"creature", WikiCategory::CREATURE},
	{"artifact", WikiCategory::ARTIFACT},
	{"spell",    WikiCategory::SPELL},
	{"skill",    WikiCategory::SKILL},
	{"terrain",  WikiCategory::TERRAIN},
	{"mod",      WikiCategory::MOD},
};

// ============================================================================
// WikiListItem
// ============================================================================

WikiListItem::WikiListItem(size_t itemIndex, std::string itemText, std::string itemSubtitle, // NOSONAR (8 params required for widget configuration)
	std::function<void(WikiListItem *)> callback,
	std::optional<WikiIconInfo> iconInfo,
	bool blueStyle_,
	int itemWidth,
	bool hasSlider)
	: onSelected(std::move(callback))
	, text(std::move(itemText))
	, index(itemIndex)
	, blueStyle(blueStyle_)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(LCLICK | HOVER | SHOW_POPUP);

	sublabelColor = blueStyle_
		? ColorRGBA(100, 160, 240, 255)
		: ColorRGBA(158, 130, 105, 255);

	int labelOffsetX = MARGIN_L;

	if(iconInfo)
	{
		// Query native image size to compute aspect-correct rendered height and center it.
		auto refImg = ENGINE->renderHandler().loadImage(
			iconInfo->path, static_cast<int>(iconInfo->frame), static_cast<int>(iconInfo->group), EImageBlitMode::COLORKEY);
		const Point nativeSz = refImg ? refImg->dimensions() : Point(ICON_SIZE, ICON_SIZE);
		// "Contain" scaling: fit the larger dimension to ICON_SIZE, preserve aspect.
		int iconW2, iconH2;
		if(nativeSz.x <= 0 || nativeSz.y <= 0)
		{
			iconW2 = ICON_SIZE; iconH2 = ICON_SIZE;
		}
		else if(nativeSz.y > nativeSz.x)
		{
			// Portrait: height is the larger dimension
			iconH2 = ICON_SIZE;
			iconW2 = std::max(1, ICON_SIZE * nativeSz.x / nativeSz.y);
		}
		else
		{
			// Landscape or square
			iconW2 = ICON_SIZE;
			iconH2 = std::max(1, ICON_SIZE * nativeSz.y / nativeSz.x);
		}
		const int iconX2 = MARGIN_L + (ICON_SIZE - iconW2) / 2;
		const int iconY2 = (ITEM_H - iconH2) / 2;
		icon = std::make_shared<CAnimImage>(
			iconInfo->path,
			iconInfo->frame,
			Rect(iconX2, iconY2, iconW2, iconH2),
			iconInfo->group);
		labelOffsetX = MARGIN_L + ICON_SIZE + 5;
	}

	// itemWidth = full item pos.w (slider area included).
	// For text centering/wrapping we use effectiveW = itemWidth minus slider when active.
	static constexpr int SLIDER_W_LOCAL = 16;
	const int effectiveW = (!iconInfo && hasSlider) ? itemWidth - SLIDER_W_LOCAL : itemWidth;
	const ETextAlignment hAlign = iconInfo ? ETextAlignment::CENTERLEFT : ETextAlignment::CENTER;
	const int labelRectX = iconInfo ? labelOffsetX : 0;
	const int labelAnchorX = iconInfo ? labelOffsetX : (effectiveW / 2);
	const int labelW = std::max(10, effectiveW - labelRectX - (iconInfo ? MARGIN_L : 0));

	if(!itemSubtitle.empty())
	{
		// Two-line layout: name centred in the top half, subtitle in the bottom half.
		label = std::make_shared<CLabel>(
			labelAnchorX, ITEM_H / 4,
			FONT_SMALL, hAlign, Colors::WHITE, text);
		sublabel = std::make_shared<CLabel>(
			labelAnchorX, ITEM_H / 4 * 3,
			FONT_SMALL, hAlign, sublabelColor, itemSubtitle);
	}
	else
	{
		// Single entry: CMultiLineLabel so long names can wrap.
		// Vertically centred via y-offset of 3 (height = ITEM_H - 6, 3 px margin each side).
		label = std::make_shared<CMultiLineLabel>(
			Rect(labelRectX, 3, labelW, ITEM_H - 6),
			FONT_SMALL, hAlign, Colors::WHITE, text);
	}
}

void WikiListItem::updateLook()
{
	label->setColor(selected ? Colors::YELLOW : Colors::WHITE);
	// sublabel keeps its characteristic hint colour at all times
	redraw();
}

void WikiListItem::setSelected(bool sel)
{
	selected = sel;
	updateLook();
}

void WikiListItem::clickPressed(const Point & cursorPosition)
{
	if(text.empty())
		return;
	// Ignore clicks that land on the scrollbar area
	if(parent)
	{
		auto * lb = dynamic_cast<CListBox *>(parent);
		if(lb && lb->getSlider() && !lb->getSlider()->isDisabled())
		{
			const int sliderW = lb->getSlider()->pos.w;
			if(cursorPosition.x >= parent->pos.x + parent->pos.w - sliderW)
				return;
		}
	}
	if(onSelected)
		onSelected(this);
}

void WikiListItem::showPopupWindow(const Point & cursorPosition)
{
	if(!text.empty())
	{
		// Ignore right-clicks on the scrollbar area
		if(parent)
		{
			auto * lb = dynamic_cast<CListBox *>(parent);
			if(lb && lb->getSlider() && !lb->getSlider()->isDisabled())
			{
				const int sliderW = lb->getSlider()->pos.w;
				if(cursorPosition.x >= parent->pos.x + parent->pos.w - sliderW)
					return;
			}
		}
		CRClickPopup::createAndPush(text);
	}
}

void WikiListItem::hover(bool on)
{
	if(text.empty())
		return;
	if(!selected)
	{
		label->setColor(on ? ColorRGBA(255, 220, 120, 255) : Colors::WHITE);
		// sublabel keeps its characteristic hint colour on hover too
		redraw();
	}
}

void WikiListItem::showAll(Canvas & to) // NOSONAR
{
	// Clip to parent (CListBox) bounds so items never spill into adjacent columns.
	// Exclude the slider area so text / icons do not overdraw the scrollbar.
	if(parent)
	{
		Rect clipArea = parent->pos;
		auto * lb = dynamic_cast<CListBox *>(parent);
		if(lb && lb->getSlider())
			clipArea.w -= 16; // slider width
		CanvasClipRectGuard guard(to, clipArea);

		if(selected && !text.empty())
		{
			const int contentW = (lb && lb->getSlider()) ? (pos.w - 16) : pos.w;
			if(contentW > 2 && pos.h > 2)
			{
				to.drawColorBlended(Rect(pos.x + 1, pos.y + 1, contentW - 2, pos.h - 2), ColorRGBA(0, 0, 0, 110));
				to.drawBorder(Rect(pos.x + 1, pos.y + 1, contentW - 2, pos.h - 2), ColorRGBA(210, 170, 70, 255), 1);
			}
		}
		// Separator line: full width without slider
		if(pos.y + pos.h < parent->pos.y + parent->pos.h)
		{
			const int sepW = (lb && lb->getSlider()) ? (pos.w - 16) : pos.w;
			to.drawColorBlended(Rect(pos.x, pos.y + pos.h - 1, sepW, 1),
			                    blueStyle ? ColorRGBA(40, 100, 200, 200) : ColorRGBA(100, 80, 55, 255));
		}
		CIntObject::showAll(to);
	}
	else
	{
		CIntObject::showAll(to);
	}
}

// ============================================================================
// WikiWindow – layout constants
// ============================================================================

// Window dimensions
static constexpr int WIN_W = 800;
static constexpr int WIN_H = 600;

// Y coordinates
static constexpr int TITLE_Y       = 14;
static constexpr int HEADER_Y      = 40;
static constexpr int CONTENT_TOP   = 58;
// CONTENT_BOT chosen so CONTENT_H is an exact multiple of ITEM_H (no half-rows)
static constexpr int CONTENT_BOT   = 538;                          // 58 + 480
static constexpr int CONTENT_H     = CONTENT_BOT - CONTENT_TOP;    // 480 = 12 * 40

// Row height – must match WikiListItem::ITEM_H
static_assert(WikiListItem::ITEM_H == 40, "ITEM_H mismatch");
static constexpr int ITEM_H        = WikiListItem::ITEM_H;
static constexpr int VISIBLE_ITEMS = CONTENT_H / ITEM_H; // 12

// Element column uses one row less to make room for the search box
static constexpr int ELEM_VISIBLE_ITEMS = VISIBLE_ITEMS - 1;        // 11
static constexpr int ELEM_LIST_H        = ELEM_VISIBLE_ITEMS * ITEM_H; // 440

// Search box placed directly below the element list – flush, fills the remaining row
static constexpr int SEARCH_BOX_Y = CONTENT_TOP + ELEM_LIST_H;       // 498
static constexpr int SEARCH_BOX_H = CONTENT_BOT - SEARCH_BOX_Y;      // 40 = ITEM_H

// Slider width
static constexpr int SLIDER_W      = 16;

// Column 1 – Categories  (x: 10 … 106)  ~60% of original
static constexpr int COL1_X        = 10;
static constexpr int COL1_LIST_W   = 80;   // ~60% of previous 138

// Column 2 – Elements + search  (x: 108 … ~269)  reduced to ~65 % of previous
static constexpr int COL2_X        = 108;  // = COL1_X + COL1_LIST_W + SLIDER_W + 2
static constexpr int COL2_LIST_W   = 144;  // ~65 % of previous 222 – gives more room to col3

// Column 3 – Content  (x: 270 … 790)
static constexpr int COL3_X        = COL2_X + COL2_LIST_W + SLIDER_W + 2; // 270
static constexpr int COL3_W        = WIN_W - COL3_X - 10; // 520 incl. textbox slider

// Close button (IOKAY.DEF is ~52 × 28 px)
static constexpr int CLOSE_Y       = 564;

// ============================================================================
// WikiWindow – constructor
// ============================================================================

WikiWindow::WikiWindow(WikiWindow::Style style_, std::optional<WikiEntryKey> initialEntry) // NOSONAR
	: CWindowObject(BORDERED)
	, style(style_)
{
	OBJECT_CONSTRUCTION;

	// Resize and centre
	pos = Rect(pos.x, pos.y, WIN_W, WIN_H);
	if(style == Style::BLUE)
	{
		auto blueBg = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, WIN_W, WIN_H));
		blueBg->setPlayerColor(PlayerColor(1));
		bgTexture = blueBg;
	}
	else
	{
		bgTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(0, 0, WIN_W, WIN_H));
	}
	updateShadow();
	center();

	// ---- Column background tints ----------------------------------------
	const ColorRGBA borderColor = (style == Style::BLUE)
	                            ? ColorRGBA(32,  96, 192, 220)   // steel-blue
	                            : ColorRGBA(96,  72,  32, 200);  // gold-brown
	const ColorRGBA fillColor   = (style == Style::BLUE)
	                            ? ColorRGBA( 0,  16,  48,  40)
	                            : ColorRGBA( 0,   0,   0,  40);
	const ColorRGBA fillColor3  = (style == Style::BLUE)
	                            ? ColorRGBA( 0,  16,  48,  60)
	                            : ColorRGBA( 0,   0,   0,  60);

	col1Bg = std::make_shared<TransparentFilledRectangle>(
		Rect(COL1_X, CONTENT_TOP, COL1_LIST_W + SLIDER_W + 1, CONTENT_H),
		fillColor, borderColor);

	col2Bg = std::make_shared<TransparentFilledRectangle>(
		Rect(COL2_X, CONTENT_TOP, COL2_LIST_W + SLIDER_W + 1, CONTENT_H),
		fillColor, borderColor);

	col3Bg = std::make_shared<TransparentFilledRectangle>(
		Rect(COL3_X, CONTENT_TOP, COL3_W, CONTENT_H),
		fillColor3, borderColor);

	// ---- Title --------------------------------------------------------------
	titleLabel = std::make_shared<CLabel>(
		WIN_W / 2, TITLE_Y,
		FONT_BIG, ETextAlignment::CENTER,
		Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.wiki.title"));

	// ---- Column headers -----------------------------------------------------
	col1Header = std::make_shared<CLabel>(
		COL1_X + (COL1_LIST_W + SLIDER_W) / 2, HEADER_Y,
		FONT_MEDIUM, ETextAlignment::CENTER,
		Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.wiki.header.category"));

	col2Header = std::make_shared<CLabel>(
		COL2_X + (COL2_LIST_W + SLIDER_W) / 2, HEADER_Y,
		FONT_MEDIUM, ETextAlignment::CENTER,
		Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.wiki.header.entry"));

	col3Header = std::make_shared<CLabel>(
		COL3_X + COL3_W / 2, HEADER_Y,
		FONT_MEDIUM, ETextAlignment::CENTER,
		Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.wiki.header.information"));

	// ---- Game data from VCMI library ----------------------------------------
	categoryNames.resize(static_cast<int>(WikiCategory::COUNT));
	categoryNames[static_cast<int>(WikiCategory::GLOSSARY)]  = LIBRARY->generaltexth->translate("vcmi.wiki.category.glossary");
	categoryNames[static_cast<int>(WikiCategory::TOWN)]      = LIBRARY->generaltexth->translate("vcmi.wiki.category.town");
	categoryNames[static_cast<int>(WikiCategory::HERO)]      = LIBRARY->generaltexth->translate("vcmi.wiki.category.hero");
	categoryNames[static_cast<int>(WikiCategory::CREATURE)]  = LIBRARY->generaltexth->translate("vcmi.wiki.category.creature");
	categoryNames[static_cast<int>(WikiCategory::ARTIFACT)]  = LIBRARY->generaltexth->translate("vcmi.wiki.category.artifact");
	categoryNames[static_cast<int>(WikiCategory::SPELL)]     = LIBRARY->generaltexth->translate("vcmi.wiki.category.spell");
	categoryNames[static_cast<int>(WikiCategory::SKILL)]     = LIBRARY->generaltexth->translate("vcmi.wiki.category.skill");
	categoryNames[static_cast<int>(WikiCategory::TERRAIN)]   = LIBRARY->generaltexth->translate("vcmi.wiki.category.terrain");
	categoryNames[static_cast<int>(WikiCategory::MOD)]       = LIBRARY->generaltexth->translate("vcmi.wiki.category.mod");
	categoryEntries.resize(static_cast<int>(WikiCategory::COUNT));

	// Glossary + custom categories – loaded from all active mods via assembleFromFiles.
	// "entries"    -> goes into the built-in GLOSSARY category (default when no "category" key).
	// "categories" -> defines additional mod-specific categories, each rendered as Markdown.
	{
		const JsonNode glossaryJson = JsonUtils::assembleFromFiles("config/wikiGlossary.json");

		// 1. Register custom categories declared by mods.
		for(const auto & [catId, catNode] : glossaryJson["categories"].Struct())
		{
			if(BUILTIN_CATEGORY_MAP.count(catId))
			{
				logGlobal->error("WikiWindow: mod-defined category '{}' conflicts with a built-in category – skipped.", catId);
				continue;
			}
			if(customCategoryIds.count(catId))
				continue; // already registered by an earlier mod

			const auto idx = static_cast<int>(categoryNames.size());
			const std::string name = LIBRARY->generaltexth->translate(catNode["name"].String());
			categoryNames.push_back(name);
			categoryEntries.emplace_back();
			customCategoryIds[catId] = idx;
		}

		// 2. Load entries; route each to its category (default: GLOSSARY).
		// Custom-category entries are collected with their optional "order" field so they
		// can be reordered after the loop (JsonMap iterates alphabetically by key).
		// key = category index, value = list of (order, entry)
		std::map<int, std::vector<std::pair<int, WikiEntry>>> customOrderedEntries;

		for(const auto & [entryId, e] : glossaryJson["entries"].Struct())
		{
			const auto catStr = e["category"].isNull() ? std::string("glossary") : e["category"].String();
			auto targetIdx = static_cast<int>(WikiCategory::GLOSSARY);
			if(catStr != "glossary")
			{
				const auto it = customCategoryIds.find(catStr);
				if(it != customCategoryIds.end())
					targetIdx = it->second;
				else
					logGlobal->warn("WikiWindow: entry '{}' references unknown category '{}' – placed in Glossary.", entryId, catStr);
			}
			const std::string name = LIBRARY->generaltexth->translate(e["name"].String());
			const std::string desc = LIBRARY->generaltexth->translate(e["description"].String());
			WikiEntry entry{ entryId, name, desc, std::nullopt, "", "" };

			if(targetIdx == static_cast<int>(WikiCategory::GLOSSARY))
			{
				categoryEntries[targetIdx].push_back(std::move(entry));
			}
			else
			{
				// Custom category: collect with order value (default INT_MAX keeps alphabetical fallback)
				const int order = e["order"].isNull() ? 999999 : static_cast<int>(e["order"].Integer());
				customOrderedEntries[targetIdx].emplace_back(order, std::move(entry));
			}
		}

		// Insert custom-category entries sorted by their "order" field
		for(auto & [catIdx, orderedList] : customOrderedEntries)
		{
			std::stable_sort(orderedList.begin(), orderedList.end(),
			                 [](const auto & a, const auto & b){ return a.first < b.first; });
			for(auto & [ord, entry] : orderedList)
				categoryEntries[catIdx].push_back(std::move(entry));
		}
	}
	// Sort the glossary alphabetically; custom categories keep their "order" field sequence.
	{
		const auto iGlossary = static_cast<int>(WikiCategory::GLOSSARY);
		std::sort(categoryEntries[iGlossary].begin(), categoryEntries[iGlossary].end(),
		          [](const WikiEntry & a, const WikiEntry & b){ return a.name < b.name; });
	}

	// Towns – playable factions that have a town (skip neutral / special)
	{
		const int iTown = static_cast<int>(WikiCategory::TOWN);
		for(const auto & faction : LIBRARY->townh->objects)
		{
			if(!faction || !faction->hasTown() || faction->special) continue;
			std::string alignStr;
			switch(faction->getAlignment())
			{
				case EAlignment::GOOD:    alignStr = LIBRARY->generaltexth->translate("vcmi.wiki.alignment.good");    break;
				case EAlignment::EVIL:    alignStr = LIBRARY->generaltexth->translate("vcmi.wiki.alignment.evil");    break;
				case EAlignment::NEUTRAL: alignStr = LIBRARY->generaltexth->translate("vcmi.wiki.alignment.neutral"); break;
				default: break;
			}
			categoryEntries[iTown].push_back({
				faction->getJsonKey(), faction->getNameTranslated(), "",
				WikiIconInfo{ AnimationPath::builtin("ITPA"), static_cast<size_t>(faction->town->clientInfo.icons[1][0]) + 2, 0 },
				faction->getModScope(),
				alignStr
			});
		}
		std::sort(categoryEntries[iTown].begin(), categoryEntries[iTown].end(),
		          [](const WikiEntry & a, const WikiEntry & b){ return a.name < b.name; });
	}

	// Hero types – skip special (campaign-only) heroes
	{
		const int iHero = static_cast<int>(WikiCategory::HERO);
		for(const auto & hero : LIBRARY->heroh->objects)
		{
			if(!hero || hero->special) continue;
			const std::string heroClassName = hero->heroClass ? hero->heroClass->getNameTranslated() : "";
			categoryEntries[iHero].push_back({
				hero->getJsonKey(), hero->getNameTranslated(), "",
				WikiIconInfo{ AnimationPath::builtin("PortraitsSmall"), static_cast<size_t>(hero->getIconIndex()), 0 },
				hero->getModScope(),
				heroClassName
			});
		}
		std::sort(categoryEntries[iHero].begin(), categoryEntries[iHero].end(),
		          [](const WikiEntry & a, const WikiEntry & b){ return a.name < b.name; });
	}

	// Build war-machine creature set so we can include them under Creatures
	std::set<CreatureID> warMachineCreatures;
	for(const auto & art : LIBRARY->arth->objects)
		if(art && art->getWarMachine() != CreatureID::NONE)
			warMachineCreatures.insert(art->getWarMachine());

	// Build faction name lookup for creature subtitles
	std::map<FactionID, std::string> factionNameById;
	for(const auto & faction : LIBRARY->townh->objects)
		if(faction)
			factionNameById[faction->getId()] = faction->getNameTranslated();

	// Creatures – normal creatures plus war machines (always show war machines)
	{
		const int iCreature = static_cast<int>(WikiCategory::CREATURE);
		for(const auto & creature : LIBRARY->creh->objects)
		{
			if(!creature) continue;
			const bool isWM = warMachineCreatures.count(CreatureID(creature->getIndex())) > 0;
			if(!creature->special || isWM)
			{
				const auto it = factionNameById.find(creature->getFactionID());
				const std::string factionName = (it != factionNameById.end()) ? it->second : "";
				categoryEntries[iCreature].push_back({
					creature->getJsonKey(), creature->getNameSingularTranslated(), "",
					WikiIconInfo{ AnimationPath::builtin("CPRSMALL"), static_cast<size_t>(creature->getIconIndex()), 0 },
					creature->getModScope(),
					factionName
				});
			}
		}
		std::sort(categoryEntries[iCreature].begin(), categoryEntries[iCreature].end(),
		          [](const WikiEntry & a, const WikiEntry & b){ return a.name < b.name; });
	}

	// Artifacts – exclude "special" class
	{
		const int iArtifact = static_cast<int>(WikiCategory::ARTIFACT);
		for(const auto & artifact : LIBRARY->arth->objects)
			if(artifact && artifact->aClass != EArtifactClass::ART_SPECIAL)
				categoryEntries[iArtifact].push_back({
					artifact->getJsonKey(),
					artifact->getNameTranslated(),
					artifact->getDescriptionTranslated(),
					WikiIconInfo{ AnimationPath::builtin("Artifact"), static_cast<size_t>(artifact->getIconIndex()), 0 },
					artifact->getModScope(), ""
				});
		std::sort(categoryEntries[iArtifact].begin(), categoryEntries[iArtifact].end(),
		          [](const WikiEntry & a, const WikiEntry & b){ return a.name < b.name; });
	}

	// Spells – exclude hero specials and creature abilities
	{
		const int iSpell = static_cast<int>(WikiCategory::SPELL);
		for(const auto & spell : LIBRARY->spellh->objects)
			if(spell && !spell->isSpecial() && !spell->isCreatureAbility())
			{
				// Build multi-level description (Basic / Advanced / Expert)
				std::string desc;
				// School(s) this spell belongs to
				{
					std::string schools;
					spell->forEachSchool([&schools](const SpellSchool & schoolId, bool &)
					{
						const auto * schoolObj = LIBRARY->spellSchoolHandler->getById(schoolId);
						if(schoolObj)
						{
							if(!schools.empty()) schools += ", ";
							schools += schoolObj->getNameTranslated();
						}
					});
					if(!schools.empty())
						desc = "{" + LIBRARY->generaltexth->translate("vcmi.wiki.spell.schools") + "}: " + schools;
				}
				for(int lvl = 1; lvl <= 3; lvl++)
				{
					const std::string ld = spell->getDescriptionTranslated(lvl);
					if(!ld.empty())
					{
						if(!desc.empty()) desc += "\n\n";
						desc += ld;
					}
				}
				categoryEntries[iSpell].push_back({
					spell->getJsonKey(),
					spell->getNameTranslated(), desc,
					WikiIconInfo{ AnimationPath::builtin("SpellInt"), static_cast<size_t>(spell->getIndex()) + 1, 0 },
					spell->getModScope(), ""
				});
			}
		std::sort(categoryEntries[iSpell].begin(), categoryEntries[iSpell].end(),
		          [](const WikiEntry & a, const WikiEntry & b){ return a.name < b.name; });
	}

	// Secondary skills
	{
		const int iSkill = static_cast<int>(WikiCategory::SKILL);
		for(const auto & skill : LIBRARY->skillh->objects)
			if(skill)
			{
				// Build multi-level description (Basic / Advanced / Expert)
				std::string desc;
				for(int lvl = 1; lvl <= 3; lvl++)
				{
					const std::string ld = skill->getDescriptionTranslated(lvl);
					if(!ld.empty())
					{
						if(!desc.empty()) desc += "\n\n";
						desc += ld;
					}
				}
				categoryEntries[iSkill].push_back({
					skill->getJsonKey(),
					skill->getNameTranslated(), desc,
					WikiIconInfo{ AnimationPath::builtin("SECSK32"), static_cast<size_t>(skill->getIndex() * 3 + 3), 0 },
					skill->getModScope(), ""
				});
			}
		std::sort(categoryEntries[iSkill].begin(), categoryEntries[iSkill].end(),
		          [](const WikiEntry & a, const WikiEntry & b){ return a.name < b.name; });
	}

	// Terrain types – tile image (or minimap colour fallback) as icon, list native towns as description
	{
		const int iTerrain = static_cast<int>(WikiCategory::TERRAIN);
		for(const auto & terrain : LIBRARY->terrainTypeHandler->objects)
			if(terrain)
			{
				WikiIconInfo colorIcon = terrainIconInfo(terrain.get());

				// List towns whose native terrain this is
				std::string nativeTowns;
				for(const auto & faction : LIBRARY->townh->objects)
				{
					if(faction && faction->hasTown() && !faction->special
						&& faction->getNativeTerrain() == terrain->getId())
					{
						if(!nativeTowns.empty()) nativeTowns += ", ";
						nativeTowns += faction->getNameTranslated();
					}
				}
				std::string desc;
				// Movement cost
				{
					const std::string label = LIBRARY->generaltexth->translate("vcmi.wiki.terrain.moveCost");
					const std::string value = (terrain->moveCost < 0)
						? "-"
						: std::to_string(terrain->moveCost) + "%";
					desc = "{" + label + "}: " + value;
				}
				if(!nativeTowns.empty())
				{
					if(!desc.empty()) desc += "\n";
					desc += "{" + LIBRARY->generaltexth->translate("vcmi.wiki.terrain.nativeTowns") + "}: " + nativeTowns;
				}
				categoryEntries[iTerrain].push_back({ terrain->getJsonKey(), terrain->getNameTranslated(), desc, colorIcon, terrain->getModScope(), "" });
			}
		std::sort(categoryEntries[iTerrain].begin(), categoryEntries[iTerrain].end(),
		          [](const WikiEntry & a, const WikiEntry & b){ return a.name < b.name; });
	}

	// Mods – collected by WikiModContent
	categoryEntries[static_cast<int>(WikiCategory::MOD)] = collectModEntries();

	// ---- Category list
	buildCategoryList();

	// ---- Element list (initially empty) ------------------------------------
	buildElementList(-1);

	// ---- Element search box (below element list) ----------------------------
	{
		const Rect sbRect(COL2_X, SEARCH_BOX_Y, COL2_LIST_W + SLIDER_W, SEARCH_BOX_H);
		const ColorRGBA sbBorderColor = (style == Style::BLUE)
		                              ? ColorRGBA(40, 120, 220, 255)
		                              : ColorRGBA(140, 110,  80, 255);
		const ColorRGBA sbHintColor   = (style == Style::BLUE)
		                              ? ColorRGBA(100, 160, 240, 255)
		                              : ColorRGBA(158, 130, 105, 255);
		searchBoxRect = std::make_shared<TransparentFilledRectangle>(
			sbRect, ColorRGBA(0, 0, 0, 120), sbBorderColor);
		searchBoxHint = std::make_shared<CLabel>(
			sbRect.center().x, sbRect.center().y,
			FONT_SMALL, ETextAlignment::CENTER, sbHintColor,
			LIBRARY->generaltexth->translate("vcmi.spellBook.search"));
		searchBox = std::make_shared<CTextInput>(
			sbRect, FONT_SMALL, ETextAlignment::CENTER, false);
		searchBox->setCallback([this](const std::string &) { onSearchInput(); });
	}
	contentBox = std::make_shared<CTextBox>(
		LIBRARY->generaltexth->translate("vcmi.wiki.content.placeholder"),
		Rect(COL3_X + 6, CONTENT_TOP + 3, COL3_W - 12, CONTENT_H - 6),
		(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN,
		FONT_SMALL,
		ETextAlignment::TOPLEFT,
		Colors::WHITE);

	// ---- Town viewport -----------------------------------------------
	// Completely fills the content column.  Only visible when category == Town (1).
	// Content is populated dynamically in rebuildTownViewport() when a town is selected.
	{
		static constexpr int VP_W = COL3_W - 6;   // leave 3 px margin each side
		static constexpr int VP_H = CONTENT_H - 6;

		townContentView = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),  // initial content = viewport size (no scroll)
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
		townContentView->disable(); // shown only for Town category

		creatureContentView = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
		creatureContentView->disable();

		heroContentView = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
		heroContentView->disable();

		artifactContentView = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
		artifactContentView->disable();

		modContentView = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
		modContentView->disable();

		glossaryContentView = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
		glossaryContentView->disable();
	}

	// Mod name shown right-aligned just below the content area (between content and close button).
	modScopeLabel = std::make_shared<CLabel>(
		COL3_X + COL3_W - 8, CONTENT_BOT + 4,
		FONT_SMALL, ETextAlignment::TOPRIGHT,
		ColorRGBA(0, 200, 0, 255), "");
	modScopeLabel->setEnabled(false);

	// ---- Close button -----------------------------------------------------------
	closeButton = std::make_shared<CButton>(
		Point(WIN_W / 2 - 26, CLOSE_Y),
		AnimationPath::builtin(style == Style::BLUE ? "MuBchck" : "IOKAY"),
		CButton::tooltip("", LIBRARY->generaltexth->translate("vcmi.wiki.button.close")),
		std::bind(&WikiWindow::close, this),
		EShortcut::GLOBAL_RETURN);

	// ---- Back button (left border, same style as key-binding reset) -----------
	backButton = std::make_shared<CButton>(
		Point(COL1_X, CLOSE_Y),
		AnimationPath::builtin(style == Style::BLUE ? "buttonBlue80" : "settingsWindow/button80"),
		CButton::tooltip("", LIBRARY->generaltexth->translate("core.help.561.hover")),
		std::bind(&WikiWindow::navigateBack, this));
	backButton->setOverlay(std::make_shared<CLabel>(0, 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
		LIBRARY->generaltexth->translate("core.help.561.hover")));
	backButton->disable(); // hidden until there is history to go back to

	// Apply scroll-wheel bounds after center() so pos is finalised
	applyScrollBounds();

	// Restore in-memory session state (kept while the game runs, not persisted to disk)
	const JsonNode & ws = settings["session"]["wiki"];
	for(const JsonNode & h : ws["history"].Vector())
	{
		navHistory.push_back(WikiEntryKey{
			static_cast<WikiCategory>(h["cat"].Integer()),
			h["entry"].String(),
			h["anchor"].String()
		});
	}

	const JsonNode & lastPageNode = ws["lastPage"];
	const bool hasLastPage = !lastPageNode["entry"].String().empty();

	if(initialEntry && hasLastPage)
	{
		// Push lastPage so Back returns to it after the shortcut entry
		navHistory.push_back(WikiEntryKey{
			static_cast<WikiCategory>(lastPageNode["cat"].Integer()),
			lastPageNode["entry"].String(),
			lastPageNode["anchor"].String()
		});
	}

	if(backButton)
		backButton->setEnabled(!navHistory.empty());

	if(!initialEntry && hasLastPage)
		navigateTo(WikiEntryKey{
			static_cast<WikiCategory>(lastPageNode["cat"].Integer()),
			lastPageNode["entry"].String(),
			lastPageNode["anchor"].String()
		});

	if(initialEntry)
		navigateTo(*initialEntry);
}

// ============================================================================
// WikiWindow – destructor (saves session state)
// ============================================================================

WikiWindow::~WikiWindow()
{
	if(activeCategoryIndex < 0 || activeElementIndex < 0
		|| activeElementIndex >= static_cast<int>(currentDisplayedEntries.size()))
		return;

	Settings ws = settings.write["session"]["wiki"];

	ws["history"].setType(JsonNode::JsonType::DATA_VECTOR);
	for(const WikiEntryKey & k : navHistory)
	{
		JsonNode h;
		h["cat"].Integer() = static_cast<int>(k.category);
		h["entry"].String() = k.entryName;
		h["anchor"].String() = k.anchor;
		ws["history"].Vector().push_back(std::move(h));
	}

	ws["lastPage"]["cat"].Integer() = activeCategoryIndex;
	ws["lastPage"]["entry"].String() = currentDisplayedEntries[activeElementIndex].identifier;
	ws["lastPage"]["anchor"].String() = {};
}

// ============================================================================
// WikiWindow – helpers
// ============================================================================

void WikiWindow::applyScrollBounds()
{
	// scrollBounds is stored relative to the slider's own pos.
	// When the wheel event fires, the engine calculates: testArea = scrollBounds + slider->pos.
	// So we compute: scrollBounds = desired_column_rect - slider->pos.topLeft().
	if(categoryList)
	{
		auto slider = categoryList->getSlider();
		if(slider)
		{
			slider->setScrollBounds(categoryList->pos - slider->pos.topLeft());
			slider->setInertiaEnabled(true);
		}
	}
	if(elementList)
	{
		auto slider = elementList->getSlider();
		if(slider)
		{
			slider->setScrollBounds(elementList->pos - slider->pos.topLeft());
			slider->setInertiaEnabled(true);
		}
	}
}

void WikiWindow::buildCategoryList()
{
	OBJECT_CONSTRUCTION;

	auto createCatItem = [this](size_t idx) -> std::shared_ptr<CIntObject>
	{
		std::string name = (idx < categoryNames.size()) ? categoryNames[idx] : "";
		std::string countStr = (idx < categoryEntries.size())
			? std::to_string(categoryEntries[idx].size()) : "";
		auto item = std::make_shared<WikiListItem>(
			idx, name, countStr,
			[this](WikiListItem * clicked){ onCatItemSelectedCallback(clicked); },
			std::nullopt,
			style == Style::BLUE,
			COL1_LIST_W + SLIDER_W,
			/*hasSlider=*/(categoryNames.size() > VISIBLE_ITEMS));
		item->pos.w = COL1_LIST_W + SLIDER_W;
		item->pos.h = ITEM_H;
		if(static_cast<int>(idx) == activeCategoryIndex)
			item->setSelected(true);
		return item;
	};

	// Slider style bits: bit0=enabled, bit1=horizontal(0=vertical), bit2=blue
	// Brown vertical = 1,  Blue vertical = 5
	const int sliderBits = (style == Style::BLUE) ? 5 : 1;

	// Relative positions: CListBox sits at (COL1_X, CONTENT_TOP) in WikiWindow space.
	// CListBox does pos += Pos first, then OBJECT_CONSTRUCTION makes the slider
	// a child with adjustPosition=true, adding listbox.pos again.
	// Therefore pass slider position RELATIVE to the listbox origin (0,0).
	// SliderPos.w is used as slider length, SliderPos.h as visual width.
	static constexpr int COL1_SLIDER_REL_X = COL1_LIST_W + 1; // = 81

	categoryList = std::make_shared<CListBox>(
		createCatItem,
		Point(COL1_X, CONTENT_TOP),
		Point(0, ITEM_H),
		VISIBLE_ITEMS,
		categoryNames.size(),
		0,
		(categoryNames.size() > VISIBLE_ITEMS) ? sliderBits : 0,
		Rect(COL1_SLIDER_REL_X, 0, CONTENT_H, SLIDER_W));

	// FIX: CListBox never sets pos.w/h – without these the CanvasClipRectGuard
	// in WikiListItem::showAll clips to a 0×0 rect, making all text invisible.
	categoryList->pos.w = COL1_LIST_W + SLIDER_W;
	categoryList->pos.h = CONTENT_H;

	// Ensure full window repaint on scroll so the semi-transparent background
	// is cleared before items are re-drawn over it.
	categoryList->setRedrawParent(true);
}

void WikiWindow::buildElementList(int categoryIndex) // NOSONAR
{
	// Remove the old list from parent before constructing a new one
	elementList.reset();

	OBJECT_CONSTRUCTION;

	const std::string filter = searchBox ? searchBox->getText() : "";

	const auto & allEntries =
		(categoryIndex >= 0 && categoryIndex < static_cast<int>(categoryEntries.size()))
		? categoryEntries[categoryIndex]
		: std::vector<WikiEntry>{};

	std::vector<WikiEntry> entries;
	if(filter.empty())
		entries = allEntries;
	else
		for(const auto & entry : allEntries)
			if(TextOperations::textSearchSimilarityScore(filter, entry.name))
				entries.push_back(entry);

	currentDisplayedEntries = entries;
	size_t total = entries.size();

	auto createElemItem = [this, entries, total](size_t idx) -> std::shared_ptr<CIntObject>
	{
		const std::string name = (idx < entries.size()) ? entries[idx].name : "";
		const std::string subtitle = (idx < entries.size()) ? entries[idx].subtitle : "";
		const std::optional<WikiIconInfo> icon = (idx < entries.size()) ? entries[idx].icon : std::nullopt;
		auto item = std::make_shared<WikiListItem>(
			idx, name, subtitle,
			[this](WikiListItem * clicked){ onElemItemSelectedCallback(clicked); },
			icon, style == Style::BLUE, COL2_LIST_W + SLIDER_W,
			/*hasSlider=*/(total > ELEM_VISIBLE_ITEMS));
		item->pos.w = COL2_LIST_W + SLIDER_W;
		item->pos.h = ITEM_H;
		if(static_cast<int>(idx) == activeElementIndex)
			item->setSelected(true);
		return item;
	};

	// Slider sits 2 px inside from the list-content boundary, 1 px below the top, shrunk 1 px in length.
	static constexpr int COL2_SLIDER_REL_X = COL2_LIST_W - 1; // = 143

	const int sliderBits = (style == Style::BLUE) ? 5 : 1;

	elementList = std::make_shared<CListBox>(
		createElemItem,
		Point(COL2_X, CONTENT_TOP),
		Point(0, ITEM_H),
		ELEM_VISIBLE_ITEMS,
		total,
		0,
		(total > ELEM_VISIBLE_ITEMS) ? sliderBits : 0,
		Rect(COL2_SLIDER_REL_X, 1, ELEM_LIST_H - 1, SLIDER_W));

	// FIX: set pos.w/h so WikiListItem::showAll's clip rect is non-zero
	elementList->pos.w = COL2_LIST_W + SLIDER_W;
	elementList->pos.h = ELEM_LIST_H;

	elementList->setRedrawParent(true);

	// Update scroll bounds for the freshly created slider
	applyScrollBounds();
}

void WikiWindow::onSearchInput()
{
	// Toggle placeholder hint visibility
	if(searchBoxHint)
		searchBoxHint->setEnabled(searchBox && searchBox->getText().empty());

	activeElementIndex = -1;
	buildElementList(activeCategoryIndex);
	updateContent();
}

void WikiWindow::clearElementList()
{
	activeCategoryIndex = -1;
	activeElementIndex  = -1;
	buildElementList(-1);
}

void WikiWindow::updateContent() // NOSONAR
{
	if(!contentBox)
		return;

	// Determine which custom viewport (if any) to show
	const bool useTownViewport = (activeCategoryIndex == static_cast<int>(WikiCategory::TOWN))
	                           && (activeElementIndex >= 0)
	                           && townContentView;
	const bool useCreatureViewport = (activeCategoryIndex == static_cast<int>(WikiCategory::CREATURE))
	                               && (activeElementIndex >= 0)
	                               && creatureContentView;
	const bool useHeroViewport = (activeCategoryIndex == static_cast<int>(WikiCategory::HERO))
	                           && (activeElementIndex >= 0)
	                           && heroContentView;
	const bool useArtifactViewport = (activeCategoryIndex == static_cast<int>(WikiCategory::ARTIFACT))
	                               && (activeElementIndex >= 0)
	                               && artifactContentView;
	const bool useModViewport = (activeCategoryIndex == static_cast<int>(WikiCategory::MOD))
	                          && (activeElementIndex >= 0)
	                          && modContentView;
	const bool useGlossaryViewport = (activeCategoryIndex == static_cast<int>(WikiCategory::GLOSSARY))
	                               && (activeElementIndex >= 0)
	                               && glossaryContentView;
	// A "mod-custom" category uses its own per-category viewport (rendered as Markdown).
	const bool isModCustomCategory = (activeCategoryIndex >= static_cast<int>(WikiCategory::COUNT));
	const bool useModCustomViewport = isModCustomCategory && (activeElementIndex >= 0);
	const bool useCustomViewport = useTownViewport || useCreatureViewport || useHeroViewport || useArtifactViewport || useModViewport || useGlossaryViewport || useModCustomViewport;

	// Toggle viewport / textbox visibility
	if(townContentView)
		townContentView->setEnabled(useTownViewport);
	if(creatureContentView)
		creatureContentView->setEnabled(useCreatureViewport);
	if(heroContentView)
		heroContentView->setEnabled(useHeroViewport);
	if(artifactContentView)
		artifactContentView->setEnabled(useArtifactViewport);
	if(modContentView)
		modContentView->setEnabled(useModViewport);
	if(glossaryContentView)
		glossaryContentView->setEnabled(useGlossaryViewport);
	// Hide all custom-category viewports; the active one is shown below.
	for(const auto & [ci, view] : customCategoryViews)
		if(view) view->setEnabled(false);
	contentBox->setEnabled(!useCustomViewport);

	// Mod name label shown below the content area for all non-glossary entries.
	{
		const bool isGlossary = (activeCategoryIndex == static_cast<int>(WikiCategory::GLOSSARY));
		const bool hasEntry = (activeElementIndex >= 0)
		                   && (activeElementIndex < static_cast<int>(currentDisplayedEntries.size()));
		const std::string & ms = hasEntry ? currentDisplayedEntries[activeElementIndex].modScope : "";
		const bool showScope = !isGlossary && !ms.empty();
		if(modScopeLabel)
		{
			modScopeLabel->setEnabled(showScope);
			if(showScope)
			{
				std::string displayName;
				if(ms == "core")
					displayName = LIBRARY->generaltexth->translate("vcmi.wiki.modName.core");
				else
				{
					const auto dot = ms.find('.');
					const std::string topModId = dot != std::string::npos ? ms.substr(0, dot) : ms;
					displayName = LIBRARY->modh->getModInfo(topModId).getName();
				}
				modScopeLabel->setText(displayName);
			}
		}
	}

	if(useCustomViewport)
	{
		if(useTownViewport)
		{
			const std::string & townIdentifier = currentDisplayedEntries[activeElementIndex].identifier;
			if(townIdentifier != currentTownName)
				rebuildTownViewport(townIdentifier);
		}
		else if(useCreatureViewport)
		{
			const std::string & creatureIdentifier = currentDisplayedEntries[activeElementIndex].identifier;
			if(creatureIdentifier != currentCreatureName)
				rebuildCreatureViewport(creatureIdentifier);
		}
		else if(useHeroViewport)
		{
			const std::string & heroIdentifier = currentDisplayedEntries[activeElementIndex].identifier;
			if(heroIdentifier != currentHeroName)
				rebuildHeroViewport(heroIdentifier);
		}
		else if(useArtifactViewport)
		{
			const std::string & artKey = currentDisplayedEntries[activeElementIndex].identifier;
			if(artKey != currentArtifactName)
				rebuildArtifactViewport(artKey);
		}
		else if(useModViewport)
		{
			const std::string & modId = currentDisplayedEntries[activeElementIndex].identifier;
			if(modId != currentModId)
				rebuildModViewport(modId);
		}
		else if(useGlossaryViewport)
		{
			const std::string & entryName = currentDisplayedEntries[activeElementIndex].identifier;
			if(entryName != currentGlossaryEntryName)
				rebuildGlossaryViewport(entryName);
			else if(!pendingAnchor.empty())
			{
				// Same page, just scroll to the anchor without a full rebuild.
				const auto anchorIt = glossaryAnchorMap.find(pendingAnchor);
				if(anchorIt != glossaryAnchorMap.end())
					glossaryContentView->scrollToY(anchorIt->second);
				pendingAnchor.clear();
			}
		}
		else if(useModCustomViewport)
		{
			const int ci = activeCategoryIndex;
			const std::string & entryName = currentDisplayedEntries[activeElementIndex].identifier;
			const std::string & currentEntry = customCategoryCurrentEntry[ci];
			if(entryName != currentEntry)
				rebuildCustomCategoryViewport(ci, entryName);
			else if(!pendingAnchor.empty())
			{
				const auto anchorIt = customCategoryAnchorMaps[ci].find(pendingAnchor);
				if(anchorIt != customCategoryAnchorMaps[ci].end())
					customCategoryViews[ci]->scrollToY(anchorIt->second);
				pendingAnchor.clear();
			}
			// Ensure this viewport is visible.
			if(customCategoryViews.count(ci) && customCategoryViews[ci])
				customCategoryViews[ci]->setEnabled(true);
		}
	}
	else if(activeCategoryIndex < 0 || activeElementIndex < 0
	     || activeElementIndex >= static_cast<int>(currentDisplayedEntries.size()))
	{
		contentBox->setText(LIBRARY->generaltexth->translate("vcmi.wiki.content.placeholder"));
	}
	else
	{
		const WikiEntry & entry = currentDisplayedEntries[activeElementIndex];

		// Spell/Skill descriptions already contain {name} and level headers inline
		const WikiCategory curCat = static_cast<WikiCategory>(activeCategoryIndex);
		const bool hasInlineTitle = (curCat == WikiCategory::ARTIFACT || curCat == WikiCategory::SPELL || curCat == WikiCategory::SKILL);

		std::string text;
		if(!entry.description.empty())
		{
			if(hasInlineTitle)
				text = entry.description;
			else
				text = "{" + entry.name + "}\n\n" + entry.description;
		}
		else
		{
			text = entry.name;
		}
		contentBox->setText(text);
	}

	ENGINE->windows().totalRedraw();
}

void WikiWindow::rebuildTownViewport(const std::string & factionIdentifier)
{
	currentTownName = factionIdentifier;
	townContentWidgets.clear();

	// Look up the faction by JSON key
	const CFaction * faction = nullptr;
	for(const auto & f : LIBRARY->townh->objects)
	{
		if(f && f->hasTown() && !f->special && f->getJsonKey() == factionIdentifier)
		{
			faction = f.get();
			break;
		}
	}

	if(!faction || !townContentView)
		return;

	// Destroy and recreate the viewport so old children are properly removed.
	static constexpr int VP_W = COL3_W - 6;
	static constexpr int VP_H = CONTENT_H - 6;

	townContentView.reset();
	{
		OBJECT_CONSTRUCTION;
		townContentView = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
	}

	// Populate with real town data.
	// Subtract scrollbar width so tables/grids end before the scrollbar.
	const bool isBlue = (style == Style::BLUE);
	auto navCb = [this](WikiCategory cat, const std::string & name)
	{
		navigateTo(WikiEntryKey{cat, name});
	};
	{
		auto moreWidgets = buildTownContent(*townContentView, faction,
			VP_W - CViewport::SLIDER_W, isBlue, navCb);
		townContentWidgets.insert(townContentWidgets.end(), moreWidgets.begin(), moreWidgets.end());
	}
	townContentView->fitContentSize();

	applyScrollBounds();
}

void WikiWindow::rebuildCreatureViewport(const std::string & creatureIdentifier)
{
	currentCreatureName = creatureIdentifier;
	creatureContentWidgets.clear();

	// Look up the creature by JSON key
	const CCreature * creature = nullptr;
	for(const auto & c : LIBRARY->creh->objects)
	{
		if(c && c->getJsonKey() == creatureIdentifier)
		{
			creature = c.get();
			break;
		}
	}

	if(!creature || !creatureContentView)
		return;

	static constexpr int VP_W = COL3_W - 6;
	static constexpr int VP_H = CONTENT_H - 6;

	creatureContentView.reset();
	{
		OBJECT_CONSTRUCTION;
		creatureContentView = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
	}

	const bool isBlue = (style == Style::BLUE);
	auto navCb = [this](const std::string & name)
	{
		navigateTo(WikiEntryKey{WikiCategory::CREATURE, name});
	};
	{
		auto moreWidgets = buildCreatureContent(*creatureContentView, creature,
			VP_W - CViewport::SLIDER_W, isBlue, navCb);
		creatureContentWidgets.insert(creatureContentWidgets.end(), moreWidgets.begin(), moreWidgets.end());
	}
	creatureContentView->fitContentSize();

	applyScrollBounds();

	// Force a full screen repaint so that leftover pixels from the previous
	// creature's content (e.g. bonus table rows beyond the new viewport area)
	// are properly cleared by redrawing the background underneath.
	ENGINE->windows().totalRedraw();
}

void WikiWindow::rebuildHeroViewport(const std::string & heroIdentifier)
{
	currentHeroName = heroIdentifier;
	heroContentWidgets.clear();

	// Look up the hero by JSON key
	const CHero * hero = nullptr;
	for(const auto & h : LIBRARY->heroh->objects)
	{
		if(h && h->getJsonKey() == heroIdentifier)
		{
			hero = h.get();
			break;
		}
	}

	if(!hero || !heroContentView)
		return;

	static constexpr int VP_W = COL3_W - 6;
	static constexpr int VP_H = CONTENT_H - 6;

	heroContentView.reset();
	{
		OBJECT_CONSTRUCTION;
		heroContentView = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
	}

	const bool isBlue = (style == Style::BLUE);
	auto navCb = [this](WikiCategory cat, const std::string & name)
	{
		navigateTo(WikiEntryKey{cat, name});
	};
	{
		auto moreWidgets = buildHeroContent(*heroContentView, hero,
			VP_W - CViewport::SLIDER_W, isBlue, navCb);
		heroContentWidgets.insert(heroContentWidgets.end(), moreWidgets.begin(), moreWidgets.end());
	}
	heroContentView->fitContentSize();

	applyScrollBounds();
}

void WikiWindow::rebuildArtifactViewport(const std::string & artKey)
{
	currentArtifactName = artKey;
	artifactContentWidgets.clear();

	// Look up artifact by JSON key
	const CArtifact * art = nullptr;
	for(const auto & a : LIBRARY->arth->objects)
	{
		if(a && a->getJsonKey() == artKey)
		{
			art = a.get();
			break;
		}
	}

	if(!art || !artifactContentView)
		return;

	static constexpr int VP_W = COL3_W - 6;
	static constexpr int VP_H = CONTENT_H - 6;

	artifactContentView.reset();
	{
		OBJECT_CONSTRUCTION;
		artifactContentView = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
	}

	const bool isBlue = (style == Style::BLUE);
	auto navCb = [this](WikiCategory cat, const std::string & name)
	{
		navigateTo(WikiEntryKey{cat, name});
	};
	{
		auto moreWidgets = buildArtifactContent(*artifactContentView, art,
			VP_W - CViewport::SLIDER_W, isBlue, navCb);
		artifactContentWidgets.insert(artifactContentWidgets.end(), moreWidgets.begin(), moreWidgets.end());
	}
	artifactContentView->fitContentSize();

	applyScrollBounds();
}

void WikiWindow::rebuildModViewport(const std::string & modId)
{
	currentModId = modId;
	modContentWidgets.clear();

	if(!modContentView)
		return;

	static constexpr int VP_W = COL3_W - 6;
	static constexpr int VP_H = CONTENT_H - 6;

	modContentView.reset();
	{
		OBJECT_CONSTRUCTION;
		modContentView = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
	}

	const bool isBlue = (style == Style::BLUE);
	auto navCb = [this](WikiCategory cat, const std::string & name)
	{
		navigateTo(WikiEntryKey{cat, name});
	};
	{
		auto moreWidgets = buildModContent(*modContentView, modId,
			VP_W - CViewport::SLIDER_W, isBlue, navCb);
		modContentWidgets.insert(modContentWidgets.end(), moreWidgets.begin(), moreWidgets.end());
	}
	modContentView->fitContentSize();

	applyScrollBounds();
}

void WikiWindow::rebuildGlossaryViewport(const std::string & entryName)
{
	currentGlossaryEntryName = entryName;
	glossaryContentWidgets.clear();
	glossaryAnchorMap.clear();

	const auto iGlossary = static_cast<int>(WikiCategory::GLOSSARY);
	const auto & entries = categoryEntries[iGlossary];
	const auto it = std::find_if(entries.begin(), entries.end(),
		[&entryName](const WikiEntry & e){ return e.identifier == entryName; });
	if(it == entries.end())
		return;

	const std::string markdownText = "# " + it->name + "\n\n" + it->description;
	rebuildMarkdownViewport(glossaryContentView, glossaryContentWidgets, glossaryAnchorMap, markdownText);
}

// ============================================================================
// WikiWindow – custom (mod-defined) category viewport
// ============================================================================

void WikiWindow::rebuildCustomCategoryViewport(int catIdx, const std::string & entryName)
{
	customCategoryCurrentEntry[catIdx] = entryName;
	customCategoryWidgets[catIdx].clear();
	customCategoryAnchorMaps[catIdx].clear();

	if(catIdx < 0 || catIdx >= static_cast<int>(categoryEntries.size()))
		return;
	const auto & entries = categoryEntries[catIdx];
	const auto it = std::find_if(entries.begin(), entries.end(),
		[&entryName](const WikiEntry & e){ return e.identifier == entryName; });
	if(it == entries.end())
		return;

	const std::string markdownText = "# " + it->name + "\n\n" + it->description;
	rebuildMarkdownViewport(customCategoryViews[catIdx], customCategoryWidgets[catIdx],
		customCategoryAnchorMaps[catIdx], markdownText);
	customCategoryViews[catIdx]->setEnabled(true);
}

// ============================================================================
// WikiWindow – shared Markdown viewport helpers
// ============================================================================

std::function<void(const std::string &)> WikiWindow::makeLinkCallback()
{
	return [this](const std::string & target){ handleWikiLink(target); };
}

void WikiWindow::handleWikiLink(const std::string & target)
{
	static const std::string PREFIX = "wiki:";
	if(target.size() <= PREFIX.size() || target.substr(0, PREFIX.size()) != PREFIX)
		return;
	const std::string rest = target.substr(PREFIX.size());
	const auto slash = rest.find('/');
	if(slash == std::string::npos) return;
	const std::string catStr = rest.substr(0, slash);
	const std::string idFull = rest.substr(slash + 1);
	const auto hashPos       = idFull.find('#');
	const std::string id     = (hashPos == std::string::npos) ? idFull : idFull.substr(0, hashPos);
	const std::string anchor = (hashPos == std::string::npos) ? std::string{} : idFull.substr(hashPos + 1);

	// Built-in categories
	const auto builtinIt = BUILTIN_CATEGORY_MAP.find(catStr);
	if(builtinIt != BUILTIN_CATEGORY_MAP.end())
	{
		navigateTo(WikiEntryKey{builtinIt->second, id, anchor});
		return;
	}
	// Mod-defined custom categories
	const auto ccIt = customCategoryIds.find(catStr);
	if(ccIt != customCategoryIds.end())
	{
		navigateTo(WikiEntryKey{static_cast<WikiCategory>(ccIt->second), id, anchor});
		return;
	}
}

void WikiWindow::onCatItemSelectedCallback(WikiListItem * clicked)
{
	if(activeCategoryIndex >= 0 && categoryList)
	{
		auto old = std::dynamic_pointer_cast<WikiListItem>(
			categoryList->getItem(activeCategoryIndex));
		if(old && old.get() != clicked)
			old->setSelected(false);
	}
	clicked->setSelected(true);
	onCategoryClicked(static_cast<int>(clicked->index));
}

void WikiWindow::onElemItemSelectedCallback(WikiListItem * clicked)
{
	if(activeElementIndex >= 0 && elementList)
	{
		auto old = std::dynamic_pointer_cast<WikiListItem>(
			elementList->getItem(activeElementIndex));
		if(old && old.get() != clicked)
			old->setSelected(false);
	}
	clicked->setSelected(true);
	onElementClicked(static_cast<int>(clicked->index));
}

void WikiWindow::rebuildMarkdownViewport(
	std::shared_ptr<CViewport> & view,
	std::vector<std::shared_ptr<CIntObject>> & widgets,
	std::map<std::string, int> & anchorMap,
	const std::string & markdownText)
{
	static constexpr int VP_W = COL3_W - 6;
	static constexpr int VP_H = CONTENT_H - 6;

	view.reset();
	{
		OBJECT_CONSTRUCTION;
		view = std::make_shared<CViewport>(
			Rect(COL3_X + 3, CONTENT_TOP + 3, VP_W, VP_H),
			Point(VP_W, VP_H),
			(style == Style::BLUE) ? CSlider::BLUE : CSlider::BROWN);
	}

	auto moreWidgets = buildMarkdownContent(*view, markdownText,
		VP_W - CViewport::SLIDER_W, (style == Style::BLUE), makeLinkCallback(), &anchorMap);
	widgets.insert(widgets.end(), moreWidgets.begin(), moreWidgets.end());
	view->fitContentSize();

	if(!pendingAnchor.empty())
	{
		const auto anchorIt = anchorMap.find(pendingAnchor);
		if(anchorIt != anchorMap.end())
			view->scrollToY(anchorIt->second);
		pendingAnchor.clear();
	}

	applyScrollBounds();
	ENGINE->windows().totalRedraw();
}

// ============================================================================
// WikiWindow – event handlers
// ============================================================================

void WikiWindow::onCategoryClicked(int index)
{
	// Push current entry to history before leaving the category
	if(index != activeCategoryIndex
		&& activeCategoryIndex >= 0 && activeElementIndex >= 0
		&& activeElementIndex < static_cast<int>(currentDisplayedEntries.size()))
	{
		WikiEntryKey cur{static_cast<WikiCategory>(activeCategoryIndex), currentDisplayedEntries[activeElementIndex].identifier};
		if(navHistory.empty() || !(navHistory.back() == cur))
		{
			navHistory.push_back(cur);
			if(backButton)
				backButton->setEnabled(true);
		}
	}

	activeCategoryIndex = index;
	activeElementIndex  = -1;

	// Clear the search filter when switching categories
	if(searchBox) { searchBox->setText(""); }
	if(searchBoxHint) { searchBoxHint->setEnabled(true); }

	// Rebuild the element list for the new category
	buildElementList(activeCategoryIndex);
	updateContent();
}

void WikiWindow::onElementClicked(int index)
{
	// Push current entry onto navigation history so the back button works
	if(index != activeElementIndex
		&& activeCategoryIndex >= 0 && activeElementIndex >= 0
		&& activeElementIndex < static_cast<int>(currentDisplayedEntries.size()))
	{
		WikiEntryKey cur{static_cast<WikiCategory>(activeCategoryIndex), currentDisplayedEntries[activeElementIndex].identifier};
		if(navHistory.empty() || !(navHistory.back() == cur))
		{
			navHistory.push_back(cur);
			if(backButton)
				backButton->setEnabled(true);
		}
	}
	activeElementIndex = index;
	updateContent();
}

void WikiWindow::navigateTo(const WikiEntryKey & key) // NOSONAR
{
	const int catIdx = static_cast<int>(key.category);
	if(catIdx < 0 || catIdx >= static_cast<int>(categoryNames.size()))
		return;

	// Store the anchor to be consumed by rebuildGlossaryViewport().
	pendingAnchor = key.anchor;

	// Deselect old highlighting before changing category/element
	if(categoryList && activeCategoryIndex >= 0)
	{
		if(auto old = std::dynamic_pointer_cast<WikiListItem>(categoryList->getItem(activeCategoryIndex)))
			old->setSelected(false);
	}
	if(elementList && activeElementIndex >= 0)
	{
		if(auto old = std::dynamic_pointer_cast<WikiListItem>(elementList->getItem(activeElementIndex)))
			old->setSelected(false);
	}

	// Push current entry onto navigation history (if we have a valid selection)
	if(activeCategoryIndex >= 0 && activeElementIndex >= 0
		&& activeElementIndex < static_cast<int>(currentDisplayedEntries.size()))
	{
		WikiEntryKey cur{static_cast<WikiCategory>(activeCategoryIndex), currentDisplayedEntries[activeElementIndex].identifier};
		if(navHistory.empty() || !(navHistory.back() == cur))
			navHistory.push_back(cur);
	}
	if(backButton)
		backButton->setEnabled(!navHistory.empty());

	activeCategoryIndex = catIdx;
	activeElementIndex = -1; // reset before rebuild to prevent stale pre-selection
	if(searchBox)
		searchBox->setText("");
	if(searchBoxHint)
		searchBoxHint->setEnabled(true);
	buildElementList(activeCategoryIndex);

	// Select the category item visually
	if(categoryList)
	{
		auto catItem = std::dynamic_pointer_cast<WikiListItem>(categoryList->getItem(activeCategoryIndex));
		if(catItem)
			catItem->setSelected(true);
		categoryList->scrollTo(activeCategoryIndex);
	}

	// Find the entry by identifier and select it (exact match first, then scope-stripped fallback)
	{
		int foundIdx = -1;
		for(int i = 0; i < static_cast<int>(currentDisplayedEntries.size()); ++i)
			if(currentDisplayedEntries[i].identifier == key.entryName)
				{ foundIdx = i; break; }
		// Fallback: "imp" matches "core:imp" (strip mod-scope prefix)
		if(foundIdx < 0 && !key.entryName.empty())
			for(int i = 0; i < static_cast<int>(currentDisplayedEntries.size()); ++i)
			{
				const auto & id = currentDisplayedEntries[i].identifier;
				const auto colon = id.find(':');
				if(colon != std::string::npos && id.substr(colon + 1) == key.entryName)
					{ foundIdx = i; break; }
			}
		if(foundIdx >= 0)
		{
			activeElementIndex = foundIdx;
			if(elementList)
			{
				auto elemItem = std::dynamic_pointer_cast<WikiListItem>(elementList->getItem(foundIdx));
				if(elemItem)
					elemItem->setSelected(true);
				elementList->scrollTo(foundIdx);
			}
		}
	}
	updateContent();
}

void WikiWindow::navigateBack() // NOSONAR
{
	if(navHistory.empty())
		return;

	WikiEntryKey prev = navHistory.back();
	navHistory.pop_back();

	if(backButton)
		backButton->setEnabled(!navHistory.empty());

	// Navigate without pushing to history (direct implementation to avoid recursive push)
	const int catIdx = static_cast<int>(prev.category);
	if(catIdx < 0 || catIdx >= static_cast<int>(categoryNames.size()))
		return;

	// Deselect old highlighting before restoring previous state
	if(categoryList && activeCategoryIndex >= 0)
	{
		if(auto old = std::dynamic_pointer_cast<WikiListItem>(categoryList->getItem(activeCategoryIndex)))
			old->setSelected(false);
	}
	if(elementList && activeElementIndex >= 0)
	{
		if(auto old = std::dynamic_pointer_cast<WikiListItem>(elementList->getItem(activeElementIndex)))
			old->setSelected(false);
	}

	activeCategoryIndex = catIdx;
	activeElementIndex = -1; // reset before rebuild to prevent stale pre-selection
	buildElementList(activeCategoryIndex);

	if(categoryList)
	{
		auto catItem = std::dynamic_pointer_cast<WikiListItem>(categoryList->getItem(activeCategoryIndex));
		if(catItem)
			catItem->setSelected(true);
		categoryList->scrollTo(activeCategoryIndex);
	}

	// Find the entry by identifier (exact match first, then scope-stripped fallback)
	{
		int foundIdx = -1;
		for(int i = 0; i < static_cast<int>(currentDisplayedEntries.size()); ++i)
			if(currentDisplayedEntries[i].identifier == prev.entryName)
				{ foundIdx = i; break; }
		if(foundIdx < 0 && !prev.entryName.empty())
			for(int i = 0; i < static_cast<int>(currentDisplayedEntries.size()); ++i)
			{
				const auto & id = currentDisplayedEntries[i].identifier;
				const auto colon = id.find(':');
				if(colon != std::string::npos && id.substr(colon + 1) == prev.entryName)
					{ foundIdx = i; break; }
			}
		if(foundIdx >= 0)
		{
			activeElementIndex = foundIdx;
			if(elementList)
			{
				auto elemItem = std::dynamic_pointer_cast<WikiListItem>(elementList->getItem(foundIdx));
				if(elemItem)
					elemItem->setSelected(true);
				elementList->scrollTo(foundIdx);
			}
		}
	}
	updateContent();
}