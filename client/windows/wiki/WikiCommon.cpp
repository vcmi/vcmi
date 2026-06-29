/*
 * WikiCommon.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "WikiCommon.h"

#include "../../widgets/Images.h"
#include "../../widgets/TextControls.h"
#include "../../render/Canvas.h"
#include "../../render/Colors.h"
#include "../../render/EFont.h"
#include "../../render/IFont.h"
#include "../../render/IRenderHandler.h"
#include "../../GameEngine.h"
#include "../../eventsSDL/InputHandler.h"

#include "../../../lib/filesystem/ResourcePath.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../../../lib/TerrainHandler.h"
#include "../../../lib/mapping/MapEditUtils.h"

// ─────────────────────────────────────────────────────────────────────────────
// WikiClickable
// ─────────────────────────────────────────────────────────────────────────────

WikiClickable::WikiClickable(Rect area,
                              std::function<void()> lclick,
                              std::function<void()> rclick,
                              bool blue,
                              std::optional<Rect> clip)
	: CIntObject(LCLICK | SHOW_POPUP | HOVER, area.topLeft())
	, onLeftClick(std::move(lclick))
	, onRightClick(std::move(rclick))
	, clipRect(clip)
	, blueTheme(blue)
{
	pos.w = area.w;
	pos.h = area.h;
}

void WikiClickable::clickPressed(const Point & cur)
{
	if(clipRect && !clipRect->isInside(cur))
		return;
	if(onLeftClick)
		onLeftClick();
}

void WikiClickable::showPopupWindow(const Point & cur)
{
	if(clipRect && !clipRect->isInside(cur))
		return;
	if(onRightClick)
		onRightClick();
}

void WikiClickable::hover(bool on)
{
	if(hovered != on)
	{
		hovered = on;
		redraw();
	}
}

void WikiClickable::showAll(Canvas & to)
{
	const Point cur = ENGINE->getCursorPosition();
	const bool inClip = !clipRect || clipRect->isInside(cur);
	hovered = pos.isInside(cur) && inClip && ENGINE->input().getCurrentInputMode() != InputMode::TOUCH;
	if(hovered)
	{
		const ColorRGBA hoverCol = blueTheme
			? ColorRGBA(60, 100, 180, 60)
			: ColorRGBA(180, 160, 100, 60);
		to.drawColorBlended(pos, hoverCol);
	}
	CIntObject::showAll(to);
}

// ─────────────────────────────────────────────────────────────────────────────
// WikiTableGrid
// ─────────────────────────────────────────────────────────────────────────────

WikiTableGrid::WikiTableGrid(
	int x, int y, int totalW,
	const std::vector<int> & colWidths,
	int headerH,
	int dataRowH,
	int dataRowCount,
	bool blue)
	: GraphicalPrimitiveCanvas(Rect(x, y, totalW, headerH + dataRowH * dataRowCount))
{
	const ColorRGBA border = wikiBorderColor(blue);
	const ColorRGBA inner  = wikiInnerColor(blue);
	const int totalH = headerH + dataRowH * dataRowCount;

	if(headerH > 0)
	{
		const ColorRGBA header = blue
			? ColorRGBA{20, 40, 80, 140}
			: ColorRGBA{60, 50, 30, 140};
		addBox(Point(0, 0), Point(totalW, headerH), header);
	}

	addRectangle(Point(0, 0), Point(totalW, totalH), border);

	if(headerH > 0)
		addLine(Point(0, headerH), Point(totalW, headerH), border);

	for(int r = 1; r < dataRowCount; ++r)
	{
		const int lineY = headerH + r * dataRowH;
		addLine(Point(1, lineY), Point(totalW - 2, lineY), inner);
	}

	int cx = 0;
	for(int i = 0; i + 1 < static_cast<int>(colWidths.size()); ++i)
	{
		cx += colWidths[i];
		addLine(Point(cx, 1), Point(cx, totalH - 2), inner);
	}
}

WikiTableGrid::WikiTableGrid(
	int x, int y, int totalW,
	std::initializer_list<int> colWidths,
	int headerH,
	int dataRowH,
	int dataRowCount,
	bool blue)
	: WikiTableGrid(x, y, totalW, std::vector<int>(colWidths), headerH, dataRowH, dataRowCount, blue)
{}

// ─────────────────────────────────────────────────────────────────────────────
// WikiResourceCost
// ─────────────────────────────────────────────────────────────────────────────

static constexpr int RES_ICON_W = 14;
static constexpr int RES_GAP    =  3;
static constexpr int RES_ENTRY_W = RES_ICON_W + 26 + RES_GAP;

WikiResourceCost::WikiResourceCost(
	const TResources & cost,
	int startX, int y,
	int maxWidth)
	: CIntObject(0, Point(startX, y))
{
	OBJECT_CONSTRUCTION;

	struct ResEntry { GameResID id; int amount; };
	std::vector<ResEntry> entries;

	TResources::nziterator it(cost);
	while(it.valid())
	{
		entries.push_back({it->resType, static_cast<int>(it->resVal)});
		++it;
	}

	if(entries.empty())
		return;

	std::stable_sort(entries.begin(), entries.end(), [](const ResEntry & a, const ResEntry & b)
	{
		if((a.id == GameResID::GOLD) != (b.id == GameResID::GOLD))
			return a.id == GameResID::GOLD;
		return a.id < b.id;
	});

	int relX = 0;
	for(const auto & e : entries)
	{
		if(relX + RES_ENTRY_W > maxWidth)
			break;

		items.push_back(std::make_shared<CAnimImage>(
			AnimationPath::builtin("SMALRES"), e.id.getNum(), 0, relX, 0));
		items.push_back(std::make_shared<CLabel>(
			relX + RES_ICON_W + 7, 1,
			FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE,
			std::to_string(e.amount)));

		relX += RES_ENTRY_W;
	}

	pos.w = relX;
	pos.h = 16;
}

// ─────────────────────────────────────────────────────────────────────────────
// terrainIconInfo
// ─────────────────────────────────────────────────────────────────────────────

WikiIconInfo terrainIconInfo(const TerrainType * terrain)
{
	size_t frame = 0;
	if(terrain)
	{
		// Use the same frame selection as the battle terrain selector (BattleOnlyModeTab):
		// find the "n1" (plain/normal) pattern and use its first mapped frame index.
		const auto & patterns = LIBRARY->terviewh->getTerrainViewPatterns(terrain->getId());
		for(const auto & p : patterns)
			if(!p.empty() && p[0].id == "n1" && !p[0].mapping.empty())
				{ frame = static_cast<size_t>(p[0].mapping[0].first); break; }
	}
	return WikiIconInfo{terrain ? terrain->tilesFilename : AnimationPath{}, frame, 0};
}

// ─────────────────────────────────────────────────────────────────────────────
// wikiGenderSpan
// ─────────────────────────────────────────────────────────────────────────────

std::string wikiGenderSpan(bool isFemale)
{
	// Check once whether FONT_SMALL can render ♂ (U+2642, UTF-8: e2 99 82).
	static std::optional<bool> glyphSupported;
	if(!glyphSupported)
	{
		auto font = ENGINE->renderHandler().loadFont(EFonts::FONT_SMALL);
		const char maleGlyph[] = "\xe2\x99\x82";
		glyphSupported = font && font->canRepresentCharacter(maleGlyph);
	}

	const char * femaleColor = "#FF69B4";
	const char * maleColor   = "#5080FF";
	const char * color = isFemale ? femaleColor : maleColor;

	if(*glyphSupported)
	{
		const char * glyph = isFemale ? "\xe2\x99\x80" : "\xe2\x99\x82";
		return std::string("{") + color + "|" + glyph + "}";
	}
	else
	{
		const std::string fallback = isFemale
			? LIBRARY->generaltexth->translate("vcmi.wiki.gender.female")
			: LIBRARY->generaltexth->translate("vcmi.wiki.gender.male");
		return std::string("{") + color + "|" + fallback + "}";
	}
}
