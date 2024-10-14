/*
 * AssetGenerator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AssetGenerator.h"

#include "../gui/CGuiHandler.h"
#include "../render/IImage.h"
#include "../render/IImageLoader.h"
#include "../render/Canvas.h"
#include "../render/ColorFilter.h"
#include "../render/IRenderHandler.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/GameSettings.h"
#include "../lib/IGameSettings.h"
#include "../lib/json/JsonNode.h"
#include "../lib/VCMI_Lib.h"

void AssetGenerator::generateAll()
{
	createBigSpellBook();
	createAdventureOptionsCleanBackground();
	for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
		createPlayerColoredBackground(PlayerColor(i));
	createCombatUnitNumberWindow();
}

void AssetGenerator::createAdventureOptionsCleanBackground()
{
	std::string filename = "data/AdventureOptionsBackgroundClear.png";

	if(CResourceHandler::get()->existsResource(ResourcePath(filename))) // overridden by mod, no generation
		return;

	if(!CResourceHandler::get("local")->createResource(filename))
		return;
	ResourcePath savePath(filename, EResType::IMAGE);

	auto locator = ImageLocator(ImagePath::builtin("ADVOPTBK"));
	locator.scalingFactor = 1;

	std::shared_ptr<IImage> img = GH.renderHandler().loadImage(locator, EImageBlitMode::OPAQUE);

	Canvas canvas = Canvas(Point(575, 585), CanvasScalingPolicy::IGNORE);
	canvas.draw(img, Point(0, 0), Rect(0, 0, 575, 585));
	canvas.draw(img, Point(54, 121), Rect(54, 123, 335, 1));
	canvas.draw(img, Point(158, 84), Rect(156, 84, 2, 37));
	canvas.draw(img, Point(234, 84), Rect(232, 84, 2, 37));
	canvas.draw(img, Point(310, 84), Rect(308, 84, 2, 37));
	canvas.draw(img, Point(53, 567), Rect(53, 520, 339, 3));
	canvas.draw(img, Point(53, 520), Rect(53, 264, 339, 47));

	std::shared_ptr<IImage> image = GH.renderHandler().createImage(canvas.getInternalSurface());

	image->exportBitmap(*CResourceHandler::get("local")->getResourceName(savePath));
}

void AssetGenerator::createBigSpellBook()
{
	std::string filename = "data/SpellBookLarge.png";

	if(CResourceHandler::get()->existsResource(ResourcePath(filename))) // overridden by mod, no generation
		return;

	if(!CResourceHandler::get("local")->createResource(filename))
		return;
	ResourcePath savePath(filename, EResType::IMAGE);

	auto locator = ImageLocator(ImagePath::builtin("SpelBack"));
	locator.scalingFactor = 1;

	std::shared_ptr<IImage> img = GH.renderHandler().loadImage(locator, EImageBlitMode::OPAQUE);
	Canvas canvas = Canvas(Point(800, 600), CanvasScalingPolicy::IGNORE);
	// edges
	canvas.draw(img, Point(0, 0), Rect(15, 38, 90, 45));
	canvas.draw(img, Point(0, 460), Rect(15, 400, 90, 141));
	canvas.draw(img, Point(705, 0), Rect(509, 38, 95, 45));
	canvas.draw(img, Point(705, 460), Rect(509, 400, 95, 141));
	// left / right
	Canvas tmp1 = Canvas(Point(90, 355 - 45), CanvasScalingPolicy::IGNORE);
	tmp1.draw(img, Point(0, 0), Rect(15, 38 + 45, 90, 355 - 45));
	canvas.drawScaled(tmp1, Point(0, 45), Point(90, 415));
	Canvas tmp2 = Canvas(Point(95, 355 - 45), CanvasScalingPolicy::IGNORE);
	tmp2.draw(img, Point(0, 0), Rect(509, 38 + 45, 95, 355 - 45));
	canvas.drawScaled(tmp2, Point(705, 45), Point(95, 415));
	// top / bottom
	Canvas tmp3 = Canvas(Point(409, 45), CanvasScalingPolicy::IGNORE);
	tmp3.draw(img, Point(0, 0), Rect(100, 38, 409, 45));
	canvas.drawScaled(tmp3, Point(90, 0), Point(615, 45));
	Canvas tmp4 = Canvas(Point(409, 141), CanvasScalingPolicy::IGNORE);
	tmp4.draw(img, Point(0, 0), Rect(100, 400, 409, 141));
	canvas.drawScaled(tmp4, Point(90, 460), Point(615, 141));
	// middle
	Canvas tmp5 = Canvas(Point(409, 141), CanvasScalingPolicy::IGNORE);
	tmp5.draw(img, Point(0, 0), Rect(100, 38 + 45, 509 - 15, 400 - 38));
	canvas.drawScaled(tmp5, Point(90, 45), Point(615, 415));
	// carpet
	Canvas tmp6 = Canvas(Point(590, 59), CanvasScalingPolicy::IGNORE);
	tmp6.draw(img, Point(0, 0), Rect(15, 484, 590, 59));
	canvas.drawScaled(tmp6, Point(0, 545), Point(800, 59));
	// remove bookmarks
	for (int i = 0; i < 56; i++)
		canvas.draw(Canvas(canvas, Rect(i < 30 ? 268 : 327, 464, 1, 46)), Point(269 + i, 464));
	for (int i = 0; i < 56; i++)
		canvas.draw(Canvas(canvas, Rect(469, 464, 1, 42)), Point(470 + i, 464));
	for (int i = 0; i < 57; i++)
		canvas.draw(Canvas(canvas, Rect(i < 30 ? 564 : 630, 464, 1, 44)), Point(565 + i, 464));
	for (int i = 0; i < 56; i++)
		canvas.draw(Canvas(canvas, Rect(656, 464, 1, 47)), Point(657 + i, 464));
	// draw bookmarks
	canvas.draw(img, Point(278, 464), Rect(220, 405, 37, 47));
	canvas.draw(img, Point(481, 465), Rect(354, 406, 37, 41));
	canvas.draw(img, Point(575, 465), Rect(417, 406, 37, 45));
	canvas.draw(img, Point(667, 465), Rect(478, 406, 37, 47));

	std::shared_ptr<IImage> image = GH.renderHandler().createImage(canvas.getInternalSurface());

	image->exportBitmap(*CResourceHandler::get("local")->getResourceName(savePath));
}

void AssetGenerator::createPlayerColoredBackground(const PlayerColor & player)
{
	std::string filename = "data/DialogBoxBackground_" + player.toString() + ".png";

	if(CResourceHandler::get()->existsResource(ResourcePath(filename))) // overridden by mod, no generation
		return;

	if(!CResourceHandler::get("local")->createResource(filename))
		return;

	ResourcePath savePath(filename, EResType::IMAGE);

	auto locator = ImageLocator(ImagePath::builtin("DiBoxBck"));
	locator.scalingFactor = 1;

	std::shared_ptr<IImage> texture = GH.renderHandler().loadImage(locator, EImageBlitMode::OPAQUE);

	// transform to make color of brown DIBOX.PCX texture match color of specified player
	auto filterSettings = VLC->settingsHandler->getFullConfig()["interface"]["playerColoredBackground"];
	static const std::array<ColorFilter, PlayerColor::PLAYER_LIMIT_I> filters = {
		ColorFilter::genRangeShifter( filterSettings["red"   ].convertTo<std::vector<float>>() ),
		ColorFilter::genRangeShifter( filterSettings["blue"  ].convertTo<std::vector<float>>() ),
		ColorFilter::genRangeShifter( filterSettings["tan"   ].convertTo<std::vector<float>>() ),
		ColorFilter::genRangeShifter( filterSettings["green" ].convertTo<std::vector<float>>() ),
		ColorFilter::genRangeShifter( filterSettings["orange"].convertTo<std::vector<float>>() ),
		ColorFilter::genRangeShifter( filterSettings["purple"].convertTo<std::vector<float>>() ),
		ColorFilter::genRangeShifter( filterSettings["teal"  ].convertTo<std::vector<float>>() ),
		ColorFilter::genRangeShifter( filterSettings["pink"  ].convertTo<std::vector<float>>() )
	};

	assert(player.isValidPlayer());
	if (!player.isValidPlayer())
	{
		logGlobal->error("Unable to colorize to invalid player color %d!", player.getNum());
		return;
	}

	texture->adjustPalette(filters[player.getNum()], 0);
	texture->exportBitmap(*CResourceHandler::get("local")->getResourceName(savePath));
}

void AssetGenerator::createCombatUnitNumberWindow()
{
	std::string filenameToSave = "data/combatUnitNumberWindow";

	ResourcePath savePathDefault(filenameToSave + "Default.png", EResType::IMAGE);
	ResourcePath savePathNeutral(filenameToSave + "Neutral.png", EResType::IMAGE);
	ResourcePath savePathPositive(filenameToSave + "Positive.png", EResType::IMAGE);
	ResourcePath savePathNegative(filenameToSave + "Negative.png", EResType::IMAGE);

	if(CResourceHandler::get()->existsResource(savePathDefault)) // overridden by mod, no generation
		return;

	if(!CResourceHandler::get("local")->createResource(savePathDefault.getOriginalName() + ".png") ||
	   !CResourceHandler::get("local")->createResource(savePathNeutral.getOriginalName() + ".png") ||
	   !CResourceHandler::get("local")->createResource(savePathPositive.getOriginalName() + ".png") ||
	   !CResourceHandler::get("local")->createResource(savePathNegative.getOriginalName() + ".png"))
		return;

	auto locator = ImageLocator(ImagePath::builtin("CMNUMWIN"));
	locator.scalingFactor = 1;

	std::shared_ptr<IImage> texture = GH.renderHandler().loadImage(locator, EImageBlitMode::OPAQUE);

	static const auto shifterNormal   = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 0.6f, 0.2f, 1.0f );
	static const auto shifterPositive = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 0.2f, 1.0f, 0.2f );
	static const auto shifterNegative = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 1.0f, 0.2f, 0.2f );
	static const auto shifterNeutral  = ColorFilter::genRangeShifter( 0.f, 0.f, 0.f, 1.0f, 1.0f, 0.2f );

	// do not change border color
	static const int32_t ignoredMask = 1 << 26;

	texture->adjustPalette(shifterNormal, ignoredMask);
	texture->exportBitmap(*CResourceHandler::get("local")->getResourceName(savePathDefault));
	texture->adjustPalette(shifterPositive, ignoredMask);
	texture->exportBitmap(*CResourceHandler::get("local")->getResourceName(savePathPositive));
	texture->adjustPalette(shifterNegative, ignoredMask);
	texture->exportBitmap(*CResourceHandler::get("local")->getResourceName(savePathNegative));
	texture->adjustPalette(shifterNeutral, ignoredMask);
	texture->exportBitmap(*CResourceHandler::get("local")->getResourceName(savePathNeutral));
}
