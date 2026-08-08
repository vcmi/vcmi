/*
 * CTrueTypeFont.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTrueTypeFont.h"

#include "CBitmapFont.h"

#include "render/Colors.h"
#include "SDL_Extensions.h"

#include "lib/CConfigHandler.h"
#include "lib/json/JsonNode.h"
#include "lib/filesystem/Filesystem.h"
#include "lib/texts/TextOperations.h"

#include <SDL3/SDL_iostream.h>

std::pair<std::unique_ptr<ui8[]>, ui64> CTrueTypeFont::loadData(const JsonNode & config)
{
	std::string filename = "Data/" + config["file"].String();
	return CResourceHandler::get()->load(ResourcePath(filename, EResType::TTF_FONT))->readAll();
}

int CTrueTypeFont::getPointSize(const JsonNode & config) const
{
	float fontScale = settings["video"]["fontScalingFactor"].Float();
	int scalingFactor = getScalingFactor();

	if (config.isNumber())
		return std::round(config.Integer() * scalingFactor * fontScale);
	else
		return std::round(config[scalingFactor-1].Integer() * fontScale);
}

TTF_Font * CTrueTypeFont::loadFont(const JsonNode &config)
{
	if(!TTF_WasInit() && !TTF_Init())
		throw std::runtime_error(std::string("Failed to initialize true type support: ") + SDL_GetError() + "\n");

	return TTF_OpenFontIO(SDL_IOFromConstMem(data.first.get(), data.second), true, getPointSize(config["size"]));
}

int CTrueTypeFont::getFontStyle(const JsonNode &config) const
{
	const JsonVector & names = config["style"].Vector();
	int ret = 0;
	for(const JsonNode & node : names)
	{
		if (node.String() == "bold")
			ret |= TTF_STYLE_BOLD;
		else if (node.String() == "italic")
			ret |= TTF_STYLE_ITALIC;
	}
	return ret;
}

CTrueTypeFont::CTrueTypeFont(const JsonNode & fontConfig):
	data(loadData(fontConfig)),
	font(loadFont(fontConfig), TTF_CloseFont),
	blended(true),
	outline(fontConfig["outline"].Bool()),
	dropShadow(!fontConfig["noShadow"].Bool())
{
	assert(font);

	TTF_SetFontStyle(font.get(), getFontStyle(fontConfig));
	TTF_SetFontHinting(font.get(),TTF_HINTING_MONO);

	logGlobal->debug("Loaded TTF font: '%s', point size %d, height %d, ascent %d, descent %d, line skip %d",
					 fontConfig["file"].String(),
					 getPointSize(fontConfig["size"]),
					 TTF_GetFontHeight(font.get()),
					 TTF_GetFontAscent(font.get()),
					 TTF_GetFontDescent(font.get()),
					 TTF_GetFontLineSkip(font.get())
	);
}

CTrueTypeFont::~CTrueTypeFont() = default;

size_t CTrueTypeFont::getFontAscentScaled() const
{
	return TTF_GetFontAscent(font.get());
}

size_t CTrueTypeFont::getLineHeightScaled() const
{
	return TTF_GetFontHeight(font.get());
}

size_t CTrueTypeFont::getGlyphWidthScaled(const char *text) const
{
	return getStringWidthScaled(std::string(text, TextOperations::getUnicodeCharacterSize(*text)));
}

bool CTrueTypeFont::canRepresentCharacter(const char * text) const
{
	uint32_t codepoint = TextOperations::getUnicodeCodepoint(text, TextOperations::getUnicodeCharacterSize(*text));
	return TTF_FontHasGlyph(font.get(), codepoint);
}

size_t CTrueTypeFont::getStringWidthScaled(const std::string & text) const
{
	int width = 0;
	TTF_GetStringSize(font.get(), text.c_str(), text.size(), &width, nullptr);

	if (hasColorGlyphs(text))
		return width;

	if (outline)
		width += getScalingFactor();
	if (dropShadow || outline)
		width += getScalingFactor();
		
	return width;
}

bool CTrueTypeFont::hasColorGlyphs(const std::string & text) const
{
	if (colorGlyphs.has_value())
		return *colorGlyphs;

	if (text.empty())
		return false;

	uint32_t codepoint = TextOperations::getUnicodeCodepoint(text.c_str(), TextOperations::getUnicodeCharacterSize(text[0]));

	TTF_ImageType imageType = TTF_IMAGE_INVALID;
	SDL_Surface * glyph = TTF_GetGlyphImage(font.get(), codepoint, &imageType);
	SDL_DestroySurface(glyph);

	colorGlyphs = imageType == TTF_IMAGE_COLOR;
	return *colorGlyphs;
}

void CTrueTypeFont::renderText(SDL_Surface * surface, const std::string & text, const ColorRGBA & color, const Point & pos) const
{
	if (text.empty())
		return;

	// an outline pass would redraw the colored glyph beside itself, not a black border
	if (hasColorGlyphs(text))
	{
		renderTextImpl(surface, text, color, pos);
		return;
	}

	if (outline)
		renderTextImpl(surface, text, Colors::BLACK, pos - Point(1,1) * getScalingFactor());

	if (dropShadow || outline)
		renderTextImpl(surface, text, Colors::BLACK, pos + Point(1,1) * getScalingFactor());

	renderTextImpl(surface, text, color, pos);
}

void CTrueTypeFont::renderTextImpl(SDL_Surface * surface, const std::string & text, const ColorRGBA & color, const Point & pos) const
{
	SDL_Surface * rendered;
	if (blended)
		rendered = TTF_RenderText_Blended(font.get(), text.c_str(), text.size(), CSDL_Ext::toSDL(color));
	else
		rendered = TTF_RenderText_Solid(font.get(), text.c_str(), text.size(), CSDL_Ext::toSDL(color));

	if (rendered)
	{
		CSDL_Ext::blitSurface(rendered, surface, pos);
		SDL_DestroySurface(rendered);
	}
	else
		logGlobal->error("Failed to render text '%s'. Reason: '%s'", text, SDL_GetError());

}

