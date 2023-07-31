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

#include "../../lib/JsonNode.h"
#include "../../lib/TextOperations.h"
#include "../../lib/filesystem/Filesystem.h"

#include <SDL_ttf.h>

std::pair<std::unique_ptr<ui8[]>, ui64> CTrueTypeFont::loadData(const JsonNode & config)
{
	std::string filename = "Data/" + config["file"].String();
	return CResourceHandler::get()->load(ResourceID(filename, EResType::TTF_FONT))->readAll();
}

TTF_Font * CTrueTypeFont::loadFont(const JsonNode &config)
{
	int pointSize = static_cast<int>(config["size"].Float());

	if(!TTF_WasInit() && TTF_Init()==-1)
		throw std::runtime_error(std::string("Failed to initialize true type support: ") + TTF_GetError() + "\n");

	return TTF_OpenFontRW(SDL_RWFromConstMem(data.first.get(), (int)data.second), 1, pointSize);
}

int CTrueTypeFont::getFontStyle(const JsonNode &config)
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
	dropShadow(fontConfig["blend"].Bool()),
	blended(fontConfig["blend"].Bool())
{
	assert(font);

	TTF_SetFontStyle(font.get(), getFontStyle(fontConfig));

	std::string fallbackName = fontConfig["fallback"].String();

	if (!fallbackName.empty())
		fallbackFont = std::make_unique<CBitmapFont>(fallbackName);
}

CTrueTypeFont::~CTrueTypeFont() = default;

size_t CTrueTypeFont::getLineHeight() const
{
	if (fallbackFont)
		fallbackFont->getLineHeight();

	return TTF_FontHeight(font.get());
}

size_t CTrueTypeFont::getGlyphWidth(const char *data) const
{
	if (fallbackFont && fallbackFont->canRepresentCharacter(data))
		return fallbackFont->getGlyphWidth(data);

	return getStringWidth(std::string(data, TextOperations::getUnicodeCharacterSize(*data)));
	int advance;
	TTF_GlyphMetrics(font.get(), *data, nullptr, nullptr, nullptr, nullptr, &advance);
	return advance;
}

size_t CTrueTypeFont::getStringWidth(const std::string & data) const
{
	if (fallbackFont && fallbackFont->canRepresentString(data))
		return fallbackFont->getStringWidth(data);

	int width;
	TTF_SizeUTF8(font.get(), data.c_str(), &width, nullptr);
	return width;
}

void CTrueTypeFont::renderText(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const
{
	if (fallbackFont && fallbackFont->canRepresentString(data))
	{
		fallbackFont->renderText(surface, data, color, pos);
		return;
	}

	if (dropShadow && color.r != 0 && color.g != 0 && color.b != 0) // not black - add shadow
		renderText(surface, data, Colors::BLACK, pos + Point(1,1));

	if (!data.empty())
	{
		SDL_Surface * rendered;
		if (blended)
			rendered = TTF_RenderUTF8_Blended(font.get(), data.c_str(), CSDL_Ext::toSDL(color));
		else
			rendered = TTF_RenderUTF8_Solid(font.get(), data.c_str(), CSDL_Ext::toSDL(color));

		assert(rendered);

		CSDL_Ext::blitSurface(rendered, surface, pos);
		SDL_FreeSurface(rendered);
	}
}

