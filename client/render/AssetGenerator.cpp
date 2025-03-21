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

#include "../GameEngine.h"
#include "../render/IImage.h"
#include "../render/IImageLoader.h"
#include "../render/Canvas.h"
#include "../render/CanvasImage.h"
#include "../render/ColorFilter.h"
#include "../render/IRenderHandler.h"
#include "../render/CAnimation.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/GameSettings.h"
#include "../lib/IGameSettings.h"
#include "../lib/json/JsonNode.h"
#include "../lib/VCMIDirs.h"
#include "../lib/GameLibrary.h"
#include "../lib/RiverHandler.h"
#include "../lib/RoadHandler.h"
#include "../lib/TerrainHandler.h"

void AssetGenerator::initialize()
{
	// clear to avoid non updated sprites after mod change (if base imnages are used)
	if(boost::filesystem::is_directory(VCMIDirs::get().userDataPath() / "Generated"))
		boost::filesystem::remove_all(VCMIDirs::get().userDataPath() / "Generated");

	imageFiles[ImagePath::builtin("AdventureOptionsBackgroundClear.png")] = [this](){ return createAdventureOptionsCleanBackground();};
	imageFiles[ImagePath::builtin("SpellBookLarge.png")] = [this](){ return createBigSpellBook();};

	imageFiles[ImagePath::builtin("combatUnitNumberWindowDefault.png")]  = [this](){ return createCombatUnitNumberWindow(0.6f, 0.2f, 1.0f);};
	imageFiles[ImagePath::builtin("combatUnitNumberWindowNeutral.png")]  = [this](){ return createCombatUnitNumberWindow(1.0f, 1.0f, 2.0f);};
	imageFiles[ImagePath::builtin("combatUnitNumberWindowPositive.png")] = [this](){ return createCombatUnitNumberWindow(0.2f, 1.0f, 0.2f);};
	imageFiles[ImagePath::builtin("combatUnitNumberWindowNegative.png")] = [this](){ return createCombatUnitNumberWindow(1.0f, 0.2f, 0.2f);};

	imageFiles[ImagePath::builtin("CampaignBackground4.png")] = [this]() { return createCampaignBackground(4); };
	imageFiles[ImagePath::builtin("CampaignBackground5.png")] = [this]() { return createCampaignBackground(5); };
	imageFiles[ImagePath::builtin("CampaignBackground6.png")] = [this]() { return createCampaignBackground(6); };
	imageFiles[ImagePath::builtin("CampaignBackground7.png")] = [this]() { return createCampaignBackground(7); };
	imageFiles[ImagePath::builtin("CampaignBackground8.png")] = [this]() { return createCampaignBackground(8); };

	for (PlayerColor color(0); color < PlayerColor::PLAYER_LIMIT; ++color)
		imageFiles[ImagePath::builtin("DialogBoxBackground_" + color.toString())] = [this, color](){ return createPlayerColoredBackground(color);};

	for(int i = 1; i < 9; i++)
		imageFiles[ImagePath::builtin("CampaignHc" + std::to_string(i) + "Image.png")] = [this, i](){ return createChroniclesCampaignImages(i);};

	createPaletteShiftedSprites();
}

std::shared_ptr<ISharedImage> AssetGenerator::generateImage(const ImagePath & image)
{
	if (imageFiles.count(image))
		return imageFiles.at(image)()->toSharedImage(); // TODO: cache?
	else
		return nullptr;
}

std::map<ImagePath, std::shared_ptr<ISharedImage>> AssetGenerator::generateAllImages()
{
	std::map<ImagePath, std::shared_ptr<ISharedImage>> result;

	for (const auto & entry : imageFiles)
		result[entry.first] = entry.second()->toSharedImage();

	return result;
}

std::map<AnimationPath, AssetGenerator::AnimationLayoutMap> AssetGenerator::generateAllAnimations()
{
	return animationFiles;
}

AssetGenerator::CanvasPtr AssetGenerator::createAdventureOptionsCleanBackground() const
{
	auto locator = ImageLocator(ImagePath::builtin("ADVOPTBK"), EImageBlitMode::OPAQUE);

	std::shared_ptr<IImage> img = ENGINE->renderHandler().loadImage(locator);

	auto image = ENGINE->renderHandler().createImage(Point(575, 585), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();

	canvas.draw(img, Point(0, 0), Rect(0, 0, 575, 585));
	canvas.draw(img, Point(54, 121), Rect(54, 123, 335, 1));
	canvas.draw(img, Point(158, 84), Rect(156, 84, 2, 37));
	canvas.draw(img, Point(234, 84), Rect(232, 84, 2, 37));
	canvas.draw(img, Point(310, 84), Rect(308, 84, 2, 37));
	canvas.draw(img, Point(53, 567), Rect(53, 520, 339, 3));
	canvas.draw(img, Point(53, 520), Rect(53, 264, 339, 47));

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createBigSpellBook() const
{
	auto locator = ImageLocator(ImagePath::builtin("SpelBack"), EImageBlitMode::OPAQUE);

	std::shared_ptr<IImage> img = ENGINE->renderHandler().loadImage(locator);
	auto image = ENGINE->renderHandler().createImage(Point(800, 600), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();
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

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createPlayerColoredBackground(const PlayerColor & player) const
{
	auto locator = ImageLocator(ImagePath::builtin("DiBoxBck"), EImageBlitMode::OPAQUE);

	std::shared_ptr<IImage> texture = ENGINE->renderHandler().loadImage(locator);

	// transform to make color of brown DIBOX.PCX texture match color of specified player
	auto filterSettings = LIBRARY->settingsHandler->getFullConfig()["interface"]["playerColoredBackground"];
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
		throw std::runtime_error("Unable to colorize to invalid player color" + std::to_string(player.getNum()));

	texture->adjustPalette(filters[player.getNum()], 0);

	auto image = ENGINE->renderHandler().createImage(texture->dimensions(), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();
	canvas.draw(texture, Point(0,0));

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createCombatUnitNumberWindow(float multR, float multG, float multB) const
{
	auto locator = ImageLocator(ImagePath::builtin("CMNUMWIN"), EImageBlitMode::OPAQUE);
	locator.layer = EImageBlitMode::OPAQUE;

	std::shared_ptr<IImage> texture = ENGINE->renderHandler().loadImage(locator);

	const auto shifter= ColorFilter::genRangeShifter(0.f, 0.f, 0.f, multR, multG, multB);

	// do not change border color
	static const int32_t ignoredMask = 1 << 26;

	texture->adjustPalette(shifter, ignoredMask);

	auto image = ENGINE->renderHandler().createImage(texture->dimensions(), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();
	canvas.draw(texture, Point(0,0));

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createCampaignBackground(int selection) const
{

	auto locator = ImageLocator(ImagePath::builtin("CAMPBACK"), EImageBlitMode::OPAQUE);
	std::shared_ptr<IImage> img = ENGINE->renderHandler().loadImage(locator);

	auto image = ENGINE->renderHandler().createImage(Point(800, 600), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();
	canvas.draw(img, Point(0, 0), Rect(0, 0, 800, 600));

	// BigBlock section
	auto bigBlock = ENGINE->renderHandler().createImage(Point(248, 114), CanvasScalingPolicy::IGNORE);
	Rect bigBlockRegion(292, 74, 248, 114);
	Canvas croppedBigBlock = bigBlock->getCanvas();
	croppedBigBlock.draw(img, Point(0, 0), bigBlockRegion);
	bigBlock->scaleTo(Point(200, 114), EScalingAlgorithm::NEAREST);

	// SmallBlock section
	auto smallBlock = ENGINE->renderHandler().createImage(Point(248, 114), CanvasScalingPolicy::IGNORE);
	Canvas croppedSmallBlock = smallBlock->getCanvas();
	croppedSmallBlock.draw(img, Point(0, 0), bigBlockRegion);
	smallBlock->scaleTo(Point(134, 114), EScalingAlgorithm::NEAREST);

	// Tripple block section
	auto trippleBlock = ENGINE->renderHandler().createImage(Point(72, 116), CanvasScalingPolicy::IGNORE);
	Rect trippleBlockSection(512, 246, 72, 116);
	Canvas croppedTrippleBlock = trippleBlock->getCanvas();
	croppedTrippleBlock.draw(img, Point(0, 0), trippleBlockSection);
	trippleBlock->scaleTo(Point(70, 114), EScalingAlgorithm::NEAREST);


	// First campaigns line
	if (selection > 7)
	{
		// Rebuild 1. campaigns line from 2 to 3 fields
		canvas.draw(bigBlock, Point(40, 72));
		canvas.draw(trippleBlock, Point(240, 73));
		canvas.draw(bigBlock, Point(310, 72));
		canvas.draw(trippleBlock, Point(510, 72));
		canvas.draw(bigBlock, Point(580, 72));
		canvas.draw(trippleBlock, Point(780, 72));
	} 
	else
	{
		// Empty 1 + 2. field
		canvas.draw(bigBlock, Point(90, 72));
		canvas.draw(bigBlock, Point(540, 72));
	}

	// Second campaigns line
	// 3. Field
	canvas.draw(bigBlock, Point(43, 245));

	if (selection == 4)
	{
		// Disabled 4. field
		canvas.draw(trippleBlock, Point(310, 245));
		canvas.draw(smallBlock, Point(380, 245));
	}
	else
	{
		// Empty 4. field
		canvas.draw(bigBlock, Point(314, 244));
	}
	
	// 5. Field
	canvas.draw(bigBlock, Point(586, 246));

	// Third campaigns line
	// 6. Field
	if (selection >= 6)
	{
		canvas.draw(bigBlock, Point(32, 417));
	}
	else
	{
		canvas.draw(trippleBlock, Point(30, 417));
		canvas.draw(smallBlock, Point(100, 417));
	}

	auto locatorSkull = ImageLocator(ImagePath::builtin("CAMPNOSC"), EImageBlitMode::OPAQUE);
	std::shared_ptr<IImage> imgSkull = ENGINE->renderHandler().loadImage(locatorSkull);

	if (selection >= 7)
	{
		// Only skull part
		canvas.draw(bigBlock, Point(404, 417));
		canvas.draw(imgSkull, Point(563, 512), Rect(178, 108, 43, 19));
	}
	else
	{
		// Original disabled field with skull and stone for 8. field
		imgSkull->scaleTo(Point(238, 150), EScalingAlgorithm::NEAREST);
		canvas.draw(imgSkull, Point(385, 400));
	}


	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createChroniclesCampaignImages(int chronicle) const
{
	auto imgPathBg = ImagePath::builtin("chronicles_" + std::to_string(chronicle) + "/GamSelBk");
	auto locator = ImageLocator(imgPathBg, EImageBlitMode::OPAQUE);

	std::shared_ptr<IImage> img = ENGINE->renderHandler().loadImage(locator);
	auto image = ENGINE->renderHandler().createImage(Point(200, 116), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();

	std::array sourceRect = {
		Rect(149, 144, 200, 116),
		Rect(156, 150, 200, 116),
		Rect(171, 153, 200, 116),
		Rect(35, 358, 200, 116),
		Rect(216, 248, 200, 116),
		Rect(58, 234, 200, 116),
		Rect(184, 219, 200, 116),
		Rect(268, 210, 200, 116),
	};
	
	canvas.draw(img, Point(0, 0), sourceRect.at(chronicle-1));

	if (chronicle == 8)
	{
		//skull
		auto locatorSkull = ImageLocator(ImagePath::builtin("CampSP1"), EImageBlitMode::OPAQUE);
		std::shared_ptr<IImage> imgSkull = ENGINE->renderHandler().loadImage(locatorSkull);
		canvas.draw(imgSkull, Point(162, 94), Rect(162, 94, 41, 22));
		canvas.draw(img, Point(162, 94), Rect(424, 304, 14, 4));
		canvas.draw(img, Point(162, 98), Rect(424, 308, 10, 4));
		canvas.draw(img, Point(158, 102), Rect(424, 312, 10, 4));
		canvas.draw(img, Point(154, 106), Rect(424, 316, 10, 4));
	}

	return image;
}

void AssetGenerator::createPaletteShiftedSprites()
{
	for(auto entity : LIBRARY->terrainTypeHandler->objects)
	{
		if(entity->paletteAnimation.empty())
			continue;

		std::vector<PaletteAnimation> paletteShifts;
		for(auto & animEntity : entity->paletteAnimation)
			paletteShifts.push_back({animEntity.start, animEntity.length});

		generatePaletteShiftedAnimation(entity->tilesFilename, paletteShifts);

	}
	for(auto entity : LIBRARY->riverTypeHandler->objects)
	{
		if(entity->paletteAnimation.empty())
			continue;

		std::vector<PaletteAnimation> paletteShifts;
		for(auto & animEntity : entity->paletteAnimation)
			paletteShifts.push_back({animEntity.start, animEntity.length});

		generatePaletteShiftedAnimation(entity->tilesFilename, paletteShifts);
	}
}

void AssetGenerator::generatePaletteShiftedAnimation(const AnimationPath & sprite, const std::vector<PaletteAnimation> & paletteAnimations)
{
	AnimationLayoutMap layout;

	auto animation = ENGINE->renderHandler().loadAnimation(sprite, EImageBlitMode::COLORKEY);

	int paletteTransformLength = 1;
	for (const auto & transform : paletteAnimations)
		paletteTransformLength = std::lcm(paletteTransformLength, transform.length);

	for(int tileIndex = 0; tileIndex < animation->size(); tileIndex++)
	{
		for(int paletteIndex = 0; paletteIndex < paletteTransformLength; paletteIndex++)
		{
			ImagePath spriteName = ImagePath::builtin(sprite.getName() + boost::str(boost::format("%02d") % tileIndex) + "_" + std::to_string(paletteIndex) + ".png");
			layout[paletteIndex].push_back(ImageLocator(spriteName, EImageBlitMode::SIMPLE));

			imageFiles[spriteName] = [this, sprite, paletteAnimations, tileIndex, paletteIndex](){
				return createPaletteShiftedImage(sprite, paletteAnimations, tileIndex, paletteIndex);
			};
		}
	}

	AnimationPath shiftedPath = AnimationPath::builtin("SPRITES/" + sprite.getName() + "_SHIFTED");
	animationFiles[shiftedPath] = layout;
}

AssetGenerator::CanvasPtr AssetGenerator::createPaletteShiftedImage(const AnimationPath & source, const std::vector<PaletteAnimation> & palette, int frameIndex, int paletteShiftCounter) const
{
	auto animation = ENGINE->renderHandler().loadAnimation(source, EImageBlitMode::COLORKEY);

	auto imgLoc = animation->getImageLocator(frameIndex, 0);
	auto img = ENGINE->renderHandler().loadImage(imgLoc);

	for(const auto & element : palette)
		img->shiftPalette(element.start, element.length, paletteShiftCounter % element.length);

	auto image = ENGINE->renderHandler().createImage(Point(32, 32), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();
	canvas.draw(img, Point((32 - img->dimensions().x) / 2, (32 - img->dimensions().y) / 2));

	return image;

}
