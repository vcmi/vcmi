/*
 * ControllerPrompt.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/ControllerPromptFamily.h"
#include "../../lib/Rect.h"

class Canvas;
class IImage;

namespace ControllerPrompt
{
enum class State { NORMAL, PRESSED, DISABLED };

struct Glyph
{
	std::string image;
	std::string label;
	Rect source;
	State state = State::NORMAL;
};

/// Uses actual binding names, including explicit remaps and Nintendo face labels.
std::optional<Glyph> resolve(Family family, const std::vector<std::string> & bindings, State state);

class Renderer
{
	std::map<std::string, std::shared_ptr<IImage>> images;
public:
	void draw(Canvas & canvas, const Glyph & glyph, const Point & position);
};
}
