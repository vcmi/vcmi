/*
 * CMessage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CMessage.h"

#include "../CGameInfo.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/TextOperations.h"

#include "../windows/InfoWindows.h"
#include "../widgets/Buttons.h"
#include "../widgets/CComponent.h"
#include "../widgets/TextControls.h"
#include "../gui/CGuiHandler.h"
#include "../render/CAnimation.h"
#include "../render/IImage.h"
#include "../render/Canvas.h"
#include "../renderSDL/SDL_Extensions.h"

#include <SDL_surface.h>

const int BETWEEN_COMPS_ROWS = 10;
const int BEFORE_COMPONENTS = 30;
const int BETWEEN_COMPS = 30;
const int SIDE_MARGIN = 30;

template <typename T, typename U> std::pair<T,U> max(const std::pair<T,U> &x, const std::pair<T,U> &y)
{
	std::pair<T,U> ret;
	ret.first = std::max(x.first,y.first);
	ret.second = std::max(x.second,y.second);
	return ret;
}

//One image component + subtitles below it
class ComponentResolved : public CIntObject
{
public:
	std::shared_ptr<CComponent> comp;

	//blit component with image centered at this position
	void showAll(Canvas & to) override;

	//ComponentResolved();
	ComponentResolved(std::shared_ptr<CComponent> Comp);
	~ComponentResolved();
};
// Full set of components for blitting on dialog box
struct ComponentsToBlit
{
	std::vector< std::vector<std::shared_ptr<ComponentResolved>>> comps;
	int w, h;

	void blitCompsOnSur(bool blitOr, int inter, int &curh, SDL_Surface *ret);
	ComponentsToBlit(std::vector<std::shared_ptr<CComponent>> & SComps, int maxw, bool blitOr);
	~ComponentsToBlit();
};

namespace
{
	std::array<std::unique_ptr<CAnimation>, PlayerColor::PLAYER_LIMIT_I> dialogBorders;
	std::array<std::vector<std::shared_ptr<IImage>>, PlayerColor::PLAYER_LIMIT_I> piecesOfBox;

	std::shared_ptr<IImage> background;//todo: should be CFilledTexture
}

void CMessage::init()
{
	for(int i=0; i<PlayerColor::PLAYER_LIMIT_I; i++)
	{
		dialogBorders[i] = std::make_unique<CAnimation>("DIALGBOX");
		dialogBorders[i]->preload();

		for(int j=0; j < dialogBorders[i]->size(0); j++)
		{
			auto image = dialogBorders[i]->getImage(j, 0);
			//assume blue color initially
			if(i != 1)
				image->playerColored(PlayerColor(i));
			piecesOfBox[i].push_back(image);
		}
	}

	background = IImage::createFromFile("DIBOXBCK.BMP", EImageBlitMode::OPAQUE);
}

void CMessage::dispose()
{
	for(auto & item : dialogBorders)
		item.reset();
}

SDL_Surface * CMessage::drawDialogBox(int w, int h, PlayerColor playerColor)
{
	//prepare surface
	SDL_Surface * ret = CSDL_Ext::newSurface(w,h);
	for (int i=0; i<w; i+=background->width())//background
	{
		for (int j=0; j<h; j+=background->height())
		{
			background->draw(ret, i, j);
		}
	}

	drawBorder(playerColor, ret, w, h);
	return ret;
}

std::vector<std::string> CMessage::breakText( std::string text, size_t maxLineWidth, EFonts font )
{
	std::vector<std::string> ret;

	boost::algorithm::trim_right_if(text,boost::algorithm::is_any_of(std::string(" ")));

	// each iteration generates one output line
	while (text.length())
	{
		ui32 lineWidth = 0;    //in characters or given char metric
		ui32 wordBreak = -1;    //last position for line break (last space character)
		ui32 currPos = 0;       //current position in text
		bool opened = false;    //set to true when opening brace is found

		size_t symbolSize = 0; // width of character, in bytes
		size_t glyphWidth = 0; // width of printable glyph, pixels

		// loops till line is full or end of text reached
		while(currPos < text.length()  &&  text[currPos] != 0x0a  &&  lineWidth < maxLineWidth)
		{
			symbolSize = TextOperations::getUnicodeCharacterSize(text[currPos]);
			glyphWidth = graphics->fonts[font]->getGlyphWidth(text.data() + currPos);

			// candidate for line break
			if (ui8(text[currPos]) <= ui8(' '))
				wordBreak = currPos;

			/* We don't count braces in string length. */
			if (text[currPos] == '{')
				opened=true;
			else if (text[currPos]=='}')
				opened=false;
			else
				lineWidth += (ui32)glyphWidth;
			currPos += (ui32)symbolSize;
		}

		// long line, create line break
		if (currPos < text.length()  &&  (text[currPos] != 0x0a))
		{
			if (wordBreak != ui32(-1))
				currPos = wordBreak;
			else
				currPos -= (ui32)symbolSize;
		}

		//non-blank line
		if(currPos != 0)
		{
			ret.push_back(text.substr(0, currPos));

			if (opened)
				/* Close the brace for the current line. */
				ret.back() += '}';

			text.erase(0, currPos);
		}
		else if(text[currPos] == 0x0a)
		{
			ret.push_back(""); //add empty string, no extra actions needed
		}

		if (text.length() != 0 && text[0] == 0x0a)
		{
			/* Remove LF */
			text.erase(0, 1);
		}
		else
		{
			// trim only if line does not starts with LF
			// FIXME: necessary? All lines will be trimmed before returning anyway
			boost::algorithm::trim_left_if(text,boost::algorithm::is_any_of(std::string(" ")));
		}

		if (opened)
		{
			/* Add an opening brace for the next line. */
			if (text.length() != 0)
				text.insert(0, "{");
		}
	}

	/* Trim whitespaces of every line. */
	for (auto & elem : ret)
		boost::algorithm::trim(elem);

	return ret;
}

std::string CMessage::guessHeader(const std::string & msg)
{
	size_t begin = 0;
	std::string delimeters = "{}";
	size_t start = msg.find_first_of(delimeters[0], begin);
	size_t end = msg.find_first_of(delimeters[1], start);
	if(start > msg.size() || end > msg.size())
		return "";
	return msg.substr(begin, end);
}

int CMessage::guessHeight(const std::string & txt, int width, EFonts font)
{
	const auto f = graphics->fonts[font];
	auto lines = CMessage::breakText(txt, width, font);
	int lineHeight = static_cast<int>(f->getLineHeight());
	return lineHeight * (int)lines.size();
}

int CMessage::getEstimatedComponentHeight(int numComps)
{
	if (numComps > 8) //Bigger than 8 components - return invalid value
		return std::numeric_limits<int>::max();
	else if (numComps > 2)
		return 160; // 32px * 1 row + 20 to offset
	else if (numComps)
		return 118; // 118 px to offset
	return 0;
}

void CMessage::drawIWindow(CInfoWindow * ret, std::string text, PlayerColor player)
{
	bool blitOr = false;
	if(dynamic_cast<CSelWindow*>(ret)) //it's selection window, so we'll blit "or" between components
		blitOr = true;

	const int sizes[][2] = {{400, 125}, {500, 150}, {600, 200}, {480, 400}};

	assert(ret && ret->text);
	for(int i = 0;
		i < std::size(sizes)
			&& sizes[i][0] < GH.screenDimensions().x - 150
			&& sizes[i][1] < GH.screenDimensions().y - 150
			&& ret->text->slider;
		i++)
	{
		ret->text->resize(Point(sizes[i][0], sizes[i][1]));
	}

	if(ret->text->slider)
	{
		ret->text->slider->addUsedEvents(CIntObject::WHEEL | CIntObject::KEYBOARD);
	}
	else
	{
		ret->text->resize(ret->text->label->textSize + Point(10, 10));
	}

	std::pair<int,int> winSize(ret->text->pos.w, ret->text->pos.h); //start with text size

	ComponentsToBlit comps(ret->components,500, blitOr);
	if (ret->components.size())
		winSize.second += 10 + comps.h; //space to first component

	int bw = 0;
	if (ret->buttons.size())
	{
		int bh = 0;
		// Compute total width of buttons
		bw = 20*((int)ret->buttons.size()-1); // space between all buttons
		for(auto & elem : ret->buttons) //and add buttons width
		{
			bw+=elem->pos.w;
			vstd::amax(bh, elem->pos.h);
		}
		winSize.second += 20 + bh;//before button + button
	}

	// Clip window size
	vstd::amax(winSize.second, 50);
	vstd::amax(winSize.first, 80);
	vstd::amax(winSize.first, comps.w);
	vstd::amax(winSize.first, bw);

	vstd::amin(winSize.first, GH.screenDimensions().x - 150);

	ret->bitmap = drawDialogBox (winSize.first + 2*SIDE_MARGIN, winSize.second + 2*SIDE_MARGIN, player);
	ret->pos.h=ret->bitmap->h;
	ret->pos.w=ret->bitmap->w;
	ret->center();

	int curh = SIDE_MARGIN;
	int xOffset = (ret->pos.w - ret->text->pos.w)/2;

	if(!ret->buttons.size() && !ret->components.size()) //improvement for very small text only popups -> center text vertically
	{
		if(ret->bitmap->h > ret->text->pos.h + 2*SIDE_MARGIN)
			curh = (ret->bitmap->h - ret->text->pos.h)/2;
	}

	ret->text->moveBy(Point(xOffset, curh));

	curh += ret->text->pos.h;

	if (ret->components.size())
	{
		curh += BEFORE_COMPONENTS;
		comps.blitCompsOnSur (blitOr, BETWEEN_COMPS, curh, ret->bitmap);
	}
	if(ret->buttons.size())
	{
		// Position the buttons at the bottom of the window
		bw = (ret->bitmap->w/2) - (bw/2);
		curh = ret->bitmap->h - SIDE_MARGIN - ret->buttons[0]->pos.h;

		for(auto & elem : ret->buttons)
		{
			elem->moveBy(Point(bw, curh));
			bw += elem->pos.w + 20;
		}
	}
	for(size_t i=0; i<ret->components.size(); i++)
		ret->components[i]->moveBy(Point(ret->pos.x, ret->pos.y));
}

void CMessage::drawBorder(PlayerColor playerColor, SDL_Surface * ret, int w, int h, int x, int y)
{
	if(playerColor.isSpectator())
		playerColor = PlayerColor(1);
	auto & box = piecesOfBox.at(playerColor.getNum());

	// Note: this code assumes that the corner dimensions are all the same.

	// Horizontal borders
	int start_x = x + box[0]->width();
	const int stop_x = x + w - box[1]->width();
	const int bottom_y = y+h-box[7]->height()+1;
	while (start_x < stop_x) {
		int cur_w = stop_x - start_x;
		if (cur_w > box[6]->width())
			cur_w = box[6]->width();

		// Top border
		Rect srcR(0, 0, cur_w, box[6]->height());
		Rect dstR(start_x, y, 0, 0);
		box[6]->draw(ret, &dstR, &srcR);

		// Bottom border
		dstR.y = bottom_y;
		box[7]->draw(ret, &dstR, &srcR);

		start_x += cur_w;
	}

	// Vertical borders
	int start_y = y + box[0]->height();
	const int stop_y = y + h - box[2]->height()+1;
	const int right_x = x+w-box[5]->width();
	while (start_y < stop_y) {
		int cur_h = stop_y - start_y;
		if (cur_h > box[4]->height())
			cur_h = box[4]->height();

		// Left border
		Rect srcR(0, 0, box[4]->width(), cur_h);
		Rect dstR(x, start_y, 0, 0);
		box[4]->draw(ret, &dstR, &srcR);

		// Right border
		dstR.x = right_x;
		box[5]->draw(ret, &dstR, &srcR);

		start_y += cur_h;
	}

	//corners
	Rect dstR(x, y, box[0]->width(), box[0]->height());
	box[0]->draw(ret, &dstR, nullptr);

	dstR=Rect(x+w-box[1]->width(), y,   box[1]->width(), box[1]->height());
	box[1]->draw(ret, &dstR, nullptr);

	dstR=Rect(x, y+h-box[2]->height()+1, box[2]->width(), box[2]->height());
	box[2]->draw(ret, &dstR, nullptr);

	dstR=Rect(x+w-box[3]->width(), y+h-box[3]->height()+1, box[3]->width(), box[3]->height());
	box[3]->draw(ret, &dstR, nullptr);
}

ComponentResolved::ComponentResolved(std::shared_ptr<CComponent> Comp):
	comp(Comp)
{
	//Temporary assign ownership on comp
	if (parent)
		parent->removeChild(this);
	if (comp->parent)
	{
		comp->parent->addChild(this);
		comp->parent->removeChild(comp.get());
	}

	addChild(comp.get());
	defActions = 255 - DISPOSE;
	pos.x = pos.y = 0;

	pos.w = comp->pos.w;
	pos.h = comp->pos.h;
}

ComponentResolved::~ComponentResolved()
{
	if (parent)
	{
		removeChild(comp.get());
		parent->addChild(comp.get());
	}
}

void ComponentResolved::showAll(Canvas & to)
{
	CIntObject::showAll(to);
	comp->showAll(to);
}

ComponentsToBlit::~ComponentsToBlit() = default;

ComponentsToBlit::ComponentsToBlit(std::vector<std::shared_ptr<CComponent>> & SComps, int maxw, bool blitOr)
{
	int orWidth = static_cast<int>(graphics->fonts[FONT_MEDIUM]->getStringWidth(CGI->generaltexth->allTexts[4]));

	w = h = 0;
	if(SComps.empty())
		return;

	comps.resize(1);
	int curw = 0;
	int curr = 0; //current row

	for(auto & SComp : SComps)
	{
		auto cur = std::make_shared<ComponentResolved>(SComp);

		int toadd = (cur->pos.w + BETWEEN_COMPS + (blitOr ? orWidth : 0));
		if (curw + toadd > maxw)
		{
			curr++;
			vstd::amax(w,curw);
			curw = cur->pos.w;
			comps.resize(curr+1);
		}
		else
		{
			curw += toadd;
			vstd::amax(w,curw);
		}

		comps[curr].push_back(cur);
	}

	for(auto & elem : comps)
	{
		int maxHeight = 0;
		for(size_t j=0;j<elem.size();j++)
			vstd::amax(maxHeight, elem[j]->pos.h);

		h += maxHeight + BETWEEN_COMPS_ROWS;
	}
}

void ComponentsToBlit::blitCompsOnSur( bool blitOr, int inter, int &curh, SDL_Surface *ret )
{
	int orWidth = static_cast<int>(graphics->fonts[FONT_MEDIUM]->getStringWidth(CGI->generaltexth->allTexts[4]));

	for (auto & elem : comps)//for each row
	{
		int totalw=0, maxHeight=0;
		for(size_t j=0;j<elem.size();j++)//find max height & total width in this row
		{
			auto cur = elem[j];
			totalw += cur->pos.w;
			vstd::amax(maxHeight, cur->pos.h);
		}

		//add space between comps in this row
		if(blitOr)
			totalw += (inter*2+orWidth) * ((int)elem.size() - 1);
		else
			totalw += (inter) * ((int)elem.size() - 1);

		int middleh = curh + maxHeight/2;//axis for image aligment
		int curw = ret->w/2 - totalw/2;

		for(size_t j=0;j<elem.size();j++)
		{
			auto cur = elem[j];
			cur->moveTo(Point(curw, curh));

			//blit component
			Canvas canvas = Canvas::createFromSurface(ret);

			cur->showAll(canvas);
			curw += cur->pos.w;

			//if there is subsequent component blit "or"
			if(j<(elem.size()-1))
			{
				if(blitOr)
				{
					curw+=inter;

					graphics->fonts[FONT_MEDIUM]->renderTextLeft(ret, CGI->generaltexth->allTexts[4], Colors::WHITE,
							Point(curw,middleh-((int)graphics->fonts[FONT_MEDIUM]->getLineHeight()/2)));

					curw+=orWidth;
				}
				curw+=inter;
			}
		}
		curh += maxHeight + BETWEEN_COMPS_ROWS;
	}
}
