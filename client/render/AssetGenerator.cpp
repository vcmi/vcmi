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
#include "../render/Colors.h"

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

	imageFiles[ImagePath::builtin("SpelTabNone.png")] = [this](){ return createSpellTabNone();};
	for (PlayerColor color(-1); color < PlayerColor::PLAYER_LIMIT; ++color)
	{
		std::string name = "ResBarElement" + (color == -1 ? "" : "-" + color.toString());
		imageFiles[ImagePath::builtin(name)] = [this, color](){ return createResBarElement(std::max(PlayerColor(0), color));};
	}

	for (PlayerColor color(-1); color < PlayerColor::PLAYER_LIMIT; ++color)
	{
		for(int amount : { 8, 9 })
		{
			auto addResWindow = [this, amount, color](std::string baseName, CreateResourceWindowType type){
				std::string name = baseName + "-R" + std::to_string(amount) + (color == -1 ? "" : "-" + color.toString());
				imageFiles[ImagePath::builtin(name)] = [this, color, amount, type](){ return createResourceWindow(type, amount, std::max(PlayerColor(0), color)); };
			};
			addResWindow("TPMRKABS", CreateResourceWindowType::ARTIFACTS_BUYING);
			addResWindow("TPMRKASS", CreateResourceWindowType::ARTIFACTS_SELLING);
			addResWindow("TPMRKRES", CreateResourceWindowType::MARKET_RESOURCES);
			addResWindow("TPMRKCRS", CreateResourceWindowType::FREELANCERS_GUILD);
			addResWindow("TPMRKPTS", CreateResourceWindowType::TRANSFER_RESOURCES);
		}
	}

	imageFiles[ImagePath::builtin("stackWindow/info-panel-0.png")] = [this](){ return createCreatureInfoPanel(2);};
	imageFiles[ImagePath::builtin("stackWindow/info-panel-1.png")] = [this](){ return createCreatureInfoPanel(3);};
	imageFiles[ImagePath::builtin("stackWindow/info-panel-2.png")] = [this](){ return createCreatureInfoPanel(4);};
	imageFiles[ImagePath::builtin("stackWindow/bonus-effects.png")] = [this](){ return createCreatureInfoPanelElement(BONUS_EFFECTS);};
	imageFiles[ImagePath::builtin("stackWindow/spell-effects.png")] = [this](){ return createCreatureInfoPanelElement(SPELL_EFFECTS);};
	imageFiles[ImagePath::builtin("stackWindow/button-panel.png")] = [this](){ return createCreatureInfoPanelElement(BUTTON_PANEL);};
	imageFiles[ImagePath::builtin("stackWindow/commander-bg.png")] = [this](){ return createCreatureInfoPanelElement(COMMANDER_BACKGROUND);};
	imageFiles[ImagePath::builtin("stackWindow/commander-abilities.png")] = [this](){ return createCreatureInfoPanelElement(COMMANDER_ABILITIES);};
	imageFiles[ImagePath::builtin("questDialog.png")] = [this](){ return createQuestWindow();};

	for (PlayerColor color(0); color < PlayerColor::PLAYER_LIMIT; ++color)
		imageFiles[ImagePath::builtin("DialogBoxBackground_" + color.toString())] = [this, color](){ return createPlayerColoredBackground(color);};

	for(int i = 1; i < 9; i++)
		imageFiles[ImagePath::builtin("CampaignHc" + std::to_string(i) + "Image.png")] = [this, i](){ return createChroniclesCampaignImages(i);};
	
	animationFiles[AnimationPath::builtin("SPRITES/adventureLayersButton")] = createAdventureMapButton(ImagePath::builtin("adventureLayers.png"), true);
	
	animationFiles[AnimationPath::builtin("SPRITES/GSPButtonClear")] = createGSPButtonClear();
	animationFiles[AnimationPath::builtin("SPRITES/GSPButton2Arrow")] = createGSPButton2Arrow();

	for (PlayerColor color(-1); color < PlayerColor::PLAYER_LIMIT; ++color)
	{
		std::string name = "TownPortalBackgroundBlue" + (color == -1 ? "" : "-" + color.toString());
		imageFiles[ImagePath::builtin(name)] = [this, color](){ return createGateListColored(std::max(PlayerColor(0), color), PlayerColor(1)); };
	}

	imageFiles[ImagePath::builtin("heroSlotsBlue.png")] = [this](){ return createHeroSlotsColored(PlayerColor(1));};

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

void AssetGenerator::addImageFile(const ImagePath & path, ImageGenerationFunctor & img)
{
	imageFiles[path] = img;
}

void AssetGenerator::addAnimationFile(const AnimationPath & path, AnimationLayoutMap & anim)
{
	animationFiles[path] = anim;
}

auto getColorFilters()
{
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
	return filters;
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
	static const std::array<ColorFilter, PlayerColor::PLAYER_LIMIT_I> filters = getColorFilters();

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
	Point bigBlockSize(200, 114);

	// SmallBlock section
	auto smallBlock = ENGINE->renderHandler().createImage(Point(248, 114), CanvasScalingPolicy::IGNORE);
	Canvas croppedSmallBlock = smallBlock->getCanvas();
	croppedSmallBlock.draw(img, Point(0, 0), bigBlockRegion);
	Point smallBlockSize(134, 114);

	// Tripple block section
	auto trippleBlock = ENGINE->renderHandler().createImage(Point(72, 116), CanvasScalingPolicy::IGNORE);
	Rect trippleBlockSection(512, 246, 72, 116);
	Canvas croppedTrippleBlock = trippleBlock->getCanvas();
	croppedTrippleBlock.draw(img, Point(0, 0), trippleBlockSection);
	Point trippleBlockSize(70, 114);


	// First campaigns line
	if (selection > 7)
	{
		// Rebuild 1. campaigns line from 2 to 3 fields
		canvas.drawScaled(bigBlock->getCanvas(), Point(40, 72), bigBlockSize);
		canvas.drawScaled(trippleBlock->getCanvas(), Point(240, 73), trippleBlockSize);
		canvas.drawScaled(bigBlock->getCanvas(), Point(310, 72), bigBlockSize);
		canvas.drawScaled(trippleBlock->getCanvas(), Point(510, 72), trippleBlockSize);
		canvas.drawScaled(bigBlock->getCanvas(), Point(580, 72), bigBlockSize);
		canvas.drawScaled(trippleBlock->getCanvas(), Point(780, 72), trippleBlockSize);
	} 
	else
	{
		// Empty 1 + 2. field
		canvas.drawScaled(bigBlock->getCanvas(), Point(90, 72), bigBlockSize);
		canvas.drawScaled(bigBlock->getCanvas(), Point(540, 72), bigBlockSize);
	}

	// Second campaigns line
	// 3. Field
	canvas.drawScaled(bigBlock->getCanvas(), Point(43, 245), bigBlockSize);

	if (selection == 4)
	{
		// Disabled 4. field
		canvas.drawScaled(trippleBlock->getCanvas(), Point(310, 245), trippleBlockSize);
		canvas.drawScaled(smallBlock->getCanvas(), Point(380, 245), smallBlockSize);
	}
	else
	{
		// Empty 4. field
		canvas.drawScaled(bigBlock->getCanvas(), Point(314, 244), bigBlockSize);
	}
	
	// 5. Field
	canvas.drawScaled(bigBlock->getCanvas(), Point(586, 246), bigBlockSize);

	// Third campaigns line
	// 6. Field
	if (selection >= 6)
	{
		canvas.drawScaled(bigBlock->getCanvas(), Point(32, 417), bigBlockSize);
	}
	else
	{
		canvas.drawScaled(trippleBlock->getCanvas(), Point(30, 417), trippleBlockSize);
		canvas.drawScaled(smallBlock->getCanvas(), Point(100, 417), smallBlockSize);
	}

	auto locatorSkull = ImageLocator(ImagePath::builtin("CAMPNOSC"), EImageBlitMode::OPAQUE);
	std::shared_ptr<IImage> imgSkull = ENGINE->renderHandler().loadImage(locatorSkull);

	if (selection >= 7)
	{
		// Only skull part
		canvas.drawScaled(bigBlock->getCanvas(), Point(404, 417), bigBlockSize);
		canvas.draw(imgSkull, Point(563, 512), Rect(178, 108, 43, 19));
	}
	else
	{
		// Original disabled field with skull and stone for 8. field
		Canvas canvasSkull = Canvas(Point(imgSkull->width(), imgSkull->height()), CanvasScalingPolicy::IGNORE);
		canvasSkull.draw(imgSkull, Point(0, 0), Rect(0, 0, imgSkull->width(), imgSkull->height()));
		canvas.drawScaled(canvasSkull, Point(385, 400), Point(238, 150));
	}


	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createResBarElement(const PlayerColor & player) const
{
	auto locator = ImageLocator(ImagePath::builtin("ARESBAR"), EImageBlitMode::COLORKEY);
	std::shared_ptr<IImage> img = ENGINE->renderHandler().loadImage(locator);
	img->playerColored(player);

	auto image = ENGINE->renderHandler().createImage(Point(84, 22), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();
	canvas.draw(img, Point(0, 0), Rect(2, 0, 84, 22));
	canvas.draw(img, Point(4, 0), Rect(29, 0, 22, 22));

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createSpellTabNone() const
{
	auto img1 = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("SPELTAB"), EImageBlitMode::COLORKEY)->getImage(0);
	auto img2 = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("SPELTAB"), EImageBlitMode::COLORKEY)->getImage(4);
	
	auto image = ENGINE->renderHandler().createImage(img1->dimensions(), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();
	canvas.draw(img1, Point(0, img1->height() / 2), Rect(0, img1->height() / 2, img1->width(), img1->height() / 2));
	canvas.draw(img2, Point(0, 0), Rect(0, 0, img2->width(), img2->height() / 2));

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

void meanImage(AssetGenerator::CanvasPtr dst, std::vector<Canvas> & images)
{
	auto image = dst->getCanvas();

	for(int x = 0; x < dst->width(); x++)
		for(int y = 0; y < dst->height(); y++)
		{
			int sumR = 0;
			int sumG = 0;
			int sumB = 0;
			int sumA = 0;
			for(auto & img : images)
			{
				auto color = img.getPixel(Point(x, y));
				sumR += color.r;
				sumG += color.g;
				sumB += color.b;
				sumA += color.a;
			}
			int ct = images.size();
			image.drawPoint(Point(x, y), ColorRGBA(sumR / ct, sumG / ct, sumB / ct, sumA / ct));
		}
}

AssetGenerator::CanvasPtr AssetGenerator::createAdventureMapButtonClear(const PlayerColor & player, bool small) const
{
	CanvasPtr dst = nullptr;
	if(small)
	{
		auto imageNames = { "iam002", "iam003", "iam004", "iam005", "iam006", "iam007", "iam008", "iam009", "iam010", "iam011" };
		std::vector<Canvas> images;

		for(auto & imageName : imageNames)
		{
			auto animation = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin(imageName), EImageBlitMode::COLORKEY);
			animation->playerColored(player);
			auto image = ENGINE->renderHandler().createImage(animation->getImage(2)->dimensions(), CanvasScalingPolicy::IGNORE);
			if(!dst)
				dst = ENGINE->renderHandler().createImage(animation->getImage(2)->dimensions(), CanvasScalingPolicy::IGNORE);
			Canvas canvas = image->getCanvas();	
			canvas.draw(animation->getImage(2), Point(0, 0));
			images.push_back(image->getCanvas());
		}

		meanImage(dst, images);
	}
	else
	{
		auto animation = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("iam001"), EImageBlitMode::COLORKEY);
		animation->playerColored(player);
		auto image = animation->getImage(2);

		dst = ENGINE->renderHandler().createImage(image->dimensions(), CanvasScalingPolicy::IGNORE);
		Canvas canvas = dst->getCanvas();	
		canvas.draw(image, Point(0, 0));

		auto tmp = ENGINE->renderHandler().createImage(Point(5, 22), CanvasScalingPolicy::IGNORE);
		std::vector<Canvas> meanImages;
		auto tmpLeft = ENGINE->renderHandler().createImage(Point(5, 22), CanvasScalingPolicy::IGNORE);
		tmpLeft->getCanvas().draw(image, Point(0, 0), Rect(18, 6, 5, 22));
		meanImages.push_back(tmpLeft->getCanvas());
		auto tmpRight = ENGINE->renderHandler().createImage(Point(5, 22), CanvasScalingPolicy::IGNORE);
		tmpRight->getCanvas().draw(image, Point(0, 0), Rect(42, 6, 5, 22));
		meanImages.push_back(tmpRight->getCanvas());
		meanImage(tmp, meanImages);

		for(int i = 0; i < 4; i++)
			canvas.draw(tmp, Point(23 + i * 5, 6));
	}

	return dst;
}

AssetGenerator::AnimationLayoutMap AssetGenerator::createAdventureMapButton(const ImagePath & overlay, bool small)
{
	std::shared_ptr<IImage> overlayImg = ENGINE->renderHandler().loadImage(ImageLocator(overlay, EImageBlitMode::OPAQUE));
	auto overlayCanvasImg = ENGINE->renderHandler().createImage(overlayImg->dimensions(), CanvasScalingPolicy::IGNORE);
	Canvas overlayCanvas = overlayCanvasImg->getCanvas();
	overlayCanvas.draw(overlayImg, Point(0, 0));

	AnimationLayoutMap layout;
	for (PlayerColor color(0); color < PlayerColor::PLAYER_LIMIT; ++color)
	{
		int offs = small ? 0 : 16;
		auto clearButtonImg = createAdventureMapButtonClear(color, small);
		for(int i = 0; i < 4; i++)
		{
			std::string baseName = overlay.getOriginalName() + "Btn" + (small ? "Small" : "Big") + std::to_string(i);
			ImagePath spriteName = ImagePath::builtin(baseName + ".png");
			ImagePath spriteNameColor = ImagePath::builtin(baseName + "-" + color.toString() + ".png");

			imageFiles[spriteNameColor] = [overlayCanvasImg, clearButtonImg, i, offs](){
				auto newImg = ENGINE->renderHandler().createImage(clearButtonImg->dimensions(), CanvasScalingPolicy::IGNORE);
				auto canvas = newImg->getCanvas();
				canvas.draw(clearButtonImg, Point(0, 0));
				switch (i)
				{
				case 0:
					canvas.draw(overlayCanvasImg, Point(offs, 0));
					return newImg;
				case 1:
					canvas.draw(clearButtonImg, Point(1, 1));
					canvas.draw(overlayCanvasImg, Point(offs + 1, 1));
					canvas.drawLine(Point(0, 0), Point(newImg->width() - 1, 0), ColorRGBA(0, 0, 0), ColorRGBA(0, 0, 0));
					canvas.drawLine(Point(0, 0), Point(0, newImg->height() - 1), ColorRGBA(0, 0, 0), ColorRGBA(0, 0, 0));
					canvas.drawColorBlended(Rect(0, 0, newImg->width(), 4), ColorRGBA(0, 0, 0, 160));
					canvas.drawColorBlended(Rect(0, 0, 4, newImg->height()), ColorRGBA(0, 0, 0, 160));
					return newImg;
				case 2:
					canvas.drawTransparent(overlayCanvasImg->getCanvas(), Point(offs, 0), 0.25);
					return newImg;
				default:
					canvas.draw(overlayCanvasImg, Point(offs, 0));
					canvas.drawLine(Point(0, 0), Point(newImg->width() - 1, 0), ColorRGBA(255, 255, 255), ColorRGBA(255, 255, 255));
					canvas.drawLine(Point(newImg->width() - 1, 0), Point(newImg->width() - 1, newImg->height() - 1), ColorRGBA(255, 255, 255), ColorRGBA(255, 255, 255));
					canvas.drawLine(Point(newImg->width() - 1, newImg->height() - 1), Point(0, newImg->height() - 1), ColorRGBA(255, 255, 255), ColorRGBA(255, 255, 255));
					canvas.drawLine(Point(0, newImg->height() - 1), Point(0, 0), ColorRGBA(255, 255, 255), ColorRGBA(255, 255, 255));
					return newImg;
				}
			};

			if(color == PlayerColor(0))
			{
				layout[0].push_back(ImageLocator(spriteName, EImageBlitMode::SIMPLE));
				imageFiles[spriteName] = imageFiles[spriteNameColor];
			}
		}
	}

	return layout;
}

AssetGenerator::AnimationLayoutMap AssetGenerator::createSliderBar(bool brown, bool horizontal, int length)
{
	AnimationLayoutMap layout;

	AnimationPath anim = brown ? AnimationPath::builtin("IGPCRDIV.DEF") : AnimationPath::builtin("SCNRBSL.DEF");

	auto genSlider = [horizontal, length](std::shared_ptr<IImage> src, Canvas & canvas){
		const int border = 6;
		const int inner  = 4;
		int pos = 0;

		while (pos < length)
		{
			int remain = length - pos;
			Rect c;

			// FIRST segment → leading border + inner
			if(pos == 0)
			{
				int w = std::min(remain, border + inner);
				c = horizontal ? Rect(0, 0, w, 16)
							: Rect(0, 0, 16, w);
			}
			// LAST segment → inner + trailing border
			else if(remain <= border + inner)
			{
				int w = std::min(remain, border + inner);
				c = horizontal ? Rect(16 - w, 0, w, 16)
							: Rect(0, 16 - w, 16, w);
			}
			// MIDDLE → pure inner (no borders)
			else
			{
				c = horizontal ? Rect(border, 0, inner, 16)
							: Rect(0, border, 16, inner);
			}

			canvas.draw(src,
						horizontal ? Point(pos,0) : Point(0,pos),
						c);

			// **Important**: advance by INNER for all but the last slice
			if(remain > border + inner) pos += inner;
			else break;
		}
	};

	for(int i = 0; i < 4; i++)
	{
		std::string baseName = "Slider-" + std::string(brown ? "brown" : "blue") + "-" + std::string(horizontal ? "horizontal" : "vertical") + "-" + std::to_string(length) + "-" + std::to_string(i);
		ImagePath spriteName = ImagePath::builtin(baseName + ".png");

		imageFiles[spriteName] = [anim, horizontal, brown, length, i, genSlider](){
			auto newImg = ENGINE->renderHandler().createImage(Point(horizontal ? length : 16, horizontal ? 16 : length), CanvasScalingPolicy::IGNORE);
			auto canvas = newImg->getCanvas();
			switch(i)
			{
			case 0:
				{
					auto src = ENGINE->renderHandler().loadAnimation(anim, EImageBlitMode::OPAQUE)->getImage(brown ? 4 : 0);
					genSlider(src, canvas);
					return newImg;
				}
			case 1:
				{
					auto tmpImg = ENGINE->renderHandler().createImage(Point(16, 16), CanvasScalingPolicy::IGNORE);
					auto tmpCanvas = tmpImg->getCanvas();
					auto srcImg = ENGINE->renderHandler().loadAnimation(anim, EImageBlitMode::OPAQUE)->getImage(brown ? 4 : 1);

					tmpCanvas.drawColor(Rect(Point(0, 0), tmpImg->dimensions()), Colors::BLACK);
					if(brown)
					{
						
						tmpCanvas.draw(srcImg, Point(1, 1));
						tmpCanvas.drawColorBlended(Rect(0, 0, tmpImg->width(), 3), ColorRGBA(0, 0, 0, 160));
						tmpCanvas.drawColorBlended(Rect(0, 0, 3, tmpImg->height()), ColorRGBA(0, 0, 0, 160));
					}
					else
						tmpCanvas.draw(srcImg, Point(0, 0));
					genSlider(tmpImg, canvas);
					return newImg;
				}
			case 2:
				{
					auto tmpImg = ENGINE->renderHandler().createImage(Point(16, 16), CanvasScalingPolicy::IGNORE);
					auto tmpCanvas = tmpImg->getCanvas();
					tmpCanvas.draw(ENGINE->renderHandler().loadAnimation(anim, EImageBlitMode::OPAQUE)->getImage(brown ? 4 : 2), Point(0, 0));
					// TODO: generate disabled brown slider (not used yet, but filling with dummy avoids warning)
					genSlider(tmpImg, canvas);
					return newImg;
				}
			default:
				{
					auto tmpImg = ENGINE->renderHandler().createImage(Point(16, 16), CanvasScalingPolicy::IGNORE);
					auto tmpCanvas = tmpImg->getCanvas();
					tmpCanvas.draw(ENGINE->renderHandler().loadAnimation(anim, EImageBlitMode::OPAQUE)->getImage(brown ? 4 : 3), Point(0, 0));
					if(brown)
						tmpCanvas.drawBorder(Rect(Point(0, 0), tmpImg->dimensions()), Colors::WHITE, 1);
					genSlider(tmpImg, canvas);
					return newImg;
				}
			}
		};

		layout[0].push_back(ImageLocator(spriteName, EImageBlitMode::SIMPLE));
	}

	return layout;
}

AssetGenerator::CanvasPtr AssetGenerator::createCreatureInfoPanel(int boxesAmount) const
{
	Point size(438, 187);

	auto image = ENGINE->renderHandler().createImage(size, CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();

	Rect r(4, 40, 102, 132);
	canvas.drawColor(r, Colors::BLACK);
	canvas.drawBorder(r, Colors::YELLOW);

    const ColorRGBA rectangleColor = ColorRGBA(0, 0, 0, 75);
    const ColorRGBA rectangleColorRed = ColorRGBA(32, 0, 0, 150);
    const ColorRGBA borderColor = ColorRGBA(128, 100, 75);

	r = Rect(60, 3, 315, 21);
	canvas.drawColorBlended(r, rectangleColor);
	canvas.drawBorder(r, borderColor);

	for(int i = 0; i < 8; i++)
	{
		Rect r(114, 30 + i * 19, 24, 20);
		canvas.drawColorBlended(r, rectangleColor);
		canvas.drawBorder(r, borderColor);
		r.x += 23;
		r.w = 173;
		canvas.drawColorBlended(r, rectangleColor);
		canvas.drawBorder(r, borderColor);
	}

	std::vector<Rect> redRects = {
		Rect(319, 30, 45, 45),
		Rect(373, 30, 45, 45)
	};
	std::vector<Rect> darkRects = {};

	if(boxesAmount == 3)
	{
		redRects.push_back(Rect(347, 109, 45, 45));
		darkRects.push_back(Rect(347, 156, 45, 19));
	}
	else if(boxesAmount == 4)
	{
		redRects.push_back(Rect(319, 109, 45, 45));
		redRects.push_back(Rect(373, 109, 45, 45));
		darkRects.push_back(Rect(319, 156, 45, 19));
		darkRects.push_back(Rect(373, 156, 45, 19));
	}
	
	for(auto & rect : darkRects)
	{
		canvas.drawColorBlended(rect, rectangleColor);
		canvas.drawBorder(rect, borderColor);
	}
	for(auto & rect : redRects)
	{
		canvas.drawColorBlended(rect, rectangleColorRed);
		canvas.drawBorder(rect, borderColor);
	}

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createResourceWindow(CreateResourceWindowType type, int count, PlayerColor color) const
{
	assert(count >= 8 && count <= 9);

	const std::map<CreateResourceWindowType, ImagePath> files = {
		{ ARTIFACTS_BUYING,   ImagePath::builtin("TPMRKABS") },
		{ ARTIFACTS_SELLING,  ImagePath::builtin("TPMRKASS") },
		{ MARKET_RESOURCES,   ImagePath::builtin("TPMRKRES") },
		{ FREELANCERS_GUILD,  ImagePath::builtin("TPMRKCRS") },
		{ TRANSFER_RESOURCES, ImagePath::builtin("TPMRKPTS") }
	};

	auto file = files.at(type);
	auto locator = ImageLocator(file, EImageBlitMode::COLORKEY);
	std::shared_ptr<IImage> baseImg = ENGINE->renderHandler().loadImage(locator);
	baseImg->playerColored(color);

	auto image = ENGINE->renderHandler().createImage(baseImg->dimensions(), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();
	canvas.draw(baseImg, Point(0, 0));

	auto drawBox = [&canvas, &baseImg](bool left, bool one){
		if(left)
		{
			canvas.draw(baseImg, Point(38, 339), Rect(121, 339, 71, 69));
			if(!one)
				canvas.draw(baseImg, Point(204, 339), Rect(121, 339, 71, 69));
		}
		else
		{
			canvas.draw(baseImg, Point(325, 339), Rect(408, 339, 71, 69));
			if(!one)
				canvas.draw(baseImg, Point(491, 339), Rect(408, 339, 71, 69));
		}
	};

	switch (type)
	{
	case ARTIFACTS_BUYING:
		drawBox(true, count == 8);
		break;
	case ARTIFACTS_SELLING:
		drawBox(false, count == 8);
		break;
	case MARKET_RESOURCES:
		drawBox(true, count == 8);
		drawBox(false, count == 8);
		break;
	case FREELANCERS_GUILD:
		drawBox(false, count == 8);
		break;
	case TRANSFER_RESOURCES:
		drawBox(true, count == 8);
		break;
	}

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createCreatureInfoPanelElement(CreatureInfoPanelElement element) const
{
	std::map<CreatureInfoPanelElement, Point> size {
		{BONUS_EFFECTS, Point(438, 59)},
		{SPELL_EFFECTS, Point(438, 42)},
		{BUTTON_PANEL, Point(438, 43)},
		{COMMANDER_BACKGROUND, Point(438, 177)},
		{COMMANDER_ABILITIES, Point(438, 59)}
	};

	auto image = ENGINE->renderHandler().createImage(size[element], CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();
	
    const ColorRGBA rectangleColor = ColorRGBA(0, 0, 0, 75);
    const ColorRGBA rectangleColorRed = ColorRGBA(32, 0, 0, 150);
    const ColorRGBA borderColor = ColorRGBA(128, 100, 75);

	switch (element)
	{
	case BONUS_EFFECTS:
		for(int i = 0; i < 2; i++)
		{
			Rect r(4 + i * 208, 0, 54, 54);
			canvas.drawColorBlended(r, rectangleColorRed);
			canvas.drawBorder(r, borderColor);
			r = Rect(61 + i * 208, 0, 144, 54);
			canvas.drawColorBlended(r, rectangleColor);
			canvas.drawBorder(r, borderColor);
		}
		break;
	case SPELL_EFFECTS:
		for(int i = 0; i < 8; i++)
		{
			Rect r(6 + i * 54, 2, 48, 36);
			canvas.drawColorBlended(r, rectangleColor);
			canvas.drawBorder(r, borderColor);
		}
		break;
	case BUTTON_PANEL:
		canvas.drawColorBlended(Rect(382, 5, 52, 36), Colors::BLACK);
		break;
	case COMMANDER_BACKGROUND:
		for(int x = 0; x < 3; x++)
		{
			for(int y = 0; y < 3; y++)
			{
				Rect r(269 + x * 52, 21 + y * 52, 44, 44);
				canvas.drawColorBlended(r, rectangleColorRed);
				canvas.drawBorder(r, borderColor);
			}
		}
		for(int x = 0; x < 3; x++)
		{
			for(int y = 0; y < 2; y++)
			{
				Rect r(10 + x * 80, 20 + y * 80, 70, 70);
				canvas.drawColor(r, Colors::BLACK);
			}
		}
		break;
	case COMMANDER_ABILITIES:
		for(int i = 0; i < 6; i++)
		{
			Rect r(37 + i * 63, 2, 54, 54);
			canvas.drawColorBlended(r, rectangleColorRed);
			canvas.drawBorder(r, borderColor);
		}
		for(int i = 0; i < 2; i++)
		{
			Rect r(10 + i * 401, 6, 22, 46);
			canvas.drawColor(r, Colors::BLACK);
		}
		break;
	}

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createQuestWindow() const
{
	auto locator = ImageLocator(ImagePath::builtin("DiBoxBck"), EImageBlitMode::OPAQUE);
	std::shared_ptr<IImage> img = ENGINE->renderHandler().loadImage(locator);

	Point size(612, 438);

	auto image = ENGINE->renderHandler().createImage(size, CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();

	for (int y = 0; y < size.y; y += img->height())
		for (int x = 0; x < size.x; x += img->width())
			canvas.draw(img, Point(x, y), Rect(0, 0, std::min(img->width(), size.x - x), std::min(img->height(), size.y - y)));

	Rect r(11, 11, 171, 171);
	canvas.drawColor(r, Colors::BLACK);
	canvas.drawBorder(r, Colors::YELLOW);

    const ColorRGBA rectangleColor = ColorRGBA(0, 0, 0, 75);
    const ColorRGBA borderColor = ColorRGBA(128, 100, 75);

	for(int i = 0; i < 6; i++)
	{
		Rect r(11, 194 + i * 32, 155, 33);
		canvas.drawColorBlended(r, rectangleColor);
		canvas.drawBorder(r, borderColor);
	}
	
	r = Rect(165, 194, 18, 193);
	canvas.drawColor(r, Colors::BLACK);
	canvas.drawBorder(r, borderColor);

	r = Rect(193, 11, 408, 376);
	canvas.drawColorBlended(r, rectangleColor);
	canvas.drawBorder(r, borderColor);

	return image;
}

AssetGenerator::AnimationLayoutMap AssetGenerator::createGSPButtonClear()
{
	auto baseImg = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("GSPBUTT"), EImageBlitMode::OPAQUE);
	auto overlayImg = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("GSPBUT2"), EImageBlitMode::OPAQUE);

	AnimationLayoutMap layout;
	for(int i = 0; i < 4; i++)
	{
		ImagePath spriteName = ImagePath::builtin("GSPButtonClear" + std::to_string(i) + ".png");

		imageFiles[spriteName] = [baseImg, overlayImg, i](){
			auto newImg = ENGINE->renderHandler().createImage(baseImg->getImage(i)->dimensions(), CanvasScalingPolicy::IGNORE);
			auto canvas = newImg->getCanvas();
			canvas.draw(baseImg->getImage(i), Point(0, 0));
			canvas.draw(overlayImg->getImage(i), Point(0, 0), Rect(0, 0, 20, 20));
			return newImg;
		};

		layout[0].push_back(ImageLocator(spriteName, EImageBlitMode::SIMPLE));
	}

	return layout;
}

AssetGenerator::AnimationLayoutMap AssetGenerator::createGSPButton2Arrow()
{
	auto baseImg = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("GSPBUT2"), EImageBlitMode::OPAQUE);
	auto overlayImg = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("GSPBUTT"), EImageBlitMode::OPAQUE);

	AnimationLayoutMap layout;
	for(int i = 0; i < 4; i++)
	{
		ImagePath spriteName = ImagePath::builtin("GSPButton2Arrow" + std::to_string(i) + ".png");

		imageFiles[spriteName] = [baseImg, overlayImg, i](){
			auto newImg = ENGINE->renderHandler().createImage(baseImg->getImage(i)->dimensions(), CanvasScalingPolicy::IGNORE);
			auto canvas = newImg->getCanvas();
			canvas.draw(baseImg->getImage(i), Point(0, 0));
			canvas.draw(overlayImg->getImage(i), Point(0, 0), Rect(0, 0, 20, 20));
			return newImg;
		};

		layout[0].push_back(ImageLocator(spriteName, EImageBlitMode::SIMPLE));
	}

	return layout;
}

AssetGenerator::CanvasPtr AssetGenerator::createGateListColored(PlayerColor color, PlayerColor backColor) const
{
	auto locator = ImageLocator(ImagePath::builtin("TpGate"), EImageBlitMode::COLORKEY);
	std::shared_ptr<IImage> img = ENGINE->renderHandler().loadImage(locator);
	img->playerColored(color);
	std::shared_ptr<IImage> imgColored = ENGINE->renderHandler().loadImage(locator);
	static const std::array<ColorFilter, PlayerColor::PLAYER_LIMIT_I> filters = getColorFilters();
	imgColored->adjustPalette(filters[backColor.getNum()], 0);

	auto image = ENGINE->renderHandler().createImage(img->dimensions(), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();

	canvas.draw(imgColored, Point(0, 0));

	std::vector<Rect> keepOriginalRects = {
		Rect(0, 0, 14, 393),
		Rect(293, 0, 13, 393),
		Rect(0, 393, 8, 76),
		Rect(299, 393, 6, 76),
		Rect(0, 0, 306, 16),
		Rect(0, 383, 306, 10),
		Rect(0, 441, 306, 2),
		Rect(0, 462, 306, 7),
		// Edges
		Rect(14, 15, 2, 5),
		Rect(16, 15, 3, 2),
		Rect(16, 17, 1, 1),
		Rect(14, 379, 3, 4),
		Rect(16, 381, 2, 2),
		Rect(16, 380, 1, 1),
		Rect(289, 16, 2, 2),
		Rect(291, 16, 2, 4),
		Rect(289, 381, 2, 2),
		Rect(291, 379, 2, 4)
	};
	for(auto & rect : keepOriginalRects)
		canvas.draw(img, Point(rect.x, rect.y), rect);

	std::vector<Rect> blackRect = {
		Rect(14, 401, 66, 32),
		Rect(227, 401, 66, 32)
	};
	for(auto & rect : blackRect)
		canvas.drawBorder(rect, Colors::BLACK);

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createHeroSlotsColored(PlayerColor backColor) const
{
	auto locator = ImageLocator(AnimationPath::builtin("OVSLOT"), 4, 0, EImageBlitMode::COLORKEY);
	std::shared_ptr<IImage> img = ENGINE->renderHandler().loadImage(locator);
	static const std::array<ColorFilter, PlayerColor::PLAYER_LIMIT_I> filters = getColorFilters();
	img->adjustPalette(filters[backColor.getNum()], 0);

	auto image = ENGINE->renderHandler().createImage(Point(327, 216), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();
	canvas.draw(img, Point(0, 0), Rect(3, 4, 253, 107));
	for(int i = 0; i<7; i++)
		canvas.draw(img, Point(1 + i * 36, 108), Rect(76, 57, 35, 17));

	// sec skill
	for(int x = 0; x<2; x++)
		for(int y = 0; y<4; y++)
		{
			canvas.draw(img, Point(255 + x * 36, y * (36 + 18)), Rect(3, 75, 36, 36));
			canvas.draw(img, Point(256 + x * 36, 37 + y * (36 + 18)), Rect(76, 57, 35, 17));
		}
	
	// artifacts
	for(int x = 0; x<7; x++)
		for(int y = 0; y<2; y++)
			canvas.draw(img, Point(x * 36, 130 + y * 36), Rect(3, 75, 36, 36));

	return image;
}
