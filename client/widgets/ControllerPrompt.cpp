/*
 * ControllerPrompt.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ControllerPrompt.h"
#include "../GameEngine.h"
#include "../render/EFont.h"
#include "../render/IFont.h"
#include "../render/IRenderHandler.h"
#include "render/Canvas.h"
#include "render/IImage.h"

namespace ControllerPrompt
{
std::optional<Glyph> resolve(Family family, const std::vector<std::string> & bindings, State state)
{
	if(family == Family::UNKNOWN || bindings.empty())
		return std::nullopt;

	std::string suffix = "normal";
	if(state == State::PRESSED)
		suffix = "pressed";
	else if(state == State::DISABLED)
		suffix = "disabled";
	Glyph result;
	result.state = state;
	if(bindings.size() == 1)
	{
		const auto & binding = bindings.front();
		const auto faceIndex = std::string("abxy").find(binding);
		if(binding.size() == 1 && faceIndex != std::string::npos)
		{
			result.source = Rect(0, 0, 24, 24);
			if(family == Family::PLAYSTATION)
				result.image = "controllerActionBar/playstation-" + binding + "-" + suffix + ".png";
			else
			{
				result.image = "controllerActionBar/generic-face-" + suffix + ".png";
				const std::string labels = family == Family::NINTENDO ? "BAYX" : "ABXY";
				result.label = labels[faceIndex];
			}
			return result;
		}
		if((binding == "leftshoulder" || binding == "rightshoulder") && state != State::DISABLED)
		{
			const std::string prefix = family == Family::PLAYSTATION ? "playstation" : "generic";
			result.image = "controllerActionBar/" + prefix + "-shoulders-" + suffix + ".png";
			result.source = Rect(binding == "leftshoulder" ? 0 : 36, 0, 36, 20);
			return result;
		}
	}

	// Unrecognized and compound remaps retain their exact configured names.
	result.label = boost::join(bindings, " / ");
	const auto font = ENGINE->renderHandler().loadFont(FONT_SMALL);
	result.source = Rect(0, 0, static_cast<int>(font->getStringWidth(result.label)) + 12,
		std::max(24, static_cast<int>(font->getLineHeight()) + 6));
	return result;
}

void Renderer::draw(Canvas & canvas, const Glyph & glyph, const Point & position)
{
	const ColorRGBA foreground = glyph.state == State::DISABLED ? ColorRGBA(115, 105, 92) : ColorRGBA(58, 40, 20);
	if(!glyph.image.empty())
	{
		auto & image = images[glyph.image];
		if(!image)
			image = ENGINE->renderHandler().loadImage(ImagePath::builtin(glyph.image), EImageBlitMode::COLORKEY);
		canvas.draw(image, position, glyph.source);
	}
	else
	{
		auto background = ColorRGBA(233, 217, 174);
		if(glyph.state == State::PRESSED)
			background = ColorRGBA(208, 186, 134);
		else if(glyph.state == State::DISABLED)
			background = ColorRGBA(225, 219, 204);
		const Rect area(position, glyph.source.dimensions());
		canvas.drawColor(area, background);
		canvas.drawBorder(area, ColorRGBA(91, 64, 37));
	}
	if(!glyph.label.empty())
		canvas.drawText(position + glyph.source.dimensions() / 2, FONT_SMALL, foreground, ETextAlignment::CENTER, glyph.label);
}
}
