/*
 * WikiMarkdown.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class CIntObject;
class CViewport;

/// Renders a subset of Markdown into a flat list of CIntObject widgets that
/// populate a CViewport's content() container.
///
/// Supported syntax
/// ----------------
///   # / ## / ###              Headings — FONT_BIG / FONT_MEDIUM / FONT_SMALL,
///                             yellow.  Default alignment is LEFT.
///   ---  ___  ***             Horizontal rule (≥3 identical chars on one line).
///   - text  /  * text         Unordered bullet list item.
///   N. text                   Ordered list item (any leading integer + dot).
///   ![alt](path.ext)          Static image (png, pcx, bmp, …).  Downscaled
///                             proportionally when wider than the viewport.
///   ![alt](path.def)          Animation — all frames looped at ~6 fps.
///   ![alt](path.def#N)        Single static frame N from an animation.
///   ![alt](path.bik)          Video (.bik / .smk / .webm / .mp4), looped and
///                             downscaled automatically to fit the viewport.
///   <center>  <left>  <right> Persistent alignment block tag.  Close with the
///   </center> </left> </right> matching close tag to reset alignment.  Affects
///                             headings, images, animations, and video.
///   | H1 | H2 |               GFM-style pipe table.  First row is the header;
///   |----------|              second row must be a separator (|---|---|).
///   | c1 | c2 |               Cells may contain any media syntax listed above.
///   [text](wiki:category/id)  Clickable link to another wiki page.  Categories:
///                             glossary, creature, spell, hero, town, artifact,
///                             skill, terrain, mod, or a custom mod category id.
///                             Append #anchor to jump to a named section.
///   [![alt](path)](wiki:id)   Clickable media link — left-click navigates,
///                             right-click shows the alt-text tooltip.
///   <a id="name" />           Invisible named anchor (also <a name="name" />).
///                             Embedding in a heading is allowed:
///                               ## <a id="top" />Section title
///                             When @p anchors is non-null the map is populated
///                             with name → content Y-offset (px) entries.
///   {VCMI color tags}         Passed through as-is to all text labels.
///   regular paragraphs        Auto-wrapped CMultiLineLabel.  A blank line ends
///                             the current paragraph and adds a vertical gap.
///
/// Unsupported syntax (logged as warnings if encountered)
/// ------------------------------------------------------
///   **bold**    Bold inline spans — H3 bitmap fonts do not support bold.
///   *italic*    Italic inline spans — not supported by H3 bitmap fonts.
///   `code`      Inline code spans — no monospace font available in-game.
///   ``` … ```   Fenced code blocks — same reason as above.
///   > quote     Blockquotes — not part of the minimal wiki feature set.
///   <p>         HTML paragraph tag — use a blank line instead.
///   <br>        HTML line-break tag — use a blank line instead.
///
/// For any of the above, a one-time warning is emitted to vcmi.log and the
/// inner text is rendered as a plain paragraph where possible.
///
/// @param viewport       CViewport whose content() is the construction target.
/// @param markdownText   Markdown source text (may contain VCMI {tag} syntax).
/// @param viewportWidth  Available pixel width for content.
/// @param blueStyle      True → blue wiki theme; false → brown theme.
/// @param onWikiLink     Optional callback invoked when a wiki link is clicked.
///                       Receives the raw URI, e.g. "wiki:creature/imp".
/// @param anchors        Optional output map: anchor name → content Y-offset (px).
/// @return               Flat list of created widgets (parented to viewport.content()).
std::vector<std::shared_ptr<CIntObject>> buildMarkdownContent(
	const CViewport & viewport,
	const std::string & markdownText,
	int viewportWidth,
	bool blueStyle,
	const std::function<void(const std::string &)> & onWikiLink = nullptr,
	std::map<std::string, int> * anchors = nullptr);
