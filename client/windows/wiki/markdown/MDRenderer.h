/*
 * MDRenderer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "MDParser.h"

#include "../../../gui/CIntObject.h"
#include "../../../gui/TextAlignment.h"
#include "../../../render/EFont.h"
#include "../../../../lib/Color.h"

// ============================================================================
// MDRenderer
//
// Stateful single-pass renderer that walks through markdown lines and creates
// CIntObject widgets as children of the target viewport's content() container.
//
// Call run() once; it populates the @p widgets vector passed at construction.
// ============================================================================

class MDRenderer
{
public:
	MDRenderer(
		std::vector<std::shared_ptr<CIntObject>> & widgets,
		int viewportW,
		int textW,
		bool blueStyle,
		const std::function<void(const std::string &)> & onWikiLink,
		std::map<std::string, int> * anchors);

	/// Process the entire markdown document and populate @p widgets.
	void run(const std::string & markdownText);

private:
	// -----------------------------------------------------------------------
	// Configuration — set at construction, unchanged during rendering
	// -----------------------------------------------------------------------

	std::vector<std::shared_ptr<CIntObject>> & widgets;
	int  W;       ///< full viewport pixel width
	int  textW;   ///< usable text width (W minus left+right margins)
	bool blueStyle;
	const std::function<void(const std::string &)> & onWikiLink;
	std::map<std::string, int> * anchors;

	// -----------------------------------------------------------------------
	// Rendering state — mutated as lines are processed
	// -----------------------------------------------------------------------

	int curY = MD_MARGIN;

	/// Persistent alignment set by <left/center/right> tags.
	/// Cleared by the matching close-tag (</left> etc.).
	std::optional<MDAlign> pendingAlign;

	std::vector<std::string> paraBuffer;   ///< accumulated paragraph lines
	std::vector<std::string> tableBuffer;  ///< accumulated table rows

	// -----------------------------------------------------------------------
	// Alignment helpers
	// -----------------------------------------------------------------------

	MDAlign currentAlign(MDAlign defaultAlign) const;

	// -----------------------------------------------------------------------
	// Widget-emission helpers
	// -----------------------------------------------------------------------

	void addParagraph(
		const std::string & text,
		int x, int width, int gap,
		EFonts font, ColorRGBA color, ETextAlignment align);

	void renderListItemText(const std::string & text, int startX, int availW);

	// -----------------------------------------------------------------------
	// Buffer flushers
	// -----------------------------------------------------------------------

	void flushPara();
	void flushTable();

	// -----------------------------------------------------------------------
	// Inline layout
	// -----------------------------------------------------------------------

	/// Single token prepared for the word-wrapping layout pass.
	struct LayoutToken
	{
		std::string  text;
		MDInlineType type   = MDInlineType::PLAIN;
		std::string  target; ///< destination URI — only set for LINK tokens
		int          width  = 0; ///< measured pixel width used for line-breaking
	};

	std::vector<LayoutToken> buildLayoutTokens(const std::string & ln) const;

	void layoutInlineLine(
		const std::string & ln,
		int startX, int availW,
		int trailGap = MD_PARA_GAP);

	// -----------------------------------------------------------------------
	// Table rendering
	// -----------------------------------------------------------------------

	static constexpr int TABLE_CELL_PAD = 3;

	/// Holds measurement and content for one table cell.
	struct CellInfo
	{
		std::string text;
		bool        isMedia   = false;
		ParsedMedia media;
		int         measuredH = 16;
		MDAlign     textAlign = MDAlign::LEFT;
	};

	void measureTableCell(CellInfo & info, const std::string & cellText, int colW) const;

	void renderTableCell(
		const CellInfo & info,
		int mx, int my, int cw, int ch, int dataRowH);

	// -----------------------------------------------------------------------
	// Media rendering
	// -----------------------------------------------------------------------

	/// Appends alt-popup and/or link-overlay widgets for a media element.
	void appendMediaOverlays(
		const ParsedMedia & pm,
		int ex, int ey, int ew, int eh);

	void renderMediaLine(const ParsedMedia & pm);

	// -----------------------------------------------------------------------
	// Line handlers — each returns true if the line was consumed
	// -----------------------------------------------------------------------

	bool handleEmptyLine(const std::string & line);
	bool handleAlignTag(const std::string & line);
	bool handleBreakTags(const std::string & line);
	bool handleHeading(const std::string & line);
	bool handleHRule(const std::string & line);
	bool handleMedia(const std::string & line);
	bool handleBulletItem(const std::string & line);
	bool handleOrderedItem(const std::string & line);
	bool handleTableRow(const std::string & line);
	bool handleBlockquote(const std::string & line);
};
