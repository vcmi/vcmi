/*
 * WikiModContent.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "WikiModContent.h"
#include "WikiCommon.h"

#include "../../widgets/CViewport.h"
#include "../../widgets/Images.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/GraphicalPrimitiveCanvas.h"
#include "../../render/Colors.h"
#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../../render/IRenderHandler.h"
#include "../InfoWindows.h"
#include "../CCreatureWindow.h"
#include "../CHeroOverview.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/CSkillHandler.h"
#include "../../../lib/spells/CSpellHandler.h"
#include "../../../lib/TerrainHandler.h"
#include "../../../lib/entities/faction/CFaction.h"
#include "../../../lib/entities/faction/CTownHandler.h"
#include "../../../lib/entities/faction/CTown.h"
#include "../../../lib/entities/hero/CHero.h"
#include "../../../lib/entities/hero/CHeroHandler.h"
#include "../../../lib/entities/artifact/CArtifact.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/entities/artifact/EArtifactClass.h"
#include "../../../lib/modding/CModHandler.h"
#include "../../../lib/modding/ModDescription.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "WikiWindow.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static constexpr int MARGIN = 4;
static constexpr int GAP    = 10;
static constexpr int CELL_L = 4;

/// Returns native frame size (capped at maxSide) for a given animation, same logic as WikiTownContent.
static Point iconDimensions(const AnimationPath & path, int maxSide = 200)
{
	if(path.getName().empty())
		return {32, 32};
	auto img = ENGINE->renderHandler().loadImage(path, 0, 0, EImageBlitMode::COLORKEY);
	if(!img)
		return {32, 32};
	const Point d = img->dimensions();
	return {std::min(d.x, maxSide), std::min(d.y, maxSide)};
}

/// Returns true if entityScope belongs to the given top-level modId.
static bool isEntityOfMod(const std::string & scope, const std::string & modId)
{
	return scope == modId || scope.rfind(modId + ".", 0) == 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// collectModEntries
// ─────────────────────────────────────────────────────────────────────────────

std::vector<WikiEntry> collectModEntries()
{
	// Build set of all entity mod-scopes that have content
	auto collectScopes = [](std::set<std::string> & out)
	{
		for(const auto & f : LIBRARY->townh->objects)
			if(f && f->hasTown() && !f->special) out.insert(f->getModScope());
		for(const auto & h : LIBRARY->heroh->objects)
			if(h && !h->special) out.insert(h->getModScope());
		for(const auto & c : LIBRARY->creh->objects)
			if(c && !c->special) out.insert(c->getModScope());
		for(const auto & a : LIBRARY->arth->objects)
			if(a && a->aClass != EArtifactClass::ART_SPECIAL) out.insert(a->getModScope());
		for(const auto & s : LIBRARY->spellh->objects)
			if(s && !s->isSpecial() && !s->isCreatureAbility()) out.insert(s->getModScope());
		for(const auto & sk : LIBRARY->skillh->objects)
			if(sk) out.insert(sk->getModScope());
		for(const auto & t : LIBRARY->terrainTypeHandler->objects)
			if(t && t->getId().getNum() > 0) out.insert(t->getModScope());
	};
	std::set<std::string> contentScopes;
	collectScopes(contentScopes);

	auto modHasContent = [&](const std::string & modId) -> bool
	{
		for(const auto & scope : contentScopes)
			if(scope == modId || scope.rfind(modId + ".", 0) == 0)
				return true;
		return false;
	};

	std::vector<WikiEntry> result;
	for(const auto & modId : LIBRARY->modh->getActiveMods())
	{
		if(modId == "core") continue;
		const auto & info = LIBRARY->modh->getModInfo(modId);
		if(!info.getParentID().empty()) continue; // sub-mod – skip
		if(!modHasContent(modId)) continue;
		result.push_back({ modId, info.getName(), "", std::nullopt, modId, "" });
	}
	std::sort(result.begin(), result.end(),
		[](const WikiEntry & a, const WikiEntry & b){ return a.name < b.name; });
	return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// buildModContent
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::shared_ptr<CIntObject>> buildModContent(
	CViewport & viewport,
	const std::string & modId,
	int viewportWidth,
	bool blueStyle,
	WikiModNavigateCallback navigateCallback)
{
	std::vector<std::shared_ptr<CIntObject>> widgets;

	OBJECT_CONSTRUCTION_TARGETED(viewport.content());
	const Rect clipRect = viewport.clipRect();
	const int W    = viewportWidth;
	int       curY = 12;

	// ── Title ─────────────────────────────────────────────────────────
	const std::string modName = LIBRARY->modh->getModInfo(modId).getName();
	widgets.push_back(std::make_shared<CLabel>(
		W / 2, curY,
		FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, modName));
	curY += 30;

	// ── Generic section builder ────────────────────────────────────────
	// Adds header + icon/name table for a list of entities.
	// iconFn(entity) → optional<WikiIconInfo>
	// nameFn(entity) → display name string
	// keyFn(entity)  → json key string
	// descFn(entity) → right-click description (may be empty)
	// category       → target wiki category for navigation

	const int tableW = W - MARGIN * 2;

	auto makeSectionHeader = [&](const std::string & translationKey)
	{
		curY += 12;
		widgets.push_back(std::make_shared<CLabel>(
			W / 2, curY,
			FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
			LIBRARY->generaltexth->translate(translationKey)));
		curY += 22;
	};

	// Helper: render one icon+name row with optional click behaviour.
	// Uses native-size CAnimImage (no forced scaling). iIconW/iRowH from iconDimensions().
	auto addIconRow = [&](
		const AnimationPath & anim, size_t frame,
		int iIconW, int iRowH,
		const std::string & name,
		WikiCategory cat, const std::string & key,
		const std::string & description,
		std::function<void()> rclickFn = nullptr)
	{
		widgets.push_back(std::make_shared<CAnimImage>(
			anim, frame, 0,
			MARGIN + 2, curY + (iRowH - iconDimensions(anim).y) / 2));
		widgets.push_back(std::make_shared<CLabel>(
			MARGIN + iIconW + CELL_L, curY + iRowH / 2 - 5,
			FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, name));

		std::function<void()> lclick;
		if(navigateCallback)
		{
			const std::string k = key;
			lclick = [navigateCallback, cat, k](){ navigateCallback(cat, k); };
		}
		std::function<void()> rclick = std::move(rclickFn);
		if(!rclick && !description.empty())
		{
			const std::string d = description;
			rclick = [d](){ CRClickPopup::createAndPush(d); };
		}
		widgets.push_back(std::make_shared<WikiClickable>(
			Rect(MARGIN, curY, tableW, iRowH),
			std::move(lclick), std::move(rclick), blueStyle, clipRect));
		curY += iRowH;
	};

	// ── Towns ─────────────────────────────────────────────────────────
	{
		std::vector<const CFaction *> entries;
		for(const auto & obj : LIBRARY->townh->objects)
			if(obj && obj->hasTown() && !obj->special && isEntityOfMod(obj->getModScope(), modId))
				entries.push_back(obj.get());

		if(!entries.empty())
		{
			std::sort(entries.begin(), entries.end(),
				[](const CFaction * a, const CFaction * b){ return a->getNameTranslated() < b->getNameTranslated(); });

			const Point iconSz = iconDimensions(AnimationPath::builtin("ITPA"));
			const int iIconW = iconSz.x + 4;
			const int iRowH  = iconSz.y + 4;
			const int iNameW = tableW - iIconW;
			makeSectionHeader("vcmi.wiki.category.town");
			widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW,
				std::vector<int>{iIconW, iNameW}, 0, iRowH, (int)entries.size(), blueStyle));

			for(const CFaction * f : entries)
				addIconRow(
					AnimationPath::builtin("ITPA"),
					(size_t)(f->town->clientInfo.icons[1][0] + 2),
					iIconW, iRowH,
					f->getNameTranslated(),
					WikiCategory::TOWN, f->getJsonKey(), "");
			curY += GAP;
		}
	}

	// ── Heroes ────────────────────────────────────────────────────────
	{
		std::vector<const CHero *> entries;
		for(const auto & obj : LIBRARY->heroh->objects)
			if(obj && !obj->special && isEntityOfMod(obj->getModScope(), modId))
				entries.push_back(obj.get());

		if(!entries.empty())
		{
			std::sort(entries.begin(), entries.end(),
				[](const CHero * a, const CHero * b){ return a->getNameTranslated() < b->getNameTranslated(); });

			const Point iconSz = iconDimensions(AnimationPath::builtin("PortraitsSmall"));
			const int iIconW = iconSz.x + 4;
			const int iRowH  = iconSz.y + 4;
			const int iNameW = tableW - iIconW;
			makeSectionHeader("vcmi.wiki.category.hero");
			widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW,
				std::vector<int>{iIconW, iNameW}, 0, iRowH, (int)entries.size(), blueStyle));

			for(const CHero * h : entries)
			{
				const HeroTypeID hId = h->getId();
				addIconRow(
					AnimationPath::builtin("PortraitsSmall"),
					(size_t)h->getIconIndex(),
					iIconW, iRowH,
					h->getNameTranslated(),
					WikiCategory::HERO, h->getJsonKey(), "",
					[hId](){ ENGINE->windows().createAndPushWindow<CHeroOverview>(hId); });
			}
			curY += GAP;
		}
	}

	// ── Creatures ─────────────────────────────────────────────────────
	{
		std::vector<const CCreature *> entries;
		for(const auto & obj : LIBRARY->creh->objects)
			if(obj && !obj->special && isEntityOfMod(obj->getModScope(), modId))
				entries.push_back(obj.get());

		if(!entries.empty())
		{
			std::sort(entries.begin(), entries.end(),
				[](const CCreature * a, const CCreature * b){ return a->getNameSingularTranslated() < b->getNameSingularTranslated(); });

			const Point iconSz = iconDimensions(AnimationPath::builtin("CPRSMALL"));
			const int iIconW = iconSz.x + 4;
			const int iRowH  = iconSz.y + 4;
			const int iNameW = tableW - iIconW;
			makeSectionHeader("vcmi.wiki.category.creature");
			widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW,
				std::vector<int>{iIconW, iNameW}, 0, iRowH, (int)entries.size(), blueStyle));

			for(const CCreature * c : entries)
			{
				const CCreature * crPtr = c;
				addIconRow(
					AnimationPath::builtin("CPRSMALL"),
					(size_t)c->getIconIndex(),
					iIconW, iRowH,
					c->getNameSingularTranslated(),
					WikiCategory::CREATURE, c->getJsonKey(), "",
					[crPtr](){ ENGINE->windows().createAndPushWindow<CStackWindow>(crPtr, true); });
			}
			curY += GAP;
		}
	}

	// ── Artifacts ─────────────────────────────────────────────────────
	{
		std::vector<const CArtifact *> entries;
		for(const auto & obj : LIBRARY->arth->objects)
			if(obj && obj->aClass != EArtifactClass::ART_SPECIAL
				&& isEntityOfMod(obj->getModScope(), modId))
				entries.push_back(obj.get());

		if(!entries.empty())
		{
			std::sort(entries.begin(), entries.end(),
				[](const CArtifact * a, const CArtifact * b){ return a->getNameTranslated() < b->getNameTranslated(); });

			const Point iconSz = iconDimensions(AnimationPath::builtin("Artifact"));
			const int iIconW = iconSz.x + 4;
			const int iRowH  = iconSz.y + 4;
			const int iNameW = tableW - iIconW;
			makeSectionHeader("vcmi.wiki.category.artifact");
			widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW,
				std::vector<int>{iIconW, iNameW}, 0, iRowH, (int)entries.size(), blueStyle));

			for(const CArtifact * a : entries)
				addIconRow(
					AnimationPath::builtin("Artifact"),
					(size_t)a->getIconIndex(),
					iIconW, iRowH,
					a->getNameTranslated(),
					WikiCategory::ARTIFACT, a->getJsonKey(),
					a->getDescriptionTranslated());
			curY += GAP;
		}
	}

	// ── Spells ────────────────────────────────────────────────────────
	{
		std::vector<const CSpell *> entries;
		for(const auto & obj : LIBRARY->spellh->objects)
			if(obj && !obj->isSpecial() && !obj->isCreatureAbility()
				&& isEntityOfMod(obj->getModScope(), modId))
				entries.push_back(obj.get());

		if(!entries.empty())
		{
			std::sort(entries.begin(), entries.end(),
				[](const CSpell * a, const CSpell * b){ return a->getNameTranslated() < b->getNameTranslated(); });

			const Point iconSz = iconDimensions(AnimationPath::builtin("SpellInt"));
			const int iIconW = iconSz.x + 4;
			const int iRowH  = iconSz.y + 4;
			const int iNameW = tableW - iIconW;
			makeSectionHeader("vcmi.wiki.category.spell");
			widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW,
				std::vector<int>{iIconW, iNameW}, 0, iRowH, (int)entries.size(), blueStyle));

			for(const CSpell * s : entries)
				addIconRow(
					AnimationPath::builtin("SpellInt"),
					(size_t)s->getIndex() + 1,
					iIconW, iRowH,
					s->getNameTranslated(),
					WikiCategory::SPELL, s->getJsonKey(),
					s->getDescriptionTranslated(1));
			curY += GAP;
		}
	}

	// ── Skills ────────────────────────────────────────────────────────
	{
		std::vector<const CSkill *> entries;
		for(const auto & obj : LIBRARY->skillh->objects)
			if(obj && isEntityOfMod(obj->getModScope(), modId))
				entries.push_back(obj.get());

		if(!entries.empty())
		{
			std::sort(entries.begin(), entries.end(),
				[](const CSkill * a, const CSkill * b){ return a->getNameTranslated() < b->getNameTranslated(); });

			const Point iconSz = iconDimensions(AnimationPath::builtin("SECSK32"));
			const int iIconW = iconSz.x + 4;
			const int iRowH  = iconSz.y + 4;
			const int iNameW = tableW - iIconW;
			makeSectionHeader("vcmi.wiki.category.skill");
			widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW,
				std::vector<int>{iIconW, iNameW}, 0, iRowH, (int)entries.size(), blueStyle));

			for(const CSkill * sk : entries)
				addIconRow(
					AnimationPath::builtin("SECSK32"),
					(size_t)(sk->getIndex() * 3 + 3),
					iIconW, iRowH,
					sk->getNameTranslated(),
					WikiCategory::SKILL, sk->getJsonKey(),
					sk->getDescriptionTranslated(1));
			curY += GAP;
		}
	}

	// ── Terrains (id > 0) ─────────────────────────────────────────────
	{
		std::vector<const TerrainType *> entries;
		for(const auto & obj : LIBRARY->terrainTypeHandler->objects)
			if(obj && obj->getId().getNum() > 0 && isEntityOfMod(obj->getModScope(), modId))
				entries.push_back(obj.get());

		if(!entries.empty())
		{
			std::sort(entries.begin(), entries.end(),
				[](const TerrainType * a, const TerrainType * b){ return a->getNameTranslated() < b->getNameTranslated(); });

			const int iRowH  = 36;
			const int iIconW = iRowH;
			const int iNameW = tableW - iIconW;
			makeSectionHeader("vcmi.wiki.category.terrain");
			widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW,
				std::vector<int>{iIconW, iNameW}, 0, iRowH, (int)entries.size(), blueStyle));

			for(const TerrainType * t : entries)
			{
				WikiIconInfo icon = terrainIconInfo(t);
				addIconRow(icon.path, icon.frame, iIconW, iRowH, t->getNameTranslated(), WikiCategory::TERRAIN, t->getJsonKey(), "");
			}
			curY += GAP;
		}
	}

	// bottom spacer so fitContentSize() sees the full height
	curY += 20;
	widgets.push_back(std::make_shared<GraphicalPrimitiveCanvas>(Rect(0, curY, 1, 1)));

	return widgets;
}
