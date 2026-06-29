/*
 * MDRenderer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MDRenderer.h"

#include "../WikiCommon.h"
#include "MDWidgets.h"

#include "../../../widgets/Images.h"
#include "../../../widgets/TextControls.h"
#include "../../../widgets/GraphicalPrimitiveCanvas.h"
#include "../../../render/Colors.h"
#include "../../../render/IFont.h"
#include "../../../render/IRenderHandler.h"
#include "../../../GameEngine.h"

// ============================================================================
// File-scope helpers used by MDRenderer methods
// ============================================================================

static ETextAlignment mdToTextAlign(MDAlign a)
{
	switch(a)
	{
		case MDAlign::CENTER: return ETextAlignment::CENTER;
		case MDAlign::RIGHT:  return ETextAlignment::TOPRIGHT;
		default:              return ETextAlignment::TOPLEFT;
	}
}

static int mdAlignedX(MDAlign a, int elemW, int margin, int contentW)
{
	switch(a)
	{
		case MDAlign::CENTER: return margin + (contentW - elemW) / 2;
		case MDAlign::RIGHT:  return margin + contentW - elemW;
		default:              return margin;
	}
}

// ============================================================================
// Constructor
// ============================================================================

MDRenderer::MDRenderer(
	std::vector<std::shared_ptr<CIntObject>> & widgets,
	int viewportW,
	int textW,
	bool blueStyle,
	const std::function<void(const std::string &)> & onWikiLink,
	std::map<std::string, int> * anchors)
	: widgets(widgets)
	, W(viewportW)
	, textW(textW)
	, blueStyle(blueStyle)
	, onWikiLink(onWikiLink)
	, anchors(anchors)
{}

// ============================================================================
// Alignment helpers
// ============================================================================

MDAlign MDRenderer::currentAlign(MDAlign defaultAlign) const
{
	return pendingAlign.value_or(defaultAlign);
}

// ============================================================================
// Widget-emission helpers
// ============================================================================

void MDRenderer::addParagraph(
	const std::string & text,
	int x, int width, int gap,
	EFonts font, ColorRGBA color, ETextAlignment align)
{
	if(text.empty())
		return;
	auto lbl = std::make_shared<CMultiLineLabel>(Rect(x, curY, width, 4000), font, align, color, text);
	lbl->pos.h = lbl->textSize.y;
	curY += lbl->textSize.y + gap;
	widgets.push_back(std::move(lbl));
}

void MDRenderer::renderListItemText(const std::string & text, int startX, int availW)
{
	if(MDParser::hasInlineMarkup(text))
	{
		layoutInlineLine(text, startX, availW);
	}
	else
	{
		auto lbl = std::make_shared<CMultiLineLabel>(
			Rect(startX, curY, availW, 4000), FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, text);
		lbl->pos.h = lbl->textSize.y;
		curY += lbl->textSize.y + MD_PARA_GAP;
		widgets.push_back(std::move(lbl));
	}
}

// ============================================================================
// Buffer flushers
// ============================================================================

void MDRenderer::flushPara()
{
	if(paraBuffer.empty())
		return;

	std::string joined;
	for(const auto & line : paraBuffer)
	{
		if(!joined.empty()) joined += ' ';
		joined += line;
	}
	paraBuffer.clear();

	// Warn and strip inline <br> tags
	if(joined.find("<br>") != std::string::npos || joined.find("<BR>") != std::string::npos)
	{
		logGlobal->warn("WikiMarkdown: <br> is not supported. Use a blank line to separate paragraphs.");
		joined = MDParser::replaceAll(joined, "<br>", " ");
		joined = MDParser::replaceAll(joined, "<BR>", " ");
	}

	const MDAlign        align = currentAlign(MDAlign::LEFT);
	const ETextAlignment ta    = mdToTextAlign(align);

	// Non-left alignment or no inline markup → simple CMultiLineLabel
	if(!MDParser::hasInlineMarkup(joined) || align != MDAlign::LEFT)
	{
		addParagraph(joined, MD_MARGIN, textW, MD_PARA_GAP, FONT_SMALL, Colors::WHITE, ta);
		return;
	}

	// Inline markup present: batch plain sub-lines, lay out styled ones word-by-word
	std::string plainAcc;
	auto flushPlainAcc = [&]()
	{
		if(plainAcc.empty())
			return;
		auto lbl = std::make_shared<CMultiLineLabel>(
			Rect(MD_MARGIN, curY, textW, 4000), FONT_SMALL, ta, Colors::WHITE, plainAcc);
		lbl->pos.h = lbl->textSize.y;
		curY += lbl->textSize.y + MD_PARA_GAP;
		widgets.push_back(std::move(lbl));
		plainAcc.clear();
	};

	std::istringstream ss(joined);
	std::string subLine;
	while(std::getline(ss, subLine))
	{
		const auto toks   = MDParser::tokenizeInline(subLine);
		const bool styled = std::any_of(toks.begin(), toks.end(),
			[](const MDInlineToken & t){ return t.type != MDInlineType::PLAIN; });

		if(!styled)
		{
			if(!plainAcc.empty()) plainAcc += '\n';
			plainAcc += subLine;
			continue;
		}
		flushPlainAcc();
		layoutInlineLine(subLine, MD_MARGIN, textW);
	}
	flushPlainAcc();
}

void MDRenderer::flushTable()
{
	if(tableBuffer.empty())
		return;

	// Filter out separator rows (|---|---|); keep content rows only
	std::vector<std::string> contentRows;
	contentRows.reserve(tableBuffer.size());
	for(const auto & row : tableBuffer)
		if(!MDParser::isTableSeparator(row))
			contentRows.push_back(row);
	tableBuffer.clear();

	if(contentRows.empty())
		return;

	// Parse all rows into cell strings
	std::vector<std::vector<std::string>> parsedRows;
	parsedRows.reserve(contentRows.size());
	for(const auto & row : contentRows)
		parsedRows.push_back(MDParser::splitTableRow(row));

	// Determine grid dimensions
	int numCols = 0;
	for(const auto & row : parsedRows)
		numCols = std::max(numCols, static_cast<int>(row.size()));
	if(numCols <= 0)
		return;

	const bool hasHeader   = parsedRows.size() >= 2;
	const int  dataStart   = hasHeader ? 1 : 0;
	const int  numDataRows = std::max(0, static_cast<int>(parsedRows.size()) - dataStart);
	const int  colW        = (textW - std::max(0, numCols - 1)) / numCols;

	// Measure every cell
	std::vector<std::vector<CellInfo>> grid(parsedRows.size(), std::vector<CellInfo>(numCols));
	for(int ri = 0; ri < static_cast<int>(parsedRows.size()); ++ri)
		for(int ci = 0; ci < numCols; ++ci)
		{
			const std::string & ct = (ci < static_cast<int>(parsedRows[ri].size()))
				? parsedRows[ri][ci] : std::string{};
			measureTableCell(grid[ri][ci], ct, colW);
		}

	// Compute row heights
	const int headerH = hasHeader ? 18 : 0;
	int dataRowH = 0;
	for(int ri = dataStart; ri < static_cast<int>(parsedRows.size()); ++ri)
		for(int ci = 0; ci < numCols; ++ci)
			dataRowH = std::max(dataRowH, grid[ri][ci].measuredH);
	dataRowH = std::max(dataRowH, 16);

	// Draw the table frame
	const std::vector<int> colWidths(numCols, colW);
	widgets.push_back(std::make_shared<WikiTableGrid>(
		MD_MARGIN, curY, textW, colWidths, headerH, dataRowH, numDataRows, blueStyle));

	// Header row: centred yellow labels
	if(hasHeader)
		for(int ci = 0; ci < numCols; ++ci)
		{
			const CellInfo & info = grid[0][ci];
			if(!info.isMedia)
				widgets.push_back(std::make_shared<CLabel>(
					MD_MARGIN + ci * (colW + 1) + colW / 2,
					curY + headerH / 2,
					FONT_SMALL, ETextAlignment::CENTER, Colors::YELLOW, info.text));
		}

	// Data rows
	for(int ri = dataStart; ri < static_cast<int>(parsedRows.size()); ++ri)
	{
		const int ry = curY + headerH + (ri - dataStart) * dataRowH;
		for(int ci = 0; ci < numCols; ++ci)
			renderTableCell(
				grid[ri][ci],
				MD_MARGIN + ci * (colW + 1) + TABLE_CELL_PAD,
				ry + TABLE_CELL_PAD,
				colW - TABLE_CELL_PAD * 2,
				dataRowH - TABLE_CELL_PAD * 2,
				dataRowH);
	}
	curY += headerH + numDataRows * dataRowH + MD_GAP;
}

// ============================================================================
// Inline layout
// ============================================================================

std::vector<MDRenderer::LayoutToken> MDRenderer::buildLayoutTokens(const std::string & ln) const
{
	const auto & font = ENGINE->renderHandler().loadFont(FONT_SMALL);
	std::vector<LayoutToken> result;

	for(const auto & tok : MDParser::tokenizeInline(ln))
	{
		if(tok.type == MDInlineType::LINK)
		{
			const int w = static_cast<int>(font->getStringWidth(tok.text));
			result.push_back({tok.text, tok.type, tok.target, w});
			continue;
		}
		// PLAIN: split at spaces so individual words wrap independently
		for(const auto & word : MDParser::splitPlainWithColorRewrap(tok.text))
		{
			const int w = static_cast<int>(font->getStringWidth(MDParser::stripColorTags(word)));
			result.push_back({word, MDInlineType::PLAIN, {}, w});
		}
	}
	return result;
}

void MDRenderer::layoutInlineLine(
	const std::string & ln, int startX, int availW, int trailGap)
{
	const auto & font  = ENGINE->renderHandler().loadFont(FONT_SMALL);
	const int    lineH = static_cast<int>(font->getLineHeight());
	const int    maxX  = startX + availW;
	int          curX  = startX;

	for(const auto & lt : buildLayoutTokens(ln))
	{
		if(lt.text.empty())
			continue;
		if(curX > startX && curX + lt.width > maxX)
		{
			curY += lineH;
			curX = startX;
		}

		// Strip trailing space from the rendered string; its width is still
		// included in lt.width for correct next-token positioning.
		const std::string rendered = (!lt.text.empty() && lt.text.back() == ' ')
			? lt.text.substr(0, lt.text.size() - 1) : lt.text;

		if(lt.type == MDInlineType::LINK)
		{
			if(onWikiLink && !rendered.empty())
				widgets.push_back(std::make_shared<WikiLinkLabel>(
					curX, curY, lt.width, lineH, rendered, lt.target, onWikiLink));
			else if(!rendered.empty())
			{
				auto lbl = std::make_shared<CLabel>(
					curX, curY, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, rendered);
				lbl->pos.w = lt.width;
				widgets.push_back(std::move(lbl));
			}
		}
		else if(!rendered.empty()) // PLAIN
		{
			auto lbl = std::make_shared<CLabel>(
				curX, curY, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, rendered);
			lbl->pos.w = lt.width;
			widgets.push_back(std::move(lbl));
		}
		curX += lt.width;
	}
	curY += lineH + trailGap;
}

// ============================================================================
// Table cell helpers
// ============================================================================

void MDRenderer::measureTableCell(CellInfo & info, const std::string & cellText, int colW) const
{
	ParsedMedia pm;
	if(MDParser::parseMediaLine(cellText, pm))
	{
		info.isMedia = true;
		info.media   = pm;
		const int cw = colW - TABLE_CELL_PAD * 2;

		if(pm.isVideo)
		{
			info.measuredH = 60;
		}
		else if(pm.isAnim)
		{
			const size_t fr = pm.hasFrame ? static_cast<size_t>(pm.frame) : 0;
			auto probe = ENGINE->renderHandler().loadImage(
				AnimationPath::builtin(pm.path), fr, 0, EImageBlitMode::COLORKEY);
			if(probe)
			{
				const Point sz = probe->dimensions();
				if(sz.x > 0)
				{
					const double sc = std::min(1.0, static_cast<double>(cw) / sz.x);
					info.measuredH = std::max(1, static_cast<int>(std::round(sz.y * sc)));
				}
			}
		}
		else
		{
			auto probe = ENGINE->renderHandler().loadImage(
				ImagePath::builtin(pm.path), EImageBlitMode::COLORKEY);
			if(probe)
			{
				const Point sz = probe->dimensions();
				if(sz.x > 0)
				{
					const double sc = std::min(1.0, static_cast<double>(cw) / sz.x);
					info.measuredH = std::max(1, static_cast<int>(std::round(sz.y * sc)));
				}
			}
		}
		info.measuredH += TABLE_CELL_PAD * 2;
	}
	else
	{
		info.text      = cellText;
		info.textAlign = MDParser::parseCellAlign(info.text);
		const std::string labelText = info.text.empty() ? " " : info.text;
		auto probe = std::make_shared<CMultiLineLabel>(
			Rect(0, 0, colW - TABLE_CELL_PAD * 2, 4000),
			FONT_SMALL, mdToTextAlign(info.textAlign), Colors::WHITE, labelText);
		info.measuredH = probe->textSize.y + TABLE_CELL_PAD * 2;
	}
	info.measuredH = std::max(info.measuredH, 16);
}

void MDRenderer::renderTableCell(
	const CellInfo & info, int mx, int my, int cw, int ch, int dataRowH)
{
	if(!info.isMedia)
	{
		widgets.push_back(std::make_shared<CMultiLineLabel>(
			Rect(mx, my, cw, dataRowH), FONT_SMALL,
			mdToTextAlign(info.textAlign), Colors::WHITE, info.text));
		return;
	}

	const auto & pm = info.media;
	if(pm.isVideo)
	{
		auto vid = std::make_shared<WikiVideoWidget>(Point(mx, my), VideoPath::builtin(pm.path), 1.0f);
		if(vid->pos.w > cw && vid->pos.w > 0)
		{
			const float sc = static_cast<float>(cw) / vid->pos.w;
			vid = std::make_shared<WikiVideoWidget>(Point(mx, my), VideoPath::builtin(pm.path), sc);
		}
		widgets.push_back(std::move(vid));
	}
	else if(pm.isAnim && !pm.hasFrame)
	{
		widgets.push_back(std::make_shared<WikiAnimLoopWidget>(
			AnimationPath::builtin(pm.path), mx, my));
	}
	else if(pm.isAnim)
	{
		auto probe = ENGINE->renderHandler().loadImage(
			AnimationPath::builtin(pm.path),
			static_cast<size_t>(pm.frame), 0, EImageBlitMode::COLORKEY);
		if(probe)
		{
			const Point sz = probe->dimensions();
			int rW, rH;
			if(sz.x <= cw)
			{
				rW = sz.x; rH = sz.y;
			}
			else
			{
				const double sc = static_cast<double>(cw) / sz.x;
				rW = cw;
				rH = std::max(1, static_cast<int>(std::round(sz.y * sc)));
			}
			const int vOff = std::max(0, (ch - rH) / 2);
			widgets.push_back(std::make_shared<CAnimImage>(
				AnimationPath::builtin(pm.path),
				static_cast<size_t>(pm.frame),
				Rect(mx, my + vOff, rW, rH), 0));
		}
	}
	else // static image
	{
		auto pic = std::make_shared<WikiScaledImage>(ImagePath::builtin(pm.path), mx, my, cw);
		if(pic->height() > 0)
		{
			const int vOff = std::max(0, (ch - pic->height()) / 2);
			pic->moveBy(Point(0, vOff));
			widgets.push_back(std::move(pic));
		}
	}
}

// ============================================================================
// Media rendering
// ============================================================================

void MDRenderer::appendMediaOverlays(
	const ParsedMedia & pm, int ex, int ey, int ew, int eh)
{
	if(!pm.alt.empty())
		widgets.push_back(std::make_shared<WikiAltPopup>(Rect(ex, ey, ew, eh), pm.alt));
	if(!pm.linkTarget.empty() && onWikiLink)
		widgets.push_back(std::make_shared<WikiLinkOverlay>(
			Rect(ex, ey, ew, eh), pm.linkTarget, onWikiLink));
}

void MDRenderer::renderMediaLine(const ParsedMedia & pm)
{
	if(pm.isVideo)
	{
		const MDAlign a = currentAlign(MDAlign::LEFT);
		auto vid = std::make_shared<WikiVideoWidget>(Point(0, curY), VideoPath::builtin(pm.path), 1.0f);
		if(vid->pos.w > textW && vid->pos.w > 0)
		{
			const float sc = static_cast<float>(textW) / vid->pos.w;
			vid = std::make_shared<WikiVideoWidget>(Point(0, curY), VideoPath::builtin(pm.path), sc);
		}
		const int vx     = mdAlignedX(a, vid->pos.w, MD_MARGIN, textW);
		const int vW     = vid->pos.w;
		const int vH     = vid->pos.h;
		vid->moveBy(Point(vx, 0));
		const int startY = curY;
		curY += vH + MD_GAP;
		widgets.push_back(std::move(vid));
		appendMediaOverlays(pm, vx, startY, vW, vH);
	}
	else if(pm.isAnim && !pm.hasFrame)
	{
		const MDAlign a  = currentAlign(MDAlign::LEFT);
		auto anim = std::make_shared<WikiAnimLoopWidget>(AnimationPath::builtin(pm.path), 0, curY);
		const int ax     = mdAlignedX(a, anim->pos.w, MD_MARGIN, textW);
		const int aW     = anim->pos.w;
		const int aH     = anim->pos.h;
		anim->moveBy(Point(ax, 0));
		const int startY = curY;
		curY += aH + MD_GAP;
		widgets.push_back(std::move(anim));
		appendMediaOverlays(pm, ax, startY, aW, aH);
	}
	else if(pm.isAnim) // single static frame
	{
		auto probe = ENGINE->renderHandler().loadImage(
			AnimationPath::builtin(pm.path),
			static_cast<size_t>(pm.frame), 0, EImageBlitMode::COLORKEY);
		if(probe)
		{
			const Point sz = probe->dimensions();
			if(sz.x > 0 && sz.y > 0)
			{
				int rW, rH;
				if(sz.x <= textW)
				{
					rW = sz.x; rH = sz.y;
				}
				else
				{
					const double sc = static_cast<double>(textW) / sz.x;
					rW = textW;
					rH = std::max(1, static_cast<int>(std::round(sz.y * sc)));
				}
				const MDAlign a  = currentAlign(MDAlign::LEFT);
				const int     rx = mdAlignedX(a, rW, MD_MARGIN, textW);
				const Rect    rct(rx, curY, rW, rH);
				widgets.push_back(std::make_shared<CAnimImage>(
					AnimationPath::builtin(pm.path),
					static_cast<size_t>(pm.frame), rct, 0));
				const int startY = curY;
				curY += rH + MD_GAP;
				appendMediaOverlays(pm, rx, startY, rW, rH);
			}
		}
	}
	else // static image
	{
		const MDAlign a = currentAlign(MDAlign::LEFT);
		auto pic = std::make_shared<WikiScaledImage>(ImagePath::builtin(pm.path), 0, curY, textW);
		if(pic->height() > 0)
		{
			const int px     = mdAlignedX(a, pic->pos.w, MD_MARGIN, textW);
			const int pW     = pic->pos.w;
			const int pH     = pic->height();
			pic->moveBy(Point(px, 0));
			const int startY = curY;
			curY += pH + MD_GAP;
			widgets.push_back(std::move(pic));
			appendMediaOverlays(pm, px, startY, pW, pH);
		}
	}
}

// ============================================================================
// Line handlers
// ============================================================================

bool MDRenderer::handleEmptyLine(const std::string & line)
{
	if(!line.empty()) return false;
	flushPara();
	curY += MD_GAP;
	return true;
}

bool MDRenderer::handleAlignTag(const std::string & line)
{
	const auto tag = MDParser::parseAlignTag(line);
	if(!tag.has_value()) return false;
	if(tag->close) pendingAlign.reset(); else pendingAlign = tag->align;
	return true;
}

bool MDRenderer::handleBreakTags(const std::string & line)
{
	if(line != "<p>" && line != "<P>" && line != "<br>" && line != "<BR>")
		return false;
	logGlobal->warn("WikiMarkdown: <p> and <br> tags are not supported. Use a blank line to separate paragraphs.");
	flushPara();
	curY += MD_GAP;
	return true;
}

bool MDRenderer::handleHeading(const std::string & line)
{
	std::string headText;
	const int level = MDParser::parseHeading(line, headText);
	if(level <= 0) return false;

	flushPara();

	// Trim whitespace (anchor tags were already stripped in run())
	const size_t first = headText.find_first_not_of(" \t");
	if(first != std::string::npos) headText = headText.substr(first);
	const size_t last  = headText.find_last_not_of(" \t");
	if(last  != std::string::npos) headText = headText.substr(0, last + 1);

	// Select font, top-padding, and approximate line height by level
	EFonts font; int topPad; int lineH;
	if(level == 1)      { font = FONT_BIG;    topPad = MD_H1_PAD_TOP; lineH = 22; }
	else if(level == 2) { font = FONT_MEDIUM; topPad = MD_H2_PAD_TOP; lineH = 16; }
	else                { font = FONT_SMALL;  topPad = MD_H3_PAD_TOP; lineH = 12; }

	curY += topPad;

	const MDAlign  a  = currentAlign(MDAlign::LEFT);
	int            lx;
	ETextAlignment ta;
	if(a == MDAlign::CENTER)     { lx = W / 2;         ta = ETextAlignment::CENTER;   }
	else if(a == MDAlign::RIGHT) { lx = W - MD_MARGIN; ta = ETextAlignment::TOPRIGHT; }
	else                         { lx = MD_MARGIN;      ta = ETextAlignment::TOPLEFT;  }

	widgets.push_back(std::make_shared<CLabel>(lx, curY, font, ta, Colors::YELLOW, headText));
	curY += lineH;

	// H1 and H2 get a thin underline
	if(level <= 2)
	{
		auto ul = std::make_shared<GraphicalPrimitiveCanvas>(Rect(MD_MARGIN, curY + 2, textW, 1));
		ul->addLine(Point(0, 0), Point(textW, 0), wikiBorderColor(blueStyle));
		widgets.push_back(std::move(ul));
		curY += 4;
	}
	curY += MD_PARA_GAP;
	return true;
}

bool MDRenderer::handleHRule(const std::string & line)
{
	if(!MDParser::isHRule(line)) return false;
	flushPara();
	curY += MD_GAP / 2;
	auto rule = std::make_shared<GraphicalPrimitiveCanvas>(Rect(MD_MARGIN, curY, textW, 2));
	rule->addBox(Point(0, 0), Point(textW, 2), wikiBorderColor(blueStyle));
	widgets.push_back(std::move(rule));
	curY += 2 + MD_GAP / 2;
	return true;
}

bool MDRenderer::handleMedia(const std::string & line)
{
	ParsedMedia pm;
	if(!MDParser::parseMediaLine(line, pm)) return false;
	flushPara();
	renderMediaLine(pm);
	return true;
}

bool MDRenderer::handleBulletItem(const std::string & line)
{
	std::string bText;
	if(!MDParser::parseBullet(line, bText)) return false;
	flushPara();
	auto dot = std::make_shared<GraphicalPrimitiveCanvas>(
		Rect(MD_BULLET_X, curY + 5, MD_BULLET_SZ, MD_BULLET_SZ));
	dot->addBox(Point(0, 0), Point(MD_BULLET_SZ, MD_BULLET_SZ), Colors::WHITE);
	widgets.push_back(std::move(dot));
	renderListItemText(bText, MD_BULLET_TXT, textW - (MD_BULLET_TXT - MD_MARGIN));
	return true;
}

bool MDRenderer::handleOrderedItem(const std::string & line)
{
	std::string oText;
	const int num = MDParser::parseOrdered(line, oText);
	if(num < 0) return false;
	flushPara();
	widgets.push_back(std::make_shared<CLabel>(
		MD_MARGIN + 2, curY,
		FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE,
		std::to_string(num) + "."));
	renderListItemText(oText, MD_ORDERED_TXT, textW - (MD_ORDERED_TXT - MD_MARGIN));
	return true;
}

bool MDRenderer::handleTableRow(const std::string & line)
{
	if(MDParser::isTableRow(line))
	{
		if(!paraBuffer.empty()) flushPara();
		tableBuffer.push_back(line);
		return true;
	}
	if(!tableBuffer.empty()) flushTable();
	return false;
}

bool MDRenderer::handleBlockquote(const std::string & line)
{
	if(line.empty() || line[0] != '>') return false;
	logGlobal->warn("WikiMarkdown: blockquotes (>) are not supported. Content is rendered as a plain paragraph.");
	const std::string plainText = (line.size() > 1 && line[1] == ' ')
		? line.substr(2) : line.substr(1);
	flushPara();
	paraBuffer.push_back(plainText);
	return true;
}

// ============================================================================
// run — main line-processing loop
// ============================================================================

void MDRenderer::run(const std::string & markdownText)
{
	// Split into individual lines, stripping \r from Windows line endings
	std::vector<std::string> lines;
	{
		std::istringstream ss(markdownText);
		std::string ln;
		while(std::getline(ss, ln))
		{
			if(!ln.empty() && ln.back() == '\r') ln.pop_back();
			lines.push_back(ln);
		}
	}

	bool inCodeFence = false;

	for(auto line : lines)
	{
		// Strip trailing whitespace
		while(!line.empty() && (line.back() == ' ' || line.back() == '\t'))
			line.pop_back();

		// Track fenced-code state; skip interior lines entirely
		if(line.size() >= 3 && line[0] == '`' && line[1] == '`' && line[2] == '`')
		{
			if(!inCodeFence)
			{
				flushPara();
				logGlobal->warn("WikiMarkdown: fenced code blocks (```) are not supported. Content inside the fence is skipped.");
			}
			inCodeFence = !inCodeFence;
			continue;
		}
		if(inCodeFence) continue;

		// Pre-process: extract and register all inline anchor tags, then strip
		// them from the line before dispatching to handlers.  This handles both
		// standalone anchors and anchors embedded in headings, e.g.:
		//   <a id="s2"/>## Section Two
		for(const auto & id : MDParser::parseAllAnchorTags(line))
			if(anchors) (*anchors)[id] = curY;
		line = MDParser::stripAnchorTags(line);

		// Dispatch to handlers; first match wins.
		// Lines not consumed by any handler are accumulated as paragraph text.
		const bool consumed =
			   handleEmptyLine(line)
			|| handleAlignTag(line)
			|| handleBreakTags(line)
			|| handleHeading(line)
			|| handleHRule(line)
			|| handleMedia(line)
			|| handleBulletItem(line)
			|| handleOrderedItem(line)
			|| handleTableRow(line)
			|| handleBlockquote(line);

		if(!consumed)
			paraBuffer.push_back(line);
	}

	// Flush any content that did not end with an explicit empty line
	flushPara();
	flushTable();
	curY += MD_MARGIN;
}
