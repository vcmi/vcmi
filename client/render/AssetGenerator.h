/*
 * AssetGenerator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ImageLocator.h"

VCMI_LIB_NAMESPACE_BEGIN
class PlayerColor;
VCMI_LIB_NAMESPACE_END

class ISharedImage;
class CanvasImage;

class AssetGenerator
{
public:
	using AnimationLayoutMap = std::map<size_t, std::vector<ImageLocator>>;
	using CanvasPtr = std::shared_ptr<CanvasImage>;
	using ImageGenerationFunctor = std::function<CanvasPtr()>;

	void initialize();

	std::shared_ptr<ISharedImage> generateImage(const ImagePath & image);

	std::map<ImagePath, std::shared_ptr<ISharedImage>> generateAllImages();
	std::map<AnimationPath, AnimationLayoutMap> generateAllAnimations();

	void addImageFile(const ImagePath & path, ImageGenerationFunctor & img);
	void addAnimationFile(const AnimationPath & path, AnimationLayoutMap & anim);

	AnimationLayoutMap createAdventureMapButton(const ImagePath & overlay, bool small);
	AnimationLayoutMap createSliderBar(bool brown, bool horizontal, int length);

private:
	struct PaletteAnimation
	{
		/// index of first color to cycle
		int32_t start;
		/// total numbers of colors to cycle
		int32_t length;
	};

	std::map<ImagePath, ImageGenerationFunctor> imageFiles;
	std::map<AnimationPath, AnimationLayoutMap> animationFiles;

	CanvasPtr createAdventureOptionsCleanBackground() const;
	CanvasPtr createBigSpellBook() const;
	CanvasPtr createPlayerColoredBackground(const PlayerColor & player) const;
	CanvasPtr createCombatUnitNumberWindow(float multR, float multG, float multB) const;
	CanvasPtr createCampaignBackground(int selection) const;
	CanvasPtr createSpellTabNone() const;
	CanvasPtr createResBarElement(const PlayerColor & player) const;
	CanvasPtr createChroniclesCampaignImages(int chronicle) const;
	CanvasPtr createPaletteShiftedImage(const AnimationPath & source, const std::vector<PaletteAnimation> & animation, int frameIndex, int paletteShiftCounter) const;
	CanvasPtr createAdventureMapButtonClear(const PlayerColor & player, bool small) const;
	CanvasPtr createCreatureInfoPanel(int boxesAmount) const;
	enum CreateResourceWindowType{ ARTIFACTS_BUYING, ARTIFACTS_SELLING, MARKET_RESOURCES, FREELANCERS_GUILD, TRANSFER_RESOURCES };
	CanvasPtr createResourceWindow(CreateResourceWindowType type, int count, PlayerColor color) const;
	enum CreatureInfoPanelElement{ BONUS_EFFECTS, SPELL_EFFECTS, BUTTON_PANEL, COMMANDER_BACKGROUND, COMMANDER_ABILITIES };
	CanvasPtr createCreatureInfoPanelElement(CreatureInfoPanelElement element) const;
	CanvasPtr createQuestWindow() const;
	AnimationLayoutMap createGSPButtonClear();
	AnimationLayoutMap createGSPButton2Arrow();
	CanvasPtr createGateListColored(PlayerColor color, PlayerColor backColor) const;
	CanvasPtr createHeroSlotsColored(PlayerColor backColor) const;

	void createPaletteShiftedSprites();
	void generatePaletteShiftedAnimation(const AnimationPath & source, const std::vector<PaletteAnimation> & animation);
};
