/*
 * WikiCommon.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../gui/CIntObject.h"
#include "../../widgets/GraphicalPrimitiveCanvas.h"
#include "../../../lib/Color.h"
#include "../../../lib/ResourceSet.h"
#include "../../../lib/filesystem/ResourcePath.h"
#include "../../../lib/TerrainHandler.h"

class Canvas;

// ─────────────────────────────────────────────────────────────────────────────
// WikiIconInfo – optional icon descriptor for a wiki list row
// ─────────────────────────────────────────────────────────────────────────────

/// Optional icon descriptor for a WikiListItem row
struct WikiIconInfo
{
	AnimationPath path;
	size_t frame = 0;
	size_t group = 0;
};

/// Returns a WikiIconInfo for the given terrain type.
/// Prefers the first tile of terrain's tile animation; falls back to minimapUnblocked color.
WikiIconInfo terrainIconInfo(const TerrainType * terrain);

/// Returns a colored gender span string ({#RRGGBB|glyph}) for inline label markup.
/// Uses ♂/♀ Unicode glyphs when FONT_SMALL supports them; otherwise falls back
/// to the translatable "vcmi.wiki.gender.male" / "vcmi.wiki.gender.female" keys.
std::string wikiGenderSpan(bool isFemale);

/// Returns the outer border colour for the given wiki theme (brown / blue).
inline ColorRGBA wikiBorderColor(bool blue) noexcept
{
	return blue ? ColorRGBA{80, 140, 220, 220} : ColorRGBA{200, 180, 120, 220};
}

/// Returns the inner divider colour for the given wiki theme.
inline ColorRGBA wikiInnerColor(bool blue) noexcept
{
	return blue ? ColorRGBA{30, 60, 120, 160} : ColorRGBA{90, 80, 50, 160};
}

/// Unified clickable overlay for wiki content rows and image areas.
/// Supports left-click (navigate), right-click (info popup), and hover highlight.
/// Pass a non-empty @p clip to restrict hit-testing to a visible viewport rect.
class WikiClickable : public CIntObject
{
	std::function<void()> onLeftClick;
	std::function<void()> onRightClick;
	std::optional<Rect>   clipRect;
	bool hovered   = false;
	bool blueTheme = false;

public:
	WikiClickable(Rect area,
	              std::function<void()> lclick,
	              std::function<void()> rclick,
	              bool blue,
	              std::optional<Rect> clip = std::nullopt);

	void clickPressed(const Point & cur) override;
	void showPopupWindow(const Point & cur) override;
	void hover(bool on) override;
	void showAll(Canvas & to) override;
};

/// Styled table-grid widget.
///
/// When @p headerH > 0 a filled header row and a separator line are added
/// above the data rows.  When @p headerH == 0 only the outer border,
/// inter-row dividers, and column separators are drawn (key-value grid style).
class WikiTableGrid : public GraphicalPrimitiveCanvas
{
public:
	WikiTableGrid(
		int x, int y, int totalW,
		const std::vector<int> & colWidths,
		int headerH,
		int dataRowH,
		int dataRowCount,
		bool blue = false);
	WikiTableGrid(
		int x, int y, int totalW,
		std::initializer_list<int> colWidths,
		int headerH,
		int dataRowH,
		int dataRowCount,
		bool blue = false);
};

/// Horizontal row of resource icon + amount pairs.
///
/// Icons and labels are laid out from (@p startX, @p y) and clipped to
/// @p maxWidth.  The widget's @p pos covers all rendered children.
class WikiResourceCost : public CIntObject
{
	std::vector<std::shared_ptr<CIntObject>> items; ///< keeps child shared_ptrs alive

public:
	WikiResourceCost(
		const TResources & cost,
		int startX, int y,
		int maxWidth);
};
