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
#include "../render/CAnimation.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/GameSettings.h"
#include "../lib/IGameSettings.h"
#include "../lib/json/JsonNode.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/RiverHandler.h"
#include "../lib/RoadHandler.h"
#include "../lib/TerrainHandler.h"

void AssetGenerator::generateAll()
{
	createBigSpellBook();
	createAdventureOptionsCleanBackground();
	for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
		createPlayerColoredBackground(PlayerColor(i));
	createCombatUnitNumberWindow();
	createCampaignBackground();
	createChroniclesCampaignImages();
	createPaletteShiftedSprites();
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

void AssetGenerator::createCampaignBackground()
{
	std::string filename = "data/CampaignBackground8.png";

	if(CResourceHandler::get()->existsResource(ResourcePath(filename))) // overridden by mod, no generation
		return;

	if(!CResourceHandler::get("local")->createResource(filename))
		return;
	ResourcePath savePath(filename, EResType::IMAGE);

	auto locator = ImageLocator(ImagePath::builtin("CAMPBACK"));
	locator.scalingFactor = 1;

	std::shared_ptr<IImage> img = GH.renderHandler().loadImage(locator, EImageBlitMode::OPAQUE);
	Canvas canvas = Canvas(Point(800, 600), CanvasScalingPolicy::IGNORE);
	
	canvas.draw(img, Point(0, 0), Rect(0, 0, 800, 600));

	// left image
	canvas.draw(img, Point(220, 73), Rect(290, 73, 141, 115));
	canvas.draw(img, Point(37, 70), Rect(87, 70, 207, 120));

	// right image
	canvas.draw(img, Point(513, 67), Rect(463, 67, 71, 126));
	canvas.draw(img, Point(586, 71), Rect(536, 71, 207, 117));

	// middle image
	canvas.draw(img, Point(306, 68), Rect(86, 68, 209, 122));

	// disabled fields
	canvas.draw(img, Point(40, 72), Rect(313, 74, 197, 114));
	canvas.draw(img, Point(310, 72), Rect(313, 74, 197, 114));
	canvas.draw(img, Point(590, 72), Rect(313, 74, 197, 114));
	canvas.draw(img, Point(43, 245), Rect(313, 74, 197, 114));
	canvas.draw(img, Point(313, 244), Rect(313, 74, 197, 114));
	canvas.draw(img, Point(586, 246), Rect(313, 74, 197, 114));
	canvas.draw(img, Point(34, 417), Rect(313, 74, 197, 114));
	canvas.draw(img, Point(404, 414), Rect(313, 74, 197, 114));

	// skull
	auto locatorSkull = ImageLocator(ImagePath::builtin("CAMPNOSC"));
	locatorSkull.scalingFactor = 1;
	std::shared_ptr<IImage> imgSkull = GH.renderHandler().loadImage(locatorSkull, EImageBlitMode::OPAQUE);
	canvas.draw(imgSkull, Point(562, 509), Rect(178, 108, 43, 19));

	std::shared_ptr<IImage> image = GH.renderHandler().createImage(canvas.getInternalSurface());

	image->exportBitmap(*CResourceHandler::get("local")->getResourceName(savePath));
}

void AssetGenerator::createChroniclesCampaignImages()
{
	for(int i = 1; i < 9; i++)
	{
		std::string filename = "data/CampaignHc" + std::to_string(i) + "Image.png";

		if(CResourceHandler::get()->existsResource(ResourcePath(filename))) // overridden by mod, no generation
			continue;
			
		auto imgPathBg = ImagePath::builtin("data/chronicles_" + std::to_string(i) + "/GamSelBk");
		if(!CResourceHandler::get()->existsResource(imgPathBg)) // Chronicle episode not installed
			continue;

		if(!CResourceHandler::get("local")->createResource(filename))
			continue;
		ResourcePath savePath(filename, EResType::IMAGE);

		auto locator = ImageLocator(imgPathBg);
		locator.scalingFactor = 1;

		std::shared_ptr<IImage> img = GH.renderHandler().loadImage(locator, EImageBlitMode::OPAQUE);
		Canvas canvas = Canvas(Point(200, 116), CanvasScalingPolicy::IGNORE);
		
		switch (i)
		{
		case 1:
			canvas.draw(img, Point(0, 0), Rect(149, 144, 200, 116));
			break;
		case 2:
			canvas.draw(img, Point(0, 0), Rect(156, 150, 200, 116));
			break;
		case 3:
			canvas.draw(img, Point(0, 0), Rect(171, 153, 200, 116));
			break;
		case 4:
			canvas.draw(img, Point(0, 0), Rect(35, 358, 200, 116));
			break;
		case 5:
			canvas.draw(img, Point(0, 0), Rect(216, 248, 200, 116));
			break;
		case 6:
			canvas.draw(img, Point(0, 0), Rect(58, 234, 200, 116));
			break;
		case 7:
			canvas.draw(img, Point(0, 0), Rect(184, 219, 200, 116));
			break;
		case 8:
			canvas.draw(img, Point(0, 0), Rect(268, 210, 200, 116));

			//skull
			auto locatorSkull = ImageLocator(ImagePath::builtin("CampSP1"));
			locatorSkull.scalingFactor = 1;
			std::shared_ptr<IImage> imgSkull = GH.renderHandler().loadImage(locatorSkull, EImageBlitMode::OPAQUE);
			canvas.draw(imgSkull, Point(162, 94), Rect(162, 94, 41, 22));
			canvas.draw(img, Point(162, 94), Rect(424, 304, 14, 4));
			canvas.draw(img, Point(162, 98), Rect(424, 308, 10, 4));
			canvas.draw(img, Point(158, 102), Rect(424, 312, 10, 4));
			canvas.draw(img, Point(154, 106), Rect(424, 316, 10, 4));
			break;
		}

		std::shared_ptr<IImage> image = GH.renderHandler().createImage(canvas.getInternalSurface());

		image->exportBitmap(*CResourceHandler::get("local")->getResourceName(savePath));
	}
}

void AssetGenerator::createPaletteShiftedSprites()
{
	std::vector<std::string> tiles;
	std::vector<std::vector<std::variant<TerrainPaletteAnimation, RiverPaletteAnimation>>> paletteAnimations;
	for(auto entity : VLC->terrainTypeHandler->objects)
	{
		if(entity->paletteAnimation.size())
		{
			tiles.push_back(entity->tilesFilename.getName());
			std::vector<std::variant<TerrainPaletteAnimation, RiverPaletteAnimation>> tmpAnim;
			for(auto & animEntity : entity->paletteAnimation)
				tmpAnim.push_back(animEntity);
			paletteAnimations.push_back(tmpAnim);
		}
	}
	for(auto entity : VLC->riverTypeHandler->objects)
	{
		if(entity->paletteAnimation.size())
		{
			tiles.push_back(entity->tilesFilename.getName());
			std::vector<std::variant<TerrainPaletteAnimation, RiverPaletteAnimation>> tmpAnim;
			for(auto & animEntity : entity->paletteAnimation)
				tmpAnim.push_back(animEntity);
			paletteAnimations.push_back(tmpAnim);
		}
	}

	for(int i = 0; i < tiles.size(); i++)
	{
		auto sprite = tiles[i];

		JsonNode config;
		config["basepath"].String() = sprite + "_Shifted/";
		config["images"].Vector();

		auto filename = AnimationPath::builtin(sprite).addPrefix("SPRITES/");
		auto filenameNew = AnimationPath::builtin(sprite + "_Shifted").addPrefix("SPRITES/");

		if(CResourceHandler::get()->existsResource(ResourcePath(filenameNew.getName(), EResType::JSON))) // overridden by mod, no generation
			return;
		
		auto anim = GH.renderHandler().loadAnimation(filename, EImageBlitMode::COLORKEY);
		for(int j = 0; j < anim->size(); j++)
		{
			int maxLen = 1;
			for(int k = 0; k < paletteAnimations[i].size(); k++)
			{
				auto element = paletteAnimations[i][k];
				if(std::holds_alternative<TerrainPaletteAnimation>(element))
					maxLen = std::lcm(maxLen, std::get<TerrainPaletteAnimation>(element).length);
				else
					maxLen = std::lcm(maxLen, std::get<RiverPaletteAnimation>(element).length);
			}
			for(int l = 0; l < maxLen; l++)
			{
				std::string spriteName = sprite + boost::str(boost::format("%02d") % j) + "_" + std::to_string(l) + ".png";
				std::string filenameNewImg = "sprites/" + sprite + "_Shifted" + "/" + spriteName;
				ResourcePath savePath(filenameNewImg, EResType::IMAGE);

				if(!CResourceHandler::get("local")->createResource(filenameNewImg))
					return;

				auto imgLoc = anim->getImageLocator(j, 0);
				imgLoc.scalingFactor = 1;
				auto img = GH.renderHandler().loadImage(imgLoc, EImageBlitMode::COLORKEY);
				for(int k = 0; k < paletteAnimations[i].size(); k++)
				{
					auto element = paletteAnimations[i][k];
					if(std::holds_alternative<TerrainPaletteAnimation>(element))
					{
						auto tmp = std::get<TerrainPaletteAnimation>(element);
						img->shiftPalette(tmp.start, tmp.length, l % tmp.length);
					}
					else
					{
						auto tmp = std::get<RiverPaletteAnimation>(element);
						img->shiftPalette(tmp.start, tmp.length, l % tmp.length);
					}
				}
				
				Canvas canvas = Canvas(Point(32, 32), CanvasScalingPolicy::IGNORE);
				canvas.draw(img, Point((32 - img->dimensions().x) / 2, (32 - img->dimensions().y) / 2));
				std::shared_ptr<IImage> image = GH.renderHandler().createImage(canvas.getInternalSurface());
				image->exportBitmap(*CResourceHandler::get("local")->getResourceName(savePath));

				JsonNode node(JsonMap{
					{ "group", JsonNode(l) },
					{ "frame", JsonNode(j) },
					{ "file", JsonNode(spriteName) }
				});
				config["images"].Vector().push_back(node);
			}
		}

		ResourcePath savePath(filenameNew.getOriginalName(), EResType::JSON);
		if(!CResourceHandler::get("local")->createResource(filenameNew.getOriginalName() + ".json"))
			return;

		std::fstream file(CResourceHandler::get("local")->getResourceName(savePath)->c_str(), std::ofstream::out | std::ofstream::trunc);
		file << config.toString();
	}
}
