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
	imageFiles[ImagePath::builtin("MuPopUpCustom.png")] = [this](){ return createMuPopUpCustom();};

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

	for(PlayerColor color(-1); color < PlayerColor::PLAYER_LIMIT; ++color)
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
	addRecruitmentBackground("TPRCRT4", Point(484, 394));
	addRecruitmentBackground("TPRCRT5", Point(594, 394));
	addRecruitmentBackground("TPRCRT6", Point(704, 394));
	addUniversityBackground("UNIVRS1", Point(466, 388), 1);
	addUniversityBackground("UNIVRS2", Point(466, 388), 2);
	addUniversityBackground("UNIVRS3", Point(466, 388), 3);
	addUniversityBackground("UNIVRS4", Point(466, 388), 4);
	addUniversityBackground("UNIVRS5", Point(570, 388), 5);
	addUniversityBackground("UNIVRS6", Point(674, 388), 6);
	addUniversityBackground("UNIVRS7", Point(778, 388), 7);
	addUniversityConfirmBackground("UNIVRSC1", Point(466, 388), 1);
	addUniversityConfirmBackground("UNIVRSC2", Point(466, 388), 2);
	addSpellResearchBackground("spellResearchDialog", Point(328, 474));
	for(int rowCount = 1; rowCount <= 7; ++rowCount)
	{
		const int tableRowsWithHeader = rowCount + 1;
		const int dialogHeight = 600 - (8 - tableRowsWithHeader) * 36;
		imageFiles[ImagePath::builtin("stackExperienceDialogRows" + std::to_string(rowCount))] = [this, tableRowsWithHeader, dialogHeight]()
		{
			return createStackExperienceDialogBackground(Point(800, dialogHeight), tableRowsWithHeader);
		};
	}

	addBackpackBackground("heroBackpackDialog", Point(426, 465));

	imageFiles[ImagePath::builtin("questDialog.png")] = [this](){ return createQuestWindow();};
	imageFiles[ImagePath::builtin("stackArtifactIndicatorSmall.png")] = [this](){ return createStackArtifactIndicator(Point(14, 14));};
	imageFiles[ImagePath::builtin("stackArtifactIndicatorLarge.png")] = [this](){ return createStackArtifactIndicator(Point(22, 22));};
	imageFiles[ImagePath::builtin("stackExperienceIconExperience.png")] = [this](){ return createStackExperienceIcon("experience"); };
	imageFiles[ImagePath::builtin("stackExperienceIconSpeed.png")] = [this](){ return createStackExperienceIcon("speed"); };
	imageFiles[ImagePath::builtin("stackExperienceIconAttack.png")] = [this](){ return createStackExperienceIcon("attack"); };
	imageFiles[ImagePath::builtin("stackExperienceIconDefense.png")] = [this](){ return createStackExperienceIcon("defense"); };
	imageFiles[ImagePath::builtin("stackExperienceIconMinDamage.png")] = [this](){ return createStackExperienceIcon("minDamage"); };
	imageFiles[ImagePath::builtin("stackExperienceIconMaxDamage.png")] = [this](){ return createStackExperienceIcon("maxDamage"); };
	imageFiles[ImagePath::builtin("stackExperienceIconHealth.png")] = [this](){ return createStackExperienceIcon("health"); };
	imageFiles[ImagePath::builtin("stackExperienceIconShots.png")] = [this](){ return createStackExperienceIcon("shots"); };
	imageFiles[ImagePath::builtin("stackExperienceIconCasts.png")] = [this](){ return createStackExperienceIcon("casts"); };
	imageFiles[ImagePath::builtin("stackExperienceIconInactiveOverlay.png")] = [this](){ return createStackExperienceInactiveOverlay(); };

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

	for (PlayerColor color(-1); color < PlayerColor::PLAYER_LIMIT; ++color)
	{
		std::string name = "AdventureOptionsBackground" + (color == -1 ? "" : "-" + color.toString());
		imageFiles[ImagePath::builtin(name)] = [this, color](){ return createAdventureOptionsBackground(std::max(PlayerColor(0), color));};
	}

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

void AssetGenerator::addBackpackBackground(const std::string & fileName, const Point & size)
{
	imageFiles[ImagePath::builtin(fileName)] = [this, size](){ return createBackpackDialogBackground(size);};
}

void AssetGenerator::addDialogBackground(const std::string & fileName, const Point & size)
{
	imageFiles[ImagePath::builtin(fileName)] = [this, size](){ return createDialogBackground(size);};
}

AssetGenerator::CanvasPtr AssetGenerator::createBackpackDialogBackground(const Point & size) const
{
	return createDialogBackground(size, true);
}

void AssetGenerator::addSpellResearchBackground(const std::string & fileName, const Point & size)
{
	imageFiles[ImagePath::builtin(fileName)] = [this, size](){ return createDialogBackground(size, true);};
}

void AssetGenerator::addRecruitmentBackground(const std::string & fileName, const Point & size)
{
	imageFiles[ImagePath::builtin(fileName)] = [this, size](){ return createRecruitmentDialogBackground(size);};
}

void AssetGenerator::addUniversityBackground(const std::string & fileName, const Point & size, int skillColumns)
{
	imageFiles[ImagePath::builtin(fileName)] = [this, size, skillColumns](){ return createUniversityDialogBackground(size, skillColumns);};
}

void AssetGenerator::addUniversityConfirmBackground(const std::string & fileName, const Point & size, int costElements)
{
	imageFiles[ImagePath::builtin(fileName)] = [this, size, costElements](){ return createUniversityConfirmDialogBackground(size, costElements);};
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

AssetGenerator::CanvasPtr AssetGenerator::createMuPopUpCustom() const
{
	auto locator = ImageLocator(ImagePath::builtin("MUPOPUP"), EImageBlitMode::OPAQUE);
	std::shared_ptr<IImage> img = ENGINE->renderHandler().loadImage(locator);

	auto image = ENGINE->renderHandler().createImage(Point(454, 449), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();

	canvas.draw(img, Point(0, 0), Rect(0, 0, 454, 449));
	canvas.draw(img, Point(0, 420), Rect(0, img->dimensions().y - 29, 454, 29));

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

AssetGenerator::CanvasPtr AssetGenerator::createAdventureOptionsBackground(PlayerColor color) const
{
	auto locator = ImageLocator(ImagePath::builtin("ADVOPTS"), EImageBlitMode::COLORKEY);
	std::shared_ptr<IImage> img = ENGINE->renderHandler().loadImage(locator);
	img->playerColored(color);

	std::shared_ptr<IImage> backgroundImg = ENGINE->renderHandler().loadImage(ImageLocator(ImagePath::builtin("DiBoxBck"), EImageBlitMode::OPAQUE));
	backgroundImg->adjustPalette(ColorFilter::genRangeShifter(0.f, 0.f, 0.f, 0.75f, 0.75f, 0.75f), 0); //darken

	auto image = ENGINE->renderHandler().createImage(img->dimensions(), CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();
	canvas.draw(img, Point(0, 0));

	Point backgroundSize(189, 48);
	for(int i = 0; i<5; i++)
		for(int x = 0; x < backgroundSize.x; x += backgroundImg->width())
			canvas.draw(backgroundImg, Point(78 + x, 25 + i * 58), Rect(0, 0, std::min(backgroundImg->width(), backgroundSize.x - x), std::min(backgroundImg->height(), backgroundSize.y)));

	return image;
}

AssetGenerator::AnimationLayoutMap AssetGenerator::createAdventureOptionsButton(const ImagePath & overlay)
{
    auto baseImg = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("ADVTURN"), EImageBlitMode::OPAQUE);

    AnimationLayoutMap layout;
    for(int i = 0; i < 4; i++)
    {
        ImagePath spriteName = ImagePath::builtin("adventureOptionsButton_" + overlay.getName() + "_" + std::to_string(i) + ".png");

        imageFiles[spriteName] = [baseImg, overlay, i](){
            auto sourceFrame = baseImg->getImage(i);
            auto newImg = ENGINE->renderHandler().createImage(sourceFrame->dimensions(), CanvasScalingPolicy::IGNORE);
            auto canvas = newImg->getCanvas();

            canvas.draw(sourceFrame, Point(0, 0));

            const int startX = 7, startY = 8;
            const int width = 36, height = 35;

            // 1. Draw gradient background into area
            {
                ColorRGBA colorLeft(201, 177, 103);
                ColorRGBA colorRight(171, 140, 74);

                for(int y = startY; y < startY + height; ++y)
                {
                    // 2147483647 is the Mersenne prime 2^31-1, used as a hash multiplier to scatter
                    // the row index across the modulus 13, producing an irregular scratch pattern
                    // instead of a visually obvious repeating stripe every 13 rows.
                    bool isScratchRow = ((static_cast<unsigned>(y) * 2147483647u) % 13u) > 8u;
                    // 1103515245 is the LCG multiplier from the glibc rand() implementation.
                    // Multiplying by it before taking mod 11 distributes the per-row brightness
                    // bias pseudo-randomly rather than cycling predictably every 11 rows.
                    int rowBaseBias = static_cast<int>((static_cast<unsigned>(y) * 1103515245u) % 11u) - 5;

                    for(int x = startX; x < startX + width; ++x)
                    {
                        float tx = static_cast<float>(x - startX) / width;

                        int r = colorLeft.r + (colorRight.r - colorLeft.r) * tx;
                        int g = colorLeft.g + (colorRight.g - colorLeft.g) * tx;
                        int b = colorLeft.b + (colorRight.b - colorLeft.b) * tx;

                        int scratchImpact = 0;
                        if(isScratchRow && (x + (y % 4)) % 15 < 10)
                            scratchImpact = (std::rand() % 100 > 70) ? -8 : -4;
                        int grain = (std::rand() % 8) - 4;

                        canvas.drawPoint(Point(x, y), ColorRGBA(
                            std::max(0, std::min(255, r + rowBaseBias + scratchImpact + grain)),
                            std::max(0, std::min(255, g + rowBaseBias + scratchImpact + grain)),
                            std::max(0, std::min(255, b + rowBaseBias + scratchImpact + grain))
                        ));
                    }
                }
            }

            // 2. Draw white copy of overlay shifted +1/+1 for border, then normal overlay on top
            auto overlayImg = ENGINE->renderHandler().loadImage(ImageLocator(overlay, EImageBlitMode::COLORKEY));

            // Create white version: draw overlay to temp canvas, then set all pixels to white keeping alpha
            auto whiteImg = ENGINE->renderHandler().createImage(overlayImg->dimensions(), CanvasScalingPolicy::IGNORE);
            Canvas whiteCanvas = whiteImg->getCanvas();
            whiteCanvas.draw(overlayImg, Point(0, 0));
            for(int y = 0; y < overlayImg->dimensions().y; ++y)
                for(int x = 0; x < overlayImg->dimensions().x; ++x)
                {
                    auto pixel = whiteCanvas.getPixel(Point(x, y));
                    whiteCanvas.drawPoint(Point(x, y), ColorRGBA(255, 255, 255, pixel.a));
                }
            canvas.draw(whiteImg, Point(startX + 1, startY + 1), Rect(0, 0, width, height));
            canvas.draw(overlayImg, Point(startX, startY), Rect(0, 0, width, height));

            // 2. Apply scanlines (frame 2) and darken (frames 1 & 3) over the entire image
            const float stateMultiplier = (i == 1 || i == 3) ? 0.7f : 1.0f;
            const bool applyScanlines = (i == 2);

            for(int y = 0; y < sourceFrame->dimensions().y; ++y)
            {
                for(int x = 0; x < sourceFrame->dimensions().x; ++x)
                {
                    auto pixel = canvas.getPixel(Point(x, y));
                    float r = pixel.r;
                    float g = pixel.g;
                    float b = pixel.b;

                    if(applyScanlines)
                    {
                        int scanPos = (x + y) % 4;
                        float scanMult;
                        if(scanPos == 0)                          scanMult = 0.0f;   // deep dark core
                        else if(scanPos == 1 || scanPos == 3)     scanMult = 0.65f;  // surrounding shadow
                        else                                      scanMult = 1.10f;  // slight highlight
                        r *= scanMult;
                        g *= scanMult;
                        b *= scanMult;
                    }

                    r *= stateMultiplier;
                    g *= stateMultiplier;
                    b *= stateMultiplier;

                    canvas.drawPoint(Point(x, y), ColorRGBA(
                        std::max(0, std::min(255, static_cast<int>(r))),
                        std::max(0, std::min(255, static_cast<int>(g))),
                        std::max(0, std::min(255, static_cast<int>(b))),
                        pixel.a
                    ));
                }
            }
            return newImg;
        };

        layout[0].push_back(ImageLocator(spriteName, EImageBlitMode::SIMPLE));
    }
    return layout;
}

AssetGenerator::CanvasPtr AssetGenerator::createStackArtifactIndicator(const Point & size) const
{
	auto image = ENGINE->renderHandler().createImage(size, CanvasScalingPolicy::IGNORE);
	auto canvas = image->getCanvas();
	canvas.applyTransparency(true);

	const Point center(size.x / 2, size.y / 2);
	const double radius = std::min(size.x, size.y) / 2.0;

	for(int y = 0; y < size.y; ++y)
	{
		for(int x = 0; x < size.x; ++x)
		{
			const double dx = x + 0.5 - center.x;
			const double dy = y + 0.5 - center.y;
			const double distanceToCenter = std::sqrt(dx * dx + dy * dy);

			if(distanceToCenter > radius)
				continue;

			const double edgeOpacity = std::clamp(radius - distanceToCenter, 0.0, 1.0);
			const double alphaValue = 191.0 * edgeOpacity;
			const auto alpha = static_cast<uint8_t>(alphaValue);
			canvas.drawPoint(Point(x, y), ColorRGBA(0, 0, 0, alpha));
		}
	}

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createStackExperienceIcon(const std::string & iconId) const
{
	auto image = ENGINE->renderHandler().createImage(Point(32, 32), CanvasScalingPolicy::IGNORE);
	auto canvas = image->getCanvas();
	canvas.drawColor(Rect(0, 0, 32, 32), Colors::TRANSPARENCY);

	auto drawScaledFrame = [&](const AnimationPath & anim, size_t frame)
	{
		auto icon = ENGINE->renderHandler().loadAnimation(anim, EImageBlitMode::COLORKEY)->getImage(frame);
		Canvas iconCanvas(Point(icon->width(), icon->height()), CanvasScalingPolicy::IGNORE);
		iconCanvas.draw(icon, Point(0, 0), Rect(0, 0, icon->width(), icon->height()));
		canvas.drawScaled(iconCanvas, Point(0, 0), Point(32, 32));
	};

	auto drawCenteredOverlay = [&](const std::shared_ptr<IImage> & overlay, int size = 28)
	{
		const int offset = (32 - size) / 2;
		auto scaledOverlay = ENGINE->renderHandler().createImage(Point(size, size), CanvasScalingPolicy::IGNORE);
		auto scaledOverlayCanvas = scaledOverlay->getCanvas();
		Canvas overlayCanvas(Point(overlay->width(), overlay->height()), CanvasScalingPolicy::IGNORE);
		overlayCanvas.draw(overlay, Point(0, 0), Rect(0, 0, overlay->width(), overlay->height()));
		scaledOverlayCanvas.drawScaled(overlayCanvas, Point(0, 0), Point(size, size));
		canvas.draw(std::static_pointer_cast<IImage>(scaledOverlay), Point(offset, offset), Rect(0, 0, size, size));
	};

	if(iconId == "experience")
	{
		auto base = ENGINE->renderHandler().loadImage(ImageLocator(ImagePath::builtin("LVLUPBKG.bmp"), EImageBlitMode::COLORKEY));
		auto cropped = ENGINE->renderHandler().createImage(Point(82, 82), CanvasScalingPolicy::IGNORE);
		cropped->getCanvas().draw(base, Point(0, 0), Rect(51, 56, 82, 82));
		Canvas croppedCanvas(Point(cropped->width(), cropped->height()), CanvasScalingPolicy::IGNORE);
		croppedCanvas.draw(std::static_pointer_cast<IImage>(cropped), Point(0, 0), Rect(0, 0, cropped->width(), cropped->height()));
		canvas.drawScaled(croppedCanvas, Point(0, 0), Point(32, 32));
	}
	else if(iconId == "speed")
	{
		drawScaledFrame(AnimationPath::builtin("SECSK82"), 0);
		auto overlayLocator = ImageLocator(AnimationPath::builtin("artifact"), 98, 0, EImageBlitMode::COLORKEY);
		overlayLocator.verticalFlip = true;
		auto overlay = ENGINE->renderHandler().loadImage(overlayLocator);
		drawCenteredOverlay(overlay, 28);
	}
	else if(iconId == "attack")
	{
		drawScaledFrame(AnimationPath::builtin("SECSK82"), 0);
		auto overlay = ENGINE->renderHandler().loadImage(ImageLocator(ImagePath::builtin("CampSwrd"), EImageBlitMode::COLORKEY));
		drawCenteredOverlay(overlay, 28);
	}
	else if(iconId == "defense")
	{
		drawScaledFrame(AnimationPath::builtin("SECSK82"), 0);
		auto source = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("PSKILL"), EImageBlitMode::COLORKEY)->getImage(1);
		auto cropped = ENGINE->renderHandler().createImage(Point(82, 82), CanvasScalingPolicy::IGNORE);
		cropped->getCanvas().draw(source, Point(0, 0), Rect(0, 4, 82, 82));
		drawCenteredOverlay(std::static_pointer_cast<IImage>(cropped));
	}
	else if(iconId == "minDamage")
	{
		drawScaledFrame(AnimationPath::builtin("SECSK82"), 0);
		auto overlay = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("SECSK82"), EImageBlitMode::COLORKEY)->getImage(69);
		drawCenteredOverlay(overlay);
	}
	else if(iconId == "maxDamage")
	{
		drawScaledFrame(AnimationPath::builtin("SECSK82"), 0);
		auto overlay = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("SECSK82"), EImageBlitMode::COLORKEY)->getImage(71);
		drawCenteredOverlay(overlay);
	}
	else if(iconId == "health")
	{
		drawScaledFrame(AnimationPath::builtin("SECSK82"), 0);
		auto overlay = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("SECSK82"), EImageBlitMode::COLORKEY)->getImage(84);
		drawCenteredOverlay(overlay);
	}
	else if(iconId == "shots")
	{
		drawScaledFrame(AnimationPath::builtin("SECSK82"), 0);
		auto overlay = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("ARTIFACT"), EImageBlitMode::COLORKEY)->getImage(91);
		drawCenteredOverlay(overlay, 28);
	}
	else if(iconId == "casts")
	{
		drawScaledFrame(AnimationPath::builtin("SECSK82"), 0);
		auto source = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("PSKILL"), EImageBlitMode::COLORKEY)->getImage(2);
		auto cropped = ENGINE->renderHandler().createImage(Point(82, 82), CanvasScalingPolicy::IGNORE);
		cropped->getCanvas().draw(source, Point(0, 0), Rect(0, 4, 82, 82));
		drawCenteredOverlay(std::static_pointer_cast<IImage>(cropped));
	}

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createStackExperienceInactiveOverlay() const
{
	auto image = ENGINE->renderHandler().createImage(Point(32, 32), CanvasScalingPolicy::IGNORE);
	auto canvas = image->getCanvas();
	canvas.drawColor(Rect(0, 0, 32, 32), Colors::TRANSPARENCY);
	canvas.drawColorBlended(Rect(0, 0, 32, 32), ColorRGBA(0, 0, 0, 110));

	return image;
}
AssetGenerator::CanvasPtr AssetGenerator::createDialogBackground(const Point & size, bool withStatusBar) const
{
	// Generic compositing helper:
	// 1) tile DiBoxBck over full target size
	// 2) add darkened status-bar strip at the bottom
	auto image = ENGINE->renderHandler().createImage(size, CanvasScalingPolicy::IGNORE);
	Canvas canvas = image->getCanvas();

	auto background = ENGINE->renderHandler().loadImage(ImageLocator(ImagePath::builtin("DiBoxBck"), EImageBlitMode::OPAQUE));
	// Fill whole area with DiBoxBck texture first.
	for (int y = 0; y < size.y; y += background->height())
	{
		for (int x = 0; x < size.x; x += background->width())
		{
			canvas.draw(background, Point(x, y), Rect(0, 0, std::min(background->width(), size.x - x), std::min(background->height(), size.y - y)));
		}
	}

	if(withStatusBar)
	{
		// Status bar overlay: darken bottom strip to match original dialog style
		const int statusBarOverlayHeight = 30;
		canvas.drawColorBlended(Rect(0, size.y - statusBarOverlayHeight, size.x, statusBarOverlayHeight), ColorRGBA(0, 0, 0, 88));
	}

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createStackExperienceDialogBackground(const Point & size, int rowCount) const
{
	auto image = createDialogBackground(size, true);
	Canvas canvas = image->getCanvas();

	const int creatureSideMargin = 25;
	const int tableSideMargin = 35;
	const int yOffset = 16;
	const int headerTop = 1 + yOffset;
	const int detailsTop = 68 + yOffset;
	const int tableTop = 218 + yOffset;
	const int rowNameWidth = 36;
	constexpr int stackExpBaseRowHeight = 35;
	const int tableHeight = stackExpBaseRowHeight * rowCount;
	const int rankColumns = 11;
	const int colWidth = (size.x - 2 * tableSideMargin - rowNameWidth) / rankColumns;
	const int rowHeight = tableHeight / rowCount;
	const int tableWidth = rowNameWidth + colWidth * rankColumns;
	const int tableRenderedHeight = rowHeight * rowCount;

	const ColorRGBA frameColor = ColorRGBA(128, 100, 75);
	const ColorRGBA fillColor = ColorRGBA(0, 0, 0, 70);
	const ColorRGBA tableContentFill = ColorRGBA(0, 0, 0, 35); // ~50% of top panel fill

	// Header frame for "<unit> - <rank>" label
	canvas.drawColorBlended(Rect(size.x / 2 - 190, headerTop + 34, 380, 24), fillColor);
	canvas.drawBorder(Rect(size.x / 2 - 190, headerTop + 34, 380, 24), frameColor);


	// Creature portrait frame
	const Rect creatureFrame(creatureSideMargin, detailsTop, 102, 132);
	canvas.drawBorder(creatureFrame, Colors::YELLOW);

	// Top info slots (short fields + multiline long fields)
	const int infoLeftX = creatureSideMargin + 116; // shifted 28px left relative to previous layout
	const int infoColumnGap = 14;
	const int infoLabelWidth = 221;
	const int infoValueWidth = 76;
	const int infoSectionGap = 10; // 2px visual gap between split blocks (+/-4px frame padding)
	const int infoWidth = infoLabelWidth + infoSectionGap + infoValueWidth;
	const int infoRightX = infoLeftX + infoWidth + infoColumnGap;
	const int infoTop = detailsTop + 4;
	const int infoFieldHeight = 22;
	const int infoFieldGap = 2;
	const int infoRowStep = infoFieldHeight + infoFieldGap;
	const int infoLongFieldHeight = infoFieldHeight * 2 + infoFieldGap + 8;

	auto drawSplitRow = [&](int baseX, int y, int height)
	{
		const Rect labelRect(baseX - 4, y, infoLabelWidth + 8, height);
		const Rect valueRect(baseX + infoLabelWidth + infoSectionGap - 4, y, infoValueWidth + 8, height);
		canvas.drawColorBlended(labelRect, fillColor);
		canvas.drawBorder(labelRect, frameColor);
		canvas.drawColorBlended(valueRect, fillColor);
		canvas.drawBorder(valueRect, frameColor);
	};

	for(int row = 0; row < 3; ++row)
	{
		const int y = infoTop + row * infoRowStep;
		drawSplitRow(infoLeftX, y, infoFieldHeight);
		drawSplitRow(infoRightX, y, infoFieldHeight);
	}

	const int longY = infoTop + 3 * infoRowStep;
	drawSplitRow(infoLeftX, longY, infoLongFieldHeight);
	drawSplitRow(infoRightX, longY, infoLongFieldHeight);

	// Bottom table background styling:
	// - header row + first column use the same fill style as top table panels
	// - content cells use lighter overlay (~50% intensity) for readability
	// Keep top-left header cell empty (no blended overlay).
	const Rect tableContentRect(tableSideMargin + rowNameWidth, tableTop + rowHeight, tableWidth - rowNameWidth, tableRenderedHeight - rowHeight);
	canvas.drawColorBlended(tableContentRect, tableContentFill);

	const Rect tableHeaderRect(tableSideMargin + rowNameWidth, tableTop, tableWidth - rowNameWidth, rowHeight);
	canvas.drawColorBlended(tableHeaderRect, fillColor);

	const Rect tableFirstColumnRect(tableSideMargin, tableTop + rowHeight, rowNameWidth, tableRenderedHeight - rowHeight);
	canvas.drawColorBlended(tableFirstColumnRect, fillColor);

	// Table border with missing top-left header cell (first column in header is intentionally empty).
	canvas.drawLine(Point(tableSideMargin + rowNameWidth, tableTop), Point(tableSideMargin + tableWidth - 1, tableTop), frameColor, frameColor);
	canvas.drawLine(Point(tableSideMargin, tableTop + rowHeight), Point(tableSideMargin, tableTop + tableRenderedHeight - 1), frameColor, frameColor);
	canvas.drawLine(Point(tableSideMargin + tableWidth - 1, tableTop), Point(tableSideMargin + tableWidth - 1, tableTop + tableRenderedHeight - 1), frameColor, frameColor);
	canvas.drawLine(Point(tableSideMargin, tableTop + tableRenderedHeight - 1), Point(tableSideMargin + tableWidth - 1, tableTop + tableRenderedHeight - 1), frameColor, frameColor);

	for(int line = 1; line < rowCount; ++line)
		canvas.drawLine(Point(tableSideMargin, tableTop + rowHeight * line), Point(tableSideMargin + tableWidth - 1, tableTop + rowHeight * line), frameColor, frameColor);

	// Icon slots in first column. Use 32x32 to match runtime row icons and avoid 1px overflow
	// against 35px table rows, which created visible artifacts on bottom row separators.
	for(int row = 1; row < rowCount; ++row)
	{
		const int cellTop = tableTop + row * rowHeight;
		const Rect iconRect(tableSideMargin + (rowNameWidth - 32) / 2, cellTop + (rowHeight - 32) / 2, 32, 32);
		canvas.drawColorBlended(iconRect, ColorRGBA(0, 0, 0, 64));
		canvas.drawBorder(iconRect, frameColor);
	}

	// Separator between icon column and rank columns.
	// Drawn as a single line to avoid visual "double border" artifacts at the join points.
	canvas.drawLine(Point(tableSideMargin + rowNameWidth, tableTop), Point(tableSideMargin + rowNameWidth, tableTop + tableRenderedHeight - 1), frameColor, frameColor);
	for(int column = 1; column < rankColumns; ++column)
	{
		const int x = tableSideMargin + rowNameWidth + column * colWidth;
		canvas.drawLine(Point(x, tableTop), Point(x, tableTop + tableRenderedHeight - 1), frameColor, frameColor);
	}

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createRecruitmentDialogBackground(const Point & size) const
{
	auto image = createDialogBackground(size, true);
	Canvas canvas = image->getCanvas();

	// Additional overlays used by original TPRCRT (semi-transparent plates and central black input area).
	const ColorRGBA rectangleColor = ColorRGBA(0, 0, 0, 75);
	const ColorRGBA borderColor = ColorRGBA(128, 100, 75);
	const Point originalSize(484, 362);
	const int offsetX = (image->width() - originalSize.x) / 2;

	auto drawPlate = [&canvas, rectangleColor, borderColor](const Rect & rect, bool blackFill = false)
	{
		if(blackFill)
			canvas.drawColor(rect, Colors::BLACK);
		else
			canvas.drawColorBlended(rect, rectangleColor);
		canvas.drawBorder(rect, borderColor);
	};

	auto centered = [offsetX](const Rect & rect)
	{
		return Rect(rect.x + offsetX, rect.y, rect.w, rect.h);
	};

	// Top row side plates (left / right) - 97x19
	drawPlate(centered(Rect(64, 223, 97, 19)));
	drawPlate(centered(Rect(323, 223, 97, 19)));

	// Bottom row side plates (left / right) - 97x19
	drawPlate(centered(Rect(64, 278, 97, 19)));
	drawPlate(centered(Rect(323, 278, 97, 19)));

	// Small middle plates above central bar (left / right) - 65x19
	drawPlate(centered(Rect(172, 244, 65, 19)));
	drawPlate(centered(Rect(247, 244, 65, 19)));

	// Central black input bar - 142x20
	drawPlate(centered(Rect(171, 278, 142, 20)), true);

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createUniversityDialogBackground(const Point & size, int skillColumns) const
{
	auto image = createDialogBackground(size, true);
	Canvas canvas = image->getCanvas();

	const ColorRGBA rectangleColor = ColorRGBA(0, 0, 0, 75);
	const ColorRGBA borderColor = ColorRGBA(128, 100, 75);
	const ColorRGBA slotColor = ColorRGBA(0, 0, 0, 95);

	auto drawPlate = [&canvas, rectangleColor, borderColor](const Rect & rect)
	{
		canvas.drawColorBlended(rect, rectangleColor);
		canvas.drawBorder(rect, borderColor);
	};

	auto drawSlot = [&canvas, slotColor, borderColor](const Rect & rect)
	{
		canvas.drawColorBlended(rect, slotColor);
		canvas.drawBorder(rect, borderColor);
	};

	// Main text block
	const int largeBlockY = 127;
	const int largeBlockHeight = 74;
	const int largeBlockWidth = 414;
	const int largeBlockX = (size.x - largeBlockWidth) / 2;
	drawPlate(Rect(largeBlockX, largeBlockY, largeBlockWidth, largeBlockHeight));

	// Skill rows. Keep 2px gap between columns and center the whole strip set.
	const int smallBlockWidth = 102;
	const int smallBlockHeight = 20;
	const int smallBlockGap = 2;
	const int firstRowY = largeBlockY + largeBlockHeight + 10; // 10px below large block
	const int secondRowY = firstRowY + smallBlockHeight + 50; // 50px below first row (edge to edge)

	std::vector<int> stripX(skillColumns);
	const int stripSetWidth = skillColumns * smallBlockWidth + (skillColumns - 1) * smallBlockGap;
	const int stripStartX = (size.x - stripSetWidth) / 2;
	for(int i = 0; i < skillColumns; ++i)
	{
		stripX[i] = stripStartX + i * (smallBlockWidth + smallBlockGap);

		drawPlate(Rect(stripX[i], firstRowY, smallBlockWidth, smallBlockHeight));
		drawPlate(Rect(stripX[i], secondRowY, smallBlockWidth, smallBlockHeight));
	}

	// Icon boxes centered in each strip column
	const int iconSize = 44;
	const int iconY = firstRowY + smallBlockHeight + 3; // 3px below first row and 3px above second row
	for(int i = 0; i < skillColumns; ++i)
	{
		const int iconX = stripX[i] + (smallBlockWidth - iconSize) / 2;
		drawSlot(Rect(iconX, iconY, iconSize, iconSize));
	}

	return image;
}

AssetGenerator::CanvasPtr AssetGenerator::createUniversityConfirmDialogBackground(const Point & size, int costElements) const
{
	auto image = createDialogBackground(size, true);
	Canvas canvas = image->getCanvas();

	const ColorRGBA rectangleColor = ColorRGBA(0, 0, 0, 75);
	const ColorRGBA borderColor = ColorRGBA(128, 100, 75);

	auto drawPlate = [&canvas, rectangleColor, borderColor](const Rect & rect)
	{
		canvas.drawColorBlended(rect, rectangleColor);
		canvas.drawBorder(rect, borderColor);
	};

	// Text block: same geometry as UNIVRS4
	drawPlate(Rect((size.x - 414) / 2, 127, 414, 74));

	// Centered header/action plates
	drawPlate(Rect((size.x - 105) / 2, 26, 105, 21));
	drawPlate(Rect((size.x - 105) / 2, 95, 105, 21));
	const int costPlateWidth = 71;
	const int costPlateHeight = 19;
	const int costPlateY = 258;
	const int costPlateGap = 10;

	if(costElements <= 1)
	{
		drawPlate(Rect((size.x - costPlateWidth) / 2, costPlateY, costPlateWidth, costPlateHeight));
	}
	else
	{
		const int totalWidth = 2 * costPlateWidth + costPlateGap;
		const int firstX = (size.x - totalWidth) / 2;
		drawPlate(Rect(firstX, costPlateY, costPlateWidth, costPlateHeight));
		drawPlate(Rect(firstX + costPlateWidth + costPlateGap, costPlateY, costPlateWidth, costPlateHeight));
	}

	return image;
}
