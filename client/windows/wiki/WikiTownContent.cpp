/*
 * WikiTownContent.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "WikiTownContent.h"
#include "WikiCommon.h"
#include "WikiWindow.h"

#include "../../widgets/CViewport.h"
#include "../../widgets/Images.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/GraphicalPrimitiveCanvas.h"

#include "../../render/Canvas.h"
#include "../../render/CanvasImage.h"
#include "../../render/IRenderHandler.h"
#include "../../render/Colors.h"
#include "../../render/IImage.h"

#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../InfoWindows.h"
#include "../CCreatureWindow.h"

#include "../../../lib/CStack.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/entities/faction/CFaction.h"
#include "../../../lib/entities/faction/CTown.h"
#include "../../../lib/entities/building/CBuilding.h"
#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/ResourceSet.h"
#include "../../../lib/texts/CGeneralTextHandler.h"
#include "../../../lib/entities/hero/CHeroHandler.h"
#include "../../../lib/entities/hero/CHero.h"
#include "../../../lib/entities/hero/CHeroClass.h"
#include "../../../lib/entities/hero/EHeroGender.h"
#include "../CHeroOverview.h"

#include <algorithm>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Layout constants
// ─────────────────────────────────────────────────────────────────────────────

static constexpr int SECTION_GAP  = 10;
static constexpr int CELL_PAD_L   =  4;
static constexpr int CELL_PAD_T   =  2;
static constexpr int TABLE_MARGIN =  4;

// ─────────────────────────────────────────────────────────────────────────────
// WikiTownView – pre-rendered town background + structure overlays
// ─────────────────────────────────────────────────────────────────────────────

class WikiTownView : public CIntObject
{
	std::shared_ptr<CanvasImage> offscreen;
	Point scaledSize;

public:
	WikiTownView(const CTown * town, int vpW, int posY)
		: CIntObject(0, Point(0, posY))
	{
		if(!town)
			return;
		if(town->clientInfo.townBackground.getName().empty())
			return;

		auto bgImg = ENGINE->renderHandler().loadImage(
			town->clientInfo.townBackground, EImageBlitMode::OPAQUE);
		if(!bgImg)
			return;

		const Point naturalSize = bgImg->dimensions();
		if(naturalSize.x <= 0 || naturalSize.y <= 0)
			return;

		const double scale = (double)vpW / naturalSize.x;
		scaledSize = Point(vpW, std::max(1, (int)std::round(naturalSize.y * scale)));

		offscreen = ENGINE->renderHandler().createImage(
			naturalSize, CanvasScalingPolicy::IGNORE);
		Canvas c = offscreen->getCanvas();
		c.draw(bgImg, {0, 0});

		// Mirrors CCastleInterface::recreate() logic for structure selection
		std::vector<const CStructure *> toRender;
		std::map<BuildingID, std::vector<const CStructure *>> groups;

		for(const auto & structure : town->clientInfo.structures)
		{
			if(!structure || structure->defName.empty())
				continue;
			if(!structure->building)
			{
				toRender.push_back(structure.get());
				continue;
			}
			groups[structure->building->getBase()].push_back(structure.get());
		}

		for(auto & [baseId, group] : groups)
		{
			auto it = town->buildings.find(baseId);
			if(it == town->buildings.end())
				continue;
			const CBuilding * base = it->second.get();
			const CStructure * best = *std::max_element(
				group.begin(), group.end(),
				[base](const CStructure * a, const CStructure * b)
				{
					return base->getDistance(a->building->bid)
					     < base->getDistance(b->building->bid);
				});
			toRender.push_back(best);
		}

		std::sort(toRender.begin(), toRender.end(),
			[](const CStructure * a, const CStructure * b){ return a->pos.z < b->pos.z; });

		for(const CStructure * structure : toRender)
		{
			auto sImg = ENGINE->renderHandler().loadImage(
				structure->defName, 0, 0, EImageBlitMode::COLORKEY);
			if(sImg)
				c.draw(sImg, Point(structure->pos.x, structure->pos.y));
		}

		pos.w = scaledSize.x;
		pos.h = scaledSize.y;
	}

	int height() const { return scaledSize.y; }

	void showAll(Canvas & to) override
	{
		if(!offscreen || scaledSize.x <= 0 || scaledSize.y <= 0)
			return;
		Canvas src = offscreen->getCanvas();
		to.drawScaled(src, pos.topLeft(), scaledSize);
	}
};

// ─────────────────────────────────────────────────────────────────────────────
// Icon dimension helper
// ─────────────────────────────────────────────────────────────────────────────

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

// ─────────────────────────────────────────────────────────────────────────────
// Section builders – each advances curY by the height of its content
// ─────────────────────────────────────────────────────────────────────────────

static void addTownTitle(
	std::vector<std::shared_ptr<CIntObject>> & widgets,
	const CFaction * faction, int W, int & curY)
{
	widgets.push_back(std::make_shared<CLabel>(
		W / 2, curY,
		FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW,
		faction->getNameTranslated()));
	curY += 24;

	// Alignment subtitle
	{
		std::string alignStr;
		ColorRGBA alignCol = Colors::WHITE;
		switch(faction->getAlignment())
		{
			case EAlignment::GOOD:    alignStr = LIBRARY->generaltexth->translate("vcmi.wiki.alignment.good");    alignCol = ColorRGBA(0,   200,  0,  255); break;
			case EAlignment::EVIL:    alignStr = LIBRARY->generaltexth->translate("vcmi.wiki.alignment.evil");    alignCol = ColorRGBA(220,  50, 50, 255); break;
			case EAlignment::NEUTRAL: alignStr = LIBRARY->generaltexth->translate("vcmi.wiki.alignment.neutral"); alignCol = Colors::WHITE;               break;
			default: break;
		}
		if(!alignStr.empty())
		{
			widgets.push_back(std::make_shared<CLabel>(
				W / 2, curY,
				FONT_SMALL, ETextAlignment::CENTER, alignCol, alignStr));
			curY += 16;
		}
	}

	auto townView = std::make_shared<WikiTownView>(faction->town.get(), W, curY);
	curY += townView->height() + SECTION_GAP;
	widgets.push_back(std::move(townView));

	const std::string desc = faction->getDescriptionTranslated();
	if(!desc.empty())
	{
		auto label = std::make_shared<CMultiLineLabel>(
			Rect(TABLE_MARGIN, curY, W - TABLE_MARGIN * 2, 4000),
			FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, desc);
		label->pos.h = label->textSize.y;
		curY += label->textSize.y + SECTION_GAP;
		widgets.push_back(std::move(label));
	}
}

static void addBuildingsTable(
	std::vector<std::shared_ptr<CIntObject>> & widgets,
	const CTown * town, int W, bool blueStyle,
	const Rect & clipRect, int & curY)
{
	curY += 8;
	widgets.push_back(std::make_shared<CLabel>(
		W / 2, curY,
		FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
		LIBRARY->generaltexth->translate("vcmi.wiki.town.buildings")));
	curY += 20;

	const Point iconSz = iconDimensions(town->clientInfo.buildingsIcons);
	const int rowH    = iconSz.y + 4;
	const int headerH = 18;
	const int tableW  = W - TABLE_MARGIN * 2;
	const int colIcon = iconSz.x + 4;
	const int colHalf = (tableW - colIcon) / 2;
	const std::vector<int> cols = {colIcon, colHalf, tableW - colIcon - colHalf};

	std::vector<const CBuilding *> bldRows;
	for(const auto & [bid, bld] : town->buildings)
	{
		if(!bld) continue;
		if(bld->mode == CBuilding::BUILD_AUTO || bld->mode == CBuilding::BUILD_SPECIAL) continue;
		bldRows.push_back(bld.get());
	}
	std::sort(bldRows.begin(), bldRows.end(),
		[](const CBuilding * a, const CBuilding * b){ return a->bid < b->bid; });

	widgets.push_back(std::make_shared<WikiTableGrid>(
		TABLE_MARGIN, curY, tableW, cols,
		headerH, rowH, (int)bldRows.size(), blueStyle));

	widgets.push_back(std::make_shared<CLabel>(
		TABLE_MARGIN + colIcon + CELL_PAD_L, curY + CELL_PAD_T,
		FONT_TINY, ETextAlignment::TOPLEFT, Colors::YELLOW,
		LIBRARY->generaltexth->translate("vcmi.wiki.town.buildingName")));
	widgets.push_back(std::make_shared<CLabel>(
		TABLE_MARGIN + colIcon + colHalf + CELL_PAD_L, curY + CELL_PAD_T,
		FONT_TINY, ETextAlignment::TOPLEFT, Colors::YELLOW,
		LIBRARY->generaltexth->translate("vcmi.wiki.town.cost")));
	curY += headerH;

	for(const CBuilding * bld : bldRows)
	{
		if(!town->clientInfo.buildingsIcons.getName().empty())
			widgets.push_back(std::make_shared<CAnimImage>(
				town->clientInfo.buildingsIcons,
				bld->bid.getNum(), 0,
				TABLE_MARGIN + 2, curY + 2));

		widgets.push_back(std::make_shared<CLabel>(
			TABLE_MARGIN + colIcon + CELL_PAD_L, curY + CELL_PAD_T,
			FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
			bld->getNameTranslated()));

		widgets.push_back(std::make_shared<WikiResourceCost>(bld->resources,
			TABLE_MARGIN + colIcon + colHalf + CELL_PAD_L,
			curY + CELL_PAD_T,
			cols[2] - CELL_PAD_L * 2));

		std::string desc = bld->getDescriptionTranslated();
		if(!desc.empty())
			desc = "{" + bld->getNameTranslated() + "}\n\n" + desc;
		std::function<void()> rclick;
		if(!desc.empty())
			rclick = [desc](){ CRClickPopup::createAndPush(desc); };
		widgets.push_back(std::make_shared<WikiClickable>(
			Rect(TABLE_MARGIN, curY, tableW, rowH),
			nullptr, std::move(rclick), blueStyle, clipRect));

		curY += rowH;
	}
}

static void addCreaturesTable(
	std::vector<std::shared_ptr<CIntObject>> & widgets,
	const CTown * town, int W, bool blueStyle,
	const Rect & clipRect,
	const WikiNavigateCallback & navigateCallback,
	int & curY)
{
	curY += 8;
	widgets.push_back(std::make_shared<CLabel>(
		W / 2, curY,
		FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
		LIBRARY->generaltexth->translate("vcmi.wiki.town.creatures")));
	curY += 20;

	const Point iconSz = iconDimensions(AnimationPath::builtin("CPRSMALL"));
	const int rowH    = iconSz.y + 4;
	const int headerH = 18;
	const int tableW  = W - TABLE_MARGIN * 2;
	const int colIcon = iconSz.x + 4;
	const int statW   = 26;
	const int numStats = 8;
	const int costW   = 96;
	const int nameW   = tableW - colIcon - costW - statW * numStats;
	const std::vector<int> cols = {
		colIcon, nameW, costW,
		statW, statW, statW, statW, statW, statW, statW, statW
	};

	struct CreatureRow { const CCreature * creature; int tier; bool isUpgrade; };
	std::vector<CreatureRow> crRows;
	for(int tier = 0; tier < (int)town->creatures.size(); ++tier)
		for(int ci = 0; ci < (int)town->creatures[tier].size(); ++ci)
		{
			const CCreature * cr =
				LIBRARY->creh->objects[town->creatures[tier][ci]].get();
			if(cr)
				crRows.push_back({cr, tier + 1, ci > 0});
		}

	widgets.push_back(std::make_shared<WikiTableGrid>(
		TABLE_MARGIN, curY, tableW, cols,
		headerH, rowH, (int)crRows.size(), blueStyle));

	{
		int hx = TABLE_MARGIN + colIcon;
		widgets.push_back(std::make_shared<CLabel>(
			hx + CELL_PAD_L, curY + CELL_PAD_T,
			FONT_TINY, ETextAlignment::TOPLEFT, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.wiki.town.creatureName")));
		hx += nameW;
		widgets.push_back(std::make_shared<CLabel>(
			hx + CELL_PAD_L, curY + CELL_PAD_T,
			FONT_TINY, ETextAlignment::TOPLEFT, Colors::YELLOW,
			LIBRARY->generaltexth->translate("vcmi.wiki.creature.cost")));
		hx += costW;
		static const std::vector<std::pair<std::string, std::string>> statIconDefs = {
			{"stackWindow/iconLevel",   "vcmi.wiki.creature.stat.level"},
			{"stackWindow/iconAttack",  "core.genrltxt.190"},
			{"stackWindow/iconDefense", "core.genrltxt.191"},
			{"stackWindow/iconDamage",  "core.genrltxt.199"},
			{"stackWindow/iconSpeed",   "core.genrltxt.193"},
			{"stackWindow/iconHealth",  "core.genrltxt.388"},
			{"stackWindow/iconGrowth",  "core.genrltxt.194"},
			{"stackWindow/iconAI",      "vcmi.wiki.creature.stat.aivalue"},
		};
		for(const auto & [iconName, descKey] : statIconDefs)
		{
			widgets.push_back(std::make_shared<CPicture>(
				ImagePath::builtin(iconName),
				hx + (statW - 17) / 2,
				curY + (headerH - 19) / 2));
			const std::string desc = LIBRARY->generaltexth->translate(descKey);
			widgets.push_back(std::make_shared<WikiClickable>(
				Rect(hx, curY, statW, headerH),
				nullptr,
				[desc](){ CRClickPopup::createAndPush(desc); },
				blueStyle,
				clipRect));
			hx += statW;
		}
	}
	curY += headerH;

	for(const auto & row : crRows)
	{
		widgets.push_back(std::make_shared<CAnimImage>(
			AnimationPath::builtin("CPRSMALL"),
			row.creature->getIconIndex(), 0,
			TABLE_MARGIN + 2, curY + 2));

		const std::string tierStr = row.isUpgrade
			? "  +  "
			: ("T" + std::to_string(row.creature->getLevel()) + ": ");
		widgets.push_back(std::make_shared<CLabel>(
			TABLE_MARGIN + colIcon + CELL_PAD_L, curY + CELL_PAD_T,
			FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
			tierStr + row.creature->getNameSingularTranslated()));

		widgets.push_back(std::make_shared<WikiResourceCost>(row.creature->getFullRecruitCost(),
			TABLE_MARGIN + colIcon + nameW + CELL_PAD_L,
			curY + CELL_PAD_T,
			costW - CELL_PAD_L * 2));

		{
			int sx = TABLE_MARGIN + colIcon + nameW + costW;
			const auto * cr = row.creature;
			// Use a hypothetical CStackInstance to evaluate limiters correctly (e.g. conditional rune bonuses)
			CStackInstance fakeStack(nullptr, cr->getId(), 1, true);
			const bool shooter = fakeStack.hasBonusOfType(BonusType::SHOOTER) && fakeStack.valOfBonuses(BonusType::SHOTS);
			const int dmgMin = fakeStack.getMinDamage(shooter);
			const int dmgMax = fakeStack.getMaxDamage(shooter);
			const std::string dmgStr = (dmgMin == dmgMax)
				? std::to_string(dmgMin)
				: (std::to_string(dmgMin) + "-" + std::to_string(dmgMax));
			for(const auto & val : std::vector<std::string>{
					std::to_string(cr->getLevel()),
					std::to_string(fakeStack.getAttack(shooter)),
					std::to_string(fakeStack.getDefense(shooter)),
					dmgStr,
					std::to_string(fakeStack.getMovementRange()),
					std::to_string(fakeStack.getMaxHealth()),
					std::to_string(cr->getGrowth()),
					std::to_string(cr->getAIValue())})
			{
				widgets.push_back(std::make_shared<CLabel>(
					sx + statW / 2, curY + CELL_PAD_T,
					FONT_TINY, ETextAlignment::TOPCENTER, Colors::WHITE, val));
				sx += statW;
			}
		}

		{
			std::function<void()> lclick;
			if(navigateCallback)
			{
				const std::string crId = row.creature->getJsonKey();
				lclick = [navigateCallback, crId]()
					{ navigateCallback(WikiCategory::CREATURE, crId); };
			}
			const CCreature * crPtr = row.creature;
			widgets.push_back(std::make_shared<WikiClickable>(
				Rect(TABLE_MARGIN, curY, tableW, rowH),
				std::move(lclick),
				[crPtr](){ ENGINE->windows().createAndPushWindow<CStackWindow>(crPtr, true); },
				blueStyle, clipRect));
		}
		curY += rowH;
	}
}

static void addHeroesTable(
	std::vector<std::shared_ptr<CIntObject>> & widgets,
	const CFaction * faction, int W, bool blueStyle,
	const Rect & clipRect,
	const WikiNavigateCallback & navigateCallback,
	int & curY)
{
	const FactionID factionId = faction->getId();

	struct HeroData { const CHero * hero; std::string className; };
	std::vector<HeroData> mightHeroes, magicHeroes;

	for(const auto & h : LIBRARY->heroh->objects)
	{
		if(!h || h->special || !h->heroClass) continue;
		if(h->heroClass->faction != factionId) continue;
		HeroData hd{h.get(), h->heroClass->getNameTranslated()};
		if(h->heroClass->affinity == CHeroClass::MAGIC)
			magicHeroes.push_back(hd);
		else
			mightHeroes.push_back(hd);
	}

	if(mightHeroes.empty() && magicHeroes.empty())
		return;

	curY += 8;
	widgets.push_back(std::make_shared<CLabel>(
		W / 2, curY,
		FONT_MEDIUM, ETextAlignment::CENTER, Colors::YELLOW,
		LIBRARY->generaltexth->translate("vcmi.wiki.town.heroes")));
	curY += 20;

	const int tableW2   = W - TABLE_MARGIN * 2;
	const int colPort   = 52;
	const int colGend   = 22;
	const int colSpecIcon = 34;  ///< specialty icon (UN44 scaled to 30×30)
	const int colSpec2  = 136;   ///< specialty name text (was 170, minus icon col)
	const int colName2  = tableW2 - colPort - colGend - colSpecIcon - colSpec2;
	const std::vector<int> heroCols = {colPort, colName2, colGend, colSpecIcon, colSpec2};

	// Gender: ♂/♀ Unicode glyphs if supported, else translatable fallback
	auto genderStr = [](const CHero * h) -> std::string {
		return wikiGenderSpan(h->gender == EHeroGender::FEMALE);
	};
	const int heroHeaderH = 18;
	const int heroRowH    = 36;

	auto classNamesStr = [](const std::vector<HeroData> & rows) -> std::string
	{
		std::string result;
		std::vector<std::string> seen;
		for(const auto & hd : rows)
		{
			if(std::find(seen.begin(), seen.end(), hd.className) == seen.end())
			{
				if(!result.empty()) result += " / ";
				result += hd.className;
				seen.push_back(hd.className);
			}
		}
		return result;
	};

	auto renderHeroGroup = [&](const std::vector<HeroData> & rows, const std::string & header)
	{
		if(rows.empty()) return;

		widgets.push_back(std::make_shared<WikiTableGrid>(
			TABLE_MARGIN, curY, tableW2, heroCols,
			heroHeaderH, heroRowH, (int)rows.size(), blueStyle));
		widgets.push_back(std::make_shared<CLabel>(
			TABLE_MARGIN + tableW2 / 2, curY + CELL_PAD_T,
			FONT_TINY, ETextAlignment::TOPCENTER, Colors::YELLOW, header));
		curY += heroHeaderH;

		for(const auto & hd : rows)
		{
			const CHero * h = hd.hero;

			widgets.push_back(std::make_shared<CAnimImage>(
				AnimationPath::builtin("PortraitsSmall"),
				h->imageIndex, 0,
				TABLE_MARGIN + 2, curY + 2));
			widgets.push_back(std::make_shared<CLabel>(
				TABLE_MARGIN + colPort + CELL_PAD_L, curY + CELL_PAD_T,
				FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
				h->getNameTranslated()));
			widgets.push_back(std::make_shared<CLabel>(
				TABLE_MARGIN + colPort + colName2 + colGend / 2, curY + CELL_PAD_T,
				FONT_SMALL, ETextAlignment::TOPCENTER, Colors::WHITE,
				genderStr(h)));

			// Specialty icon (UN44, scaled to fit the row)
			const int specIconSz = heroRowH - 4;
			widgets.push_back(std::make_shared<CAnimImage>(
				AnimationPath::builtin("UN44"),
				h->imageIndex,
				Rect(TABLE_MARGIN + colPort + colName2 + colGend + 2,
				     curY + 2, specIconSz, specIconSz),
				0));

			widgets.push_back(std::make_shared<CMultiLineLabel>(
				Rect(TABLE_MARGIN + colPort + colName2 + colGend + colSpecIcon + CELL_PAD_L,
				     curY + CELL_PAD_T,
				     colSpec2 - CELL_PAD_L * 2,
				     heroRowH - CELL_PAD_T * 2),
				FONT_TINY, ETextAlignment::TOPLEFT, Colors::WHITE,
				h->getSpecialtyNameTranslated()));

			std::function<void()> lclick;
			if(navigateCallback)
			{
				const std::string heroJsonKey = h->getJsonKey();
				lclick = [navigateCallback, heroJsonKey]()
					{ navigateCallback(WikiCategory::HERO, heroJsonKey); };
			}
			const HeroTypeID hId = h->getId();
			widgets.push_back(std::make_shared<WikiClickable>(
				Rect(TABLE_MARGIN, curY, tableW2, heroRowH),
				std::move(lclick),
				[hId](){ ENGINE->windows().createAndPushWindow<CHeroOverview>(hId); },
				blueStyle, clipRect));

			curY += heroRowH;
		}
	};

	renderHeroGroup(mightHeroes, classNamesStr(mightHeroes));
	curY += SECTION_GAP;
	renderHeroGroup(magicHeroes, classNamesStr(magicHeroes));
}

// ─────────────────────────────────────────────────────────────────────────────
// buildTownContent – public entry point
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::shared_ptr<CIntObject>> buildTownContent(
	CViewport & viewport,
	const CFaction * faction,
	int viewportWidth,
	bool blueStyle,
	WikiNavigateCallback navigateCallback)
{
	if(!faction)
		throw std::runtime_error("buildTownContent: null faction");
	if(!faction->hasTown())
		throw std::runtime_error("buildTownContent: faction has no town");
	if(!faction->town)
		throw std::runtime_error("buildTownContent: faction->town is null");

	std::vector<std::shared_ptr<CIntObject>> widgets;

	OBJECT_CONSTRUCTION_TARGETED(viewport.content());
	const Rect clipRect = viewport.clipRect();
	const int W = viewportWidth;
	int curY = 12;

	addTownTitle(widgets, faction, W, curY);

	curY += SECTION_GAP;
	addBuildingsTable(widgets, faction->town.get(), W, blueStyle, clipRect, curY);

	curY += SECTION_GAP;
	addCreaturesTable(widgets, faction->town.get(), W, blueStyle, clipRect, navigateCallback, curY);

	curY += SECTION_GAP;
	addHeroesTable(widgets, faction, W, blueStyle, clipRect, navigateCallback, curY);

	return widgets;
}
