/*
 * MDParser.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

// ============================================================================
// Layout constants — shared between parser and renderer.
// ============================================================================

constexpr int MD_MARGIN      = 4;   ///< left/right margin for block elements (px)
constexpr int MD_GAP         = 8;   ///< vertical gap after standalone elements (px)
constexpr int MD_PARA_GAP    = 4;   ///< smaller gap after paragraph/list text (px)
constexpr int MD_BULLET_X    = MD_MARGIN + 4;   ///< x-position of the •  bullet dot
constexpr int MD_BULLET_SZ   = 4;               ///< bullet dot side length (px)
constexpr int MD_BULLET_TXT  = MD_MARGIN + 14;  ///< x-position where bullet text starts
constexpr int MD_ORDERED_TXT = MD_MARGIN + 20;  ///< x-position where ordered list text starts
constexpr int MD_H1_PAD_TOP  = 12;  ///< extra top padding before # headings
constexpr int MD_H2_PAD_TOP  = 8;   ///< extra top padding before ## headings
constexpr int MD_H3_PAD_TOP  = 4;   ///< extra top padding before ### headings

// ============================================================================
// Block-level alignment state — set by <left>, <center>, <right> tags.
// ============================================================================

enum class MDAlign { LEFT, CENTER, RIGHT };

/// Describes an opening or closing alignment tag found on its own line.
struct MDAlignTag
{
	MDAlign align;
	bool    close = false; ///< true for </center>, </left>, </right>
};

// ============================================================================
// Inline token types
//
// Bold (**), italic (*), and inline-code (`) markers are detected by the
// tokenizer, their inner text is returned as PLAIN, and a one-time warning
// is emitted — these constructs are not rendered by the VCMI wiki engine.
// ============================================================================

enum class MDInlineType { PLAIN, LINK };

struct MDInlineToken
{
	std::string  text;
	MDInlineType type   = MDInlineType::PLAIN;
	std::string  target; ///< destination URI — only set for LINK tokens
};

// ============================================================================
// ParsedMedia — result of MDParser::parseMediaLine()
// ============================================================================

struct ParsedMedia
{
	std::string path;
	std::string alt;
	std::string linkTarget; ///< non-empty for linked-media syntax: [![](path)](uri)
	int  frame    = 0;
	bool hasFrame = false;  ///< path contained a #N frame-index suffix
	bool isAnim   = false;  ///< .def or .json extension → AnimationPath
	bool isVideo  = false;  ///< .bik / .smk / .webm / .mp4 → VideoPath
};

// ============================================================================
// MDParser — stateless, pure string-parsing utilities.
// ============================================================================

namespace MDParser
{
	// -----------------------------------------------------------------------
	// Block-level parsers
	// -----------------------------------------------------------------------

	/// Returns heading level 1–3 and sets @p outText (trailing whitespace stripped).
	/// Returns 0 when @p line is not a heading.
	int parseHeading(const std::string & line, std::string & outText);

	/// Returns true when @p line is a horizontal rule (≥3 of ---, ___,  or ***).
	bool isHRule(const std::string & line);

	/// Returns true for unordered list items ("- " or "* " prefix), sets @p outText.
	bool parseBullet(const std::string & line, std::string & outText);

	/// Returns the ordinal (≥0) for ordered list items ("N. " prefix), sets @p outText.
	/// Returns -1 when the line is not an ordered list item.
	int parseOrdered(const std::string & line, std::string & outText);

	/// Returns the alignment tag description when @p line is exactly one of
	/// <left>, <center>, <right>, </left>, </center>, </right>.
	std::optional<MDAlignTag> parseAlignTag(const std::string & line);

	/// Extracts the anchor name from the first <a id="name" /> or <a name="name" /> tag
	/// found anywhere in @p s.  Returns an empty string when no tag is present.
	std::string parseAnchorTag(const std::string & s);

	/// Returns all anchor names found in @p s (in left-to-right order).
	std::vector<std::string> parseAllAnchorTags(const std::string & s);

	/// Returns @p s with all <a id|name="…" /> tags removed.
	std::string stripAnchorTags(const std::string & s);

	/// Parses a media line: "![alt](path)" or "[![alt](path)](uri)".
	/// Returns true on success; @p out is populated.
	bool parseMediaLine(const std::string & line, ParsedMedia & out);

	// -----------------------------------------------------------------------
	// Table helpers
	// -----------------------------------------------------------------------

	/// Returns true when @p line is a pipe-table row (starts and ends with '|').
	bool isTableRow(const std::string & line);

	/// Returns true when @p row is an all-dash/pipe separator row (|---|---|).
	bool isTableSeparator(const std::string & row);

	/// Splits a trimmed pipe-table row into individual cell strings.
	std::vector<std::string> splitTableRow(const std::string & row);

	// -----------------------------------------------------------------------
	// Inline helpers
	// -----------------------------------------------------------------------

	/// Tokenises @p line into PLAIN and LINK inline spans.
	/// Bold (**), italic (*), and inline-code (`) markers are consumed, their
	/// inner text appended to the preceding PLAIN token, and a one-time warning
	/// is logged for each unsupported construct type encountered.
	std::vector<MDInlineToken> tokenizeInline(const std::string & line);

	/// Returns true when @p s contains characters that may begin an inline span.
	/// Used to decide whether to run the full inline tokenizer or a simple label.
	bool hasInlineMarkup(const std::string & s);

	/// Splits @p text at space boundaries while preserving VCMI {color|…} tag
	/// prefixes across word boundaries so each returned chunk is self-contained.
	std::vector<std::string> splitPlainWithColorRewrap(const std::string & text);

	/// Returns @p s with all VCMI {…} colour-tag markup stripped; only the
	/// visible characters are kept.
	std::string stripColorTags(const std::string & s);

	/// Returns @p str with every occurrence of @p from replaced by @p to.
	std::string replaceAll(std::string str, const std::string & from, const std::string & to);

	/// Strips the leading alignment tag pair from @p s in-place and returns the
	/// detected MDAlign.  Returns MDAlign::LEFT when no tag is found.
	MDAlign parseCellAlign(std::string & s);
}
