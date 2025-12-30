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

#include "../render/Colors.h"
#include "../renderSDL/SDL_Extensions.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/texts/TextOperations.h"

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
	if(!TTF_WasInit() && TTF_Init()==-1)
		throw std::runtime_error(std::string("Failed to initialize true type support: ") + TTF_GetError() + "\n");

	return TTF_OpenFontRW(SDL_RWFromConstMem(data.first.get(), data.second), 1, getPointSize(config["size"]));
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
					 TTF_FontHeight(font.get()),
					 TTF_FontAscent(font.get()),
					 TTF_FontDescent(font.get()),
					 TTF_FontLineSkip(font.get())
	);
}

CTrueTypeFont::~CTrueTypeFont() = default;

size_t CTrueTypeFont::getFontAscentScaled() const
{
	return TTF_FontAscent(font.get());
}

size_t CTrueTypeFont::getLineHeightScaled() const
{
	return TTF_FontHeight(font.get());
}

size_t CTrueTypeFont::getGlyphWidthScaled(const char *text) const
{
	return getStringWidthScaled(std::string(text, TextOperations::getUnicodeCharacterSize(*text)));
}

bool CTrueTypeFont::canRepresentCharacter(const char * text) const
{
	uint32_t codepoint = TextOperations::getUnicodeCodepoint(text, TextOperations::getUnicodeCharacterSize(*text));
#if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
	return TTF_GlyphIsProvided32(font.get(), codepoint);
#elif SDL_TTF_VERSION_ATLEAST(2, 0, 12)
	if (codepoint <= 0xffff)
		return TTF_GlyphIsProvided(font.get(), codepoint);
	return true;
#else
	return true;
#endif
}

size_t CTrueTypeFont::getStringWidthScaled(const std::string & text) const
{
	int width;
	TTF_SizeUTF8(font.get(), text.c_str(), &width, nullptr);

	if (outline)
		width += getScalingFactor();
	if (dropShadow || outline)
		width += getScalingFactor();
		
	return width;
}

void CTrueTypeFont::renderText(SDL_Surface * surface, const std::string & text, const ColorRGBA & color, const Point & pos) const
{
	if (text.empty())
		return;

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
		rendered = TTF_RenderUTF8_Blended(font.get(), text.c_str(), CSDL_Ext::toSDL(color));
	else
		rendered = TTF_RenderUTF8_Solid(font.get(), text.c_str(), CSDL_Ext::toSDL(color));

	if (rendered)
	{
		CSDL_Ext::blitSurface(rendered, surface, pos);
		SDL_FreeSurface(rendered);
	}
	else
		logGlobal->error("Failed to render text '%s'. Reason: '%s'", text, TTF_GetError());

}

