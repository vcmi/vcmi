/*
 * WikiArtifactContent.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "WikiArtifactContent.h"
#include "WikiCommon.h"

#include "../../widgets/CViewport.h"
#include "../../widgets/Images.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/GraphicalPrimitiveCanvas.h"
#include "../../render/Canvas.h"
#include "../../render/Colors.h"
#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/entities/artifact/CArtifact.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/entities/artifact/EArtifactClass.h"
#include "../../../lib/entities/artifact/ArtBearer.h"
#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../../../lib/GameConstants.h"
#include "WikiWindow.h"
#include "../InfoWindows.h"

// ─────────────────────────────────────────────────────────────────────────────
// Layout constants
// ─────────────────────────────────────────────────────────────────────────────

static constexpr int MARGIN = 4;
static constexpr int GAP    = 8;
static constexpr int CELL_L = 4;
static constexpr int CELL_T = 2;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string artifactClassName(int serial)
{
	switch(serial)
	{
		case 0: return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.class.treasure");
		case 1: return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.class.minor");
		case 2: return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.class.major");
		case 3: return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.class.relic");
		case 4: return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.class.scroll");
		default: return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.class.other");
	}
}

static std::string slotName(int pos)
{
	using P = ArtifactPositionBase;
	switch(pos)
	{
		case P::HEAD:        return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.head");
		case P::SHOULDERS:   return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.shoulders");
		case P::NECK:        return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.neck");
		case P::RIGHT_HAND:  return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.rightHand");
		case P::LEFT_HAND:   return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.leftHand");
		case P::TORSO:       return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.torso");
		case P::RIGHT_RING:  return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.rightRing");
		case P::LEFT_RING:   return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.leftRing");
		case P::FEET:        return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.feet");
		case P::MISC1:
		case P::MISC2:
		case P::MISC3:
		case P::MISC4:
		case P::MISC5:       return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.misc");
		case P::MACH1:
		case P::MACH2:
		case P::MACH3:
		case P::MACH4:       return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.warMachine");
		case P::SPELLBOOK:   return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.spellbook");
		default:
			if(pos >= P::BACKPACK_START)
				return LIBRARY->generaltexth->translate("vcmi.wiki.artifact.slot.backpack");
			return "";
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// buildArtifactContent
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::shared_ptr<CIntObject>> buildArtifactContent(
	CViewport & viewport,
	const CArtifact * art,
	int viewportWidth,
	bool blueStyle,
	WikiArtifactNavigateCallback navigateCallback)
{
	std::vector<std::shared_ptr<CIntObject>> widgets;
	if(!art) return widgets;

	OBJECT_CONSTRUCTION_TARGETED(viewport.content());
	const int W    = viewportWidth;
	int       curY = 12;
	const Rect clipRect = viewport.clipRect();

	const ColorRGBA valCol = blueStyle
		? ColorRGBA(160, 210, 255, 255)
		: Colors::YELLOW;

	// ── 1. Title ────────────────────────────────────────────────────────
	widgets.push_back(std::make_shared<CLabel>(
		W / 2, curY,
		FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,
		art->getNameTranslated()));
	curY += 26;

	// ── 2. Class · Price · Slots info row ────────────────────────────────
	{
		// Class name
		const std::string cls = artifactClassName(art->getArtClassSerial());

		// Hero slot names (deduplicated)
		std::string slotsStr;
		{
			const auto & slotMap = art->getPossibleSlots();
			auto it = slotMap.find(ArtBearer::HERO);
			if(it != slotMap.end())
			{
				std::vector<std::string> seen;
				for(const ArtifactPosition & pos : it->second)
				{
					std::string name = slotName(pos.num);
					if(!name.empty() && std::find(seen.begin(), seen.end(), name) == seen.end())
					{
						seen.push_back(name);
						if(!slotsStr.empty()) slotsStr += ", ";
						slotsStr += name;
					}
				}
			}
		}

		// Combined badge
		if(art->isCombined())
		{
			const ColorRGBA combineCol = blueStyle
				? ColorRGBA(100, 220, 100, 255)
				: ColorRGBA(80, 200, 80, 255);
			widgets.push_back(std::make_shared<CLabel>(
				W / 2, curY,
				FONT_SMALL, ETextAlignment::CENTER, combineCol,
				LIBRARY->generaltexth->translate("vcmi.wiki.artifact.combined")));
			curY += 16;
		}

		// Info table: Class | Price | Slots
		const int tableW  = W - MARGIN * 2;
		const int rowH    = 18;
		const int colCls  = tableW / 3;
		const int colPrc  = tableW / 3;
		const int colSlt  = tableW - colCls - colPrc;

		widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW,
			std::vector<int>{colCls, colPrc, colSlt}, 16, rowH, 1, blueStyle));

		// Column headers
		widgets.push_back(std::make_shared<CLabel>(
			MARGIN + colCls / 2, curY + CELL_T,
			FONT_TINY, ETextAlignment::TOPCENTER, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.wiki.artifact.header.class")));
		widgets.push_back(std::make_shared<CLabel>(
			MARGIN + colCls + colPrc / 2, curY + CELL_T,
			FONT_TINY, ETextAlignment::TOPCENTER, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.wiki.artifact.header.price")));
		widgets.push_back(std::make_shared<CLabel>(
			MARGIN + colCls + colPrc + CELL_L, curY + CELL_T,
			FONT_TINY, ETextAlignment::TOPLEFT, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.wiki.artifact.header.slots")));
		curY += 16;

		// Data row
		widgets.push_back(std::make_shared<CLabel>(
			MARGIN + colCls / 2, curY + CELL_T,
			FONT_SMALL, ETextAlignment::TOPCENTER, Colors::WHITE, cls));
		// Price (gold icon + number)
		const int goldFrame = GameResID(EGameResID::GOLD).getNum();
		const int iconSz    = rowH - 4; // ~14px for rowH=18
		const int iconX     = MARGIN + colCls + 2;
		widgets.push_back(std::make_shared<CAnimImage>(
			AnimationPath::builtin("SMALRES"), goldFrame,
			Rect(iconX, curY + CELL_T, iconSz, iconSz), 0));
		widgets.push_back(std::make_shared<CLabel>(
			iconX + iconSz + 2, curY + CELL_T,
			FONT_SMALL, ETextAlignment::TOPLEFT, valCol,
			std::to_string(art->getPrice())));
		widgets.push_back(std::make_shared<CLabel>(
			MARGIN + colCls + colPrc + CELL_L, curY + CELL_T,
			FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, slotsStr));
		curY += rowH + GAP;
	}

	// ── 3. Description ─────────────────────────────────────────────────────
	{
		const std::string desc = art->getDescriptionTranslated();
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

	// ── Helper: render one artifact row in a clickable table ───────────
	auto addArtifactRow = [&](const CArtifact * a, int tableW, int rowH)
	{
		if(!a) return;
		const int iconSz = rowH - 4;
		widgets.push_back(std::make_shared<CAnimImage>(
			AnimationPath::builtin("Artifact"),
			a->getIconIndex(),
			Rect(MARGIN + 2, curY + 2, iconSz, iconSz), 0));
		widgets.push_back(std::make_shared<CLabel>(
			MARGIN + rowH + CELL_L, curY + rowH / 2 - 5,
			FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
			a->getNameTranslated()));

		std::function<void()> lclick;
		if(navigateCallback)
		{
			const std::string artKey = a->getJsonKey();
			lclick = [navigateCallback, artKey](){ navigateCallback(WikiCategory::ARTIFACT, artKey); };
		}
		const ArtifactID artId = a->getId();
		widgets.push_back(std::make_shared<WikiClickable>(
			Rect(MARGIN, curY, tableW, rowH),
			std::move(lclick),
			[artId](){ CRClickPopup::createAndPush(LIBRARY->arth->objects[artId.getNum()]->getDescriptionTranslated()); },
			blueStyle, clipRect));
		curY += rowH;
	};

	// ── 4. Constituents (if combined artifact) ───────────────────────────
	if(art->isCombined() && !art->getConstituents().empty())
	{
		curY += 8;
		widgets.push_back(std::make_shared<CLabel>(
			W / 2, curY,
			FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.wiki.artifact.constituents")));
		curY += 20;

		const int rowH   = 36;
		const int tableW = W - MARGIN * 2;
		const int iconW  = 36;
		const std::vector<int> cols = {iconW, tableW - iconW};

		widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW, cols,
			0, rowH, static_cast<int>(art->getConstituents().size()), blueStyle));

		for(const CArtifact * c : art->getConstituents())
			addArtifactRow(c, tableW, rowH);

		curY += GAP;
	}

	// ── 5. Part of (if this is a constituent of combined artifact(s)) ────
	if(!art->getPartOf().empty())
	{
		curY += 8;
		widgets.push_back(std::make_shared<CLabel>(
			W / 2, curY,
			FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.wiki.artifact.partOf")));
		curY += 20;

		const int rowH   = 36;
		const int tableW = W - MARGIN * 2;
		const int iconW  = 36;
		const std::vector<int> cols = {iconW, tableW - iconW};

		widgets.push_back(std::make_shared<WikiTableGrid>(MARGIN, curY, tableW, cols,
			0, rowH, static_cast<int>(art->getPartOf().size()), blueStyle));

		for(const CArtifact * p : art->getPartOf())
			addArtifactRow(p, tableW, rowH);

		curY += GAP;
	}

	return widgets;
}
