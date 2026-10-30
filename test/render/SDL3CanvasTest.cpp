/*
 * SDL3CanvasTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "../../client/StdInc.h"
#include "render/Canvas.h"
#include "render/GpuResources.h"
#include "render/ScreenHandler.h"
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

// The application entry point owns this fatal-error boundary.
[[noreturn]] void handleFatalError(const std::string & message, bool terminate)
{
	throw std::runtime_error(message);
}

class SDL3CanvasTest : public testing::Test
{
protected:
	SDL_Window * window = nullptr;
	SDL_Texture * target = nullptr;
	SDL_Texture * intermediate = nullptr;
	const Point size{16, 16};
	const Rect area{Point(0, 0), size};
	const ColorRGBA background{40, 80, 120, 255};

	void SetUp() override
	{
		ASSERT_TRUE(SDL_Init(SDL_INIT_VIDEO)) << SDL_GetError();
		window = SDL_CreateWindow("Canvas regression", 32, 32, SDL_WINDOW_HIDDEN);
		ASSERT_NE(window, nullptr) << SDL_GetError();
		auto * renderer = SDL_CreateRenderer(window, nullptr);
		ASSERT_NE(renderer, nullptr) << SDL_GetError();
		GpuResources::get().setRenderer(renderer);
		target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, size.x, size.y);
		intermediate = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, size.x, size.y);
		ASSERT_NE(target, nullptr) << SDL_GetError();
		ASSERT_NE(intermediate, nullptr) << SDL_GetError();
	}

	void TearDown() override
	{
		SDL_DestroyTexture(intermediate);
		SDL_DestroyTexture(target);
		GpuResources::get().destroyRenderer();
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	static void expectPixel(const Canvas & canvas, ColorRGBA expected)
	{
		const auto actual = canvas.getPixel(Point(4, 4));
		EXPECT_NEAR(actual.r, expected.r, 1);
		EXPECT_NEAR(actual.g, expected.g, 1);
		EXPECT_NEAR(actual.b, expected.b, 1);
		EXPECT_NEAR(actual.a, expected.a, 1);
	}
};

TEST_F(SDL3CanvasTest, ScaledSurfaceUsesSourceAlpha)
{
	auto output = Canvas::createFromRenderTarget(target, size, CanvasScalingPolicy::IGNORE);
	Canvas source(size, CanvasScalingPolicy::IGNORE);
	Canvas reference(size, CanvasScalingPolicy::IGNORE);
	for(int alpha : {0, 128, 255})
		for(int width : {8, 16})
		{
			SCOPED_TRACE(testing::Message() << "alpha=" << alpha << " width=" << width);
			source.drawColor(area, ColorRGBA(200, 100, 50, alpha));
			source.applyTransparency(true);
			output.drawColor(area, background);
			reference.drawColor(area, background);
			output.drawScaled(source, Point(0, 0), Point(width, width));
			reference.drawScaled(source, Point(0, 0), Point(width, width));
			expectPixel(output, reference.getPixel(Point(4, 4)));
		}
}

TEST_F(SDL3CanvasTest, TransparentTargetPreservesColorThroughFadeAndReuse)
{
	auto output = Canvas::createFromRenderTarget(target, size, CanvasScalingPolicy::IGNORE);
	auto layer = Canvas::createFromRenderTarget(intermediate, size, CanvasScalingPolicy::IGNORE);
	layer.drawColor(area, ColorRGBA(0, 0, 0, 0));
	layer.drawColorBlended(area, ColorRGBA(200, 100, 50, 128));
	for(double opacity : {1.0, 0.5, 0.0, 1.0})
	{
		SCOPED_TRACE(opacity);
		output.drawColor(area, background);
		output.drawTransparent(layer, Point(0, 0), opacity);
		const double alpha = 128.0 / 255.0 * opacity;
		expectPixel(output, ColorRGBA(std::lround(200 * alpha + 40 * (1 - alpha)),
			std::lround(100 * alpha + 80 * (1 - alpha)), std::lround(50 * alpha + 120 * (1 - alpha)), 255));
	}
}

TEST_F(SDL3CanvasTest, ReplacementWritesNormalizeAndEraseTransparentPixels)
{
	auto output = Canvas::createFromRenderTarget(target, size, CanvasScalingPolicy::IGNORE);
	auto layer = Canvas::createFromRenderTarget(intermediate, size, CanvasScalingPolicy::IGNORE);
	layer.drawColor(area, ColorRGBA(200, 100, 50, 128));
	output.drawColor(area, background);
	output.drawTransparent(layer, Point(0, 0), 1.0);
	expectPixel(output, ColorRGBA(120, 90, 85, 255));
	layer.drawColor(area, ColorRGBA(200, 100, 50, 0));
	output.drawColor(area, background);
	output.drawTransparent(layer, Point(0, 0), 1.0);
	expectPixel(output, background);
}

class ScreenHandlerTest : public testing::Test
{
protected:
	static void checkComposition()
	{
		ScreenHandler handler;
		SDL_HideWindow(handler.mainWindow);
		ASSERT_TRUE(handler.isGpuRenderingEnabled());
		const auto dimensions = handler.getLogicalResolution();
		const Rect area(Point(0, 0), dimensions);
		auto screen = Canvas::createFromRenderTarget(handler.screenTarget, dimensions, CanvasScalingPolicy::IGNORE);
		auto base = Canvas::createFromRenderTarget(handler.layerTextures[0], dimensions, CanvasScalingPolicy::IGNORE);
		auto overlay = Canvas::createFromRenderTarget(handler.layerTextures[1], dimensions, CanvasScalingPolicy::IGNORE);
		handler.layerActive[0] = true;
		handler.layerActive[1] = true;
		base.drawColor(area, ColorRGBA(40, 80, 120, 255));
		overlay.drawColor(area, ColorRGBA(0, 0, 0, 0));
		screen.drawColor(area, ColorRGBA(0, 0, 0, 0));

		auto * renderer = GpuResources::get().renderer();
		auto * target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, dimensions.x, dimensions.y);
		ASSERT_NE(target, nullptr) << SDL_GetError();
		const std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> targetOwner(target, &SDL_DestroyTexture);
		auto output = Canvas::createFromRenderTarget(target, dimensions, CanvasScalingPolicy::IGNORE);
		for(bool useScreen : {false, true})
		{
			SCOPED_TRACE(useScreen ? "screen" : "layer");
			auto & source = useScreen ? screen : overlay;
			source.drawColorBlended(area, ColorRGBA(200, 100, 50, 128));
			// Canvas copies change the source texture's blend mode; composition must restore it.
			output.draw(source, Point(0, 0));
			ASSERT_TRUE(SDL_SetRenderTarget(renderer, target));
			handler.composeFrame();
			const auto pixel = output.getPixel(Point(4, 4));
			EXPECT_NEAR(pixel.r, 120, 1);
			EXPECT_NEAR(pixel.g, 90, 1);
			EXPECT_NEAR(pixel.b, 85, 1);
			EXPECT_EQ(pixel.a, 255);
			source.drawColor(area, ColorRGBA(0, 0, 0, 0));
		}
	}
};

TEST_F(ScreenHandlerTest, ComposesTransparentLayersAndScreenAfterCanvasCopy)
{
	checkComposition();
}
