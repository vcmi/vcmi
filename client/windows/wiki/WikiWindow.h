/*
 * WikiWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "WikiCommon.h"
#include "../CWindowObject.h"
#include "../../widgets/CViewport.h"

#include "../../../lib/constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN
class CGHeroInstance;
VCMI_LIB_NAMESPACE_END

class CButton;
class CLabel;
class CListBox;
class CTextBox;
class CTextInput;
class CFilledTexture;
class TransparentFilledRectangle;
class CAnimImage;
class Canvas;

/// Wiki category identifiers
enum class WikiCategory : int
{
	GLOSSARY  = 0,
	TOWN      = 1,
	HERO      = 2,
	CREATURE  = 3,
	ARTIFACT  = 4,
	SPELL     = 5,
	SKILL     = 6,
	TERRAIN   = 7,
	MOD       = 8,
	COUNT     = 9
};

/// A single wiki entry – identifier (JSON key), name (translated), optional description and optional icon
struct WikiEntry
{
	std::string identifier;   ///< unique entity identifier / JSON key (used for lookup)
	std::string name;         ///< translated display name (shown in list)
	std::string description;  ///< full description; empty = show auto-stub text
	std::optional<WikiIconInfo> icon;
	std::string modScope;     ///< mod scope the entity belongs to (empty for glossary)
	std::string subtitle;     ///< second display line (hero class, faction, alignment, entry count)
};

/// A single clickable row inside the category or element columns
class WikiListItem : public CIntObject
{
	// Private constants – sizes / margins
	static constexpr int ICON_SIZE  = 28;  ///< icon rendered at this square size (doubled for 40 px rows)
	static constexpr int MARGIN_L   = 6;   ///< left padding
	static constexpr int MARGIN_TOP = 2;   ///< top padding for text

	// Private members
	std::shared_ptr<CAnimImage>  icon;
	std::shared_ptr<CLabel>      label;
	std::shared_ptr<CLabel>      sublabel;       ///< second line (hero class / faction / alignment / count)
	ColorRGBA                    sublabelColor;  ///< hint colour picked once in ctor (blue or brown)
	bool selected = false;
	bool blueStyle = false;

	void updateLook();

public:
	static constexpr int ITEM_H = 40;  ///< row height – set on item->pos.h by callers

	std::function<void(WikiListItem *)> onSelected;
	std::string text;
	size_t index = 0;

	WikiListItem(size_t index, std::string text, std::string subtitle, // NOSONAR (8 params needed for widget configuration)
	             std::function<void(WikiListItem *)> callback,
	             std::optional<WikiIconInfo> iconInfo = std::nullopt,
	             bool blueStyle = false,
	             int itemWidth = 100,
	             bool hasSlider = false);

	void clickPressed(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void hover(bool on) override;
	void showAll(Canvas & to) override;
	void setSelected(bool sel);
	bool isSelected() const { return selected; }
};

/// Identifies a specific entry to directly open the Wiki at.
struct WikiEntryKey
{
	WikiCategory category; ///< Which category tab to open
	std::string entryName; ///< Entity identifier / JSON key (used for lookup, not a translated string)
	/// Optional anchor to scroll to after the entry is displayed.
	/// Corresponds to the id/name attribute of an <a id="name" /> tag in the entry's Markdown.
	std::string anchor = {};

	bool operator==(const WikiEntryKey & o) const { return category == o.category && entryName == o.entryName; }
};

/// In-game Glossary / Wiki - 800x600 stub window
/// Layout has three vertically-scrollable columns:
///   [Categories] | [Elements] | [Content (largest)]
class WikiWindow : public CWindowObject
{
public:
	/// Visual colour theme – matches CSlider::EStyle values
	enum class Style { BROWN = 0, BLUE = 1 };

private:
	Style style;

	// --- background & decoration -----------------------------------------
	std::shared_ptr<CFilledTexture>             bgTexture;
	std::shared_ptr<TransparentFilledRectangle> col1Bg;
	std::shared_ptr<TransparentFilledRectangle> col2Bg;
	std::shared_ptr<TransparentFilledRectangle> col3Bg;

	// --- title + column headers -------------------------------------------
	std::shared_ptr<CLabel> titleLabel;
	std::shared_ptr<CLabel> col1Header;
	std::shared_ptr<CLabel> col2Header;
	std::shared_ptr<CLabel> col3Header;

	// --- selection lists --------------------------------------------------
	std::shared_ptr<CListBox> categoryList;
	std::shared_ptr<CListBox> elementList;

	// selected state
	/// Currently active category index (-1 = none)
	int activeCategoryIndex = -1;
	/// Currently active element index within the category (-1 = none)
	int activeElementIndex  = -1;

	// stub data
	std::vector<std::string> categoryNames;
	/// Maps category index -> list of entries
	std::vector<std::vector<WikiEntry>> categoryEntries;

	// --- element search box (below element list) -------------------------
	std::shared_ptr<TransparentFilledRectangle> searchBoxRect;
	std::shared_ptr<CLabel>                     searchBoxHint;
	std::shared_ptr<CTextInput>                 searchBox;

	/// Currently displayed (possibly filtered) entries; activeElementIndex indexes into this
	std::vector<WikiEntry> currentDisplayedEntries;

	// --- content area -----------------------------------------------------
	std::shared_ptr<CTextBox>  contentBox;
	std::shared_ptr<CViewport> townContentView; ///< scrollable viewport for Town category
	std::vector<std::shared_ptr<CIntObject>> townContentWidgets; ///< ownership for viewport children
	std::string currentTownName; ///< name of the town currently displayed in the viewport

	std::shared_ptr<CViewport> creatureContentView; ///< scrollable viewport for Creature category
	std::vector<std::shared_ptr<CIntObject>> creatureContentWidgets;
	std::string currentCreatureName;

	std::shared_ptr<CViewport> heroContentView; ///< scrollable viewport for Hero category
	std::vector<std::shared_ptr<CIntObject>> heroContentWidgets;
	std::string currentHeroName;

	std::shared_ptr<CViewport> artifactContentView; ///< scrollable viewport for Artifact category
	std::vector<std::shared_ptr<CIntObject>> artifactContentWidgets;
	std::string currentArtifactName;

	std::shared_ptr<CViewport> modContentView; ///< scrollable viewport for Mod category
	std::vector<std::shared_ptr<CIntObject>> modContentWidgets;
	std::string currentModId;

	std::shared_ptr<CViewport> glossaryContentView; ///< scrollable viewport for Glossary (markdown renderer)
	std::vector<std::shared_ptr<CIntObject>> glossaryContentWidgets;
	std::string currentGlossaryEntryName; ///< identifier of the Glossary entry currently shown
	std::map<std::string, int> glossaryAnchorMap; ///< anchor name -> Y offset for the current glossary entry
	std::string pendingAnchor; ///< anchor to scroll to on the next rebuildGlossaryViewport() call

	/// Custom mod-defined categories: string id -> index in categoryNames / categoryEntries
	std::map<std::string, int> customCategoryIds;
	/// Per-custom-category viewport, keyed by category index
	std::map<int, std::shared_ptr<CViewport>> customCategoryViews;
	/// Per-custom-category content widgets, keyed by category index
	std::map<int, std::vector<std::shared_ptr<CIntObject>>> customCategoryWidgets;
	/// Per-custom-category anchor maps, keyed by category index
	std::map<int, std::map<std::string, int>> customCategoryAnchorMaps;
	/// Identifier of the entry currently shown for each custom category
	std::map<int, std::string> customCategoryCurrentEntry;

	// --- navigation history -----------------------------------------------
	std::vector<WikiEntryKey> navHistory; ///< back-navigation stack
	std::shared_ptr<CButton> backButton;

	// --- controls ---------------------------------------------------------
	std::shared_ptr<CButton> closeButton;

	/// Mod-scope label shown in the content header area (hidden for Glossary)
	std::shared_ptr<CLabel> modScopeLabel;

	// Helpers
	void buildCategoryList();
	void buildElementList(int categoryIndex);
	void clearElementList();
	void updateContent();
	/// Applies correct scroll bounds to both slider widgets so that
	/// mouse-wheel scrolling works when the cursor is anywhere in the window.
	void applyScrollBounds();
	/// Called when the search text in the element search box changes.
	void onSearchInput();

	void onCategoryClicked(int index);
	void onElementClicked(int index);
	void navigateTo(const WikiEntryKey & key);
	void navigateBack();

	/// Rebuilds the town viewport content for the given faction name.
	void rebuildTownViewport(const std::string & factionName);
	/// Rebuilds the creature viewport content for the given creature name.
	void rebuildCreatureViewport(const std::string & creatureName);
	/// Rebuilds the hero viewport content for the given hero name.
	void rebuildHeroViewport(const std::string & heroName);
	/// Rebuilds the artifact viewport content for the given artifact key.
	void rebuildArtifactViewport(const std::string & artKey);
	/// Rebuilds the mod viewport content for the given mod ID.
	void rebuildModViewport(const std::string & modId);
	/// Rebuilds the glossary viewport by rendering the entry description as Markdown.
	void rebuildGlossaryViewport(const std::string & entryName);
	/// Rebuilds a custom-category viewport for the given category index and entry.
	void rebuildCustomCategoryViewport(int catIdx, const std::string & entryName);

	/// Returns the CGHeroInstance on the currently loaded map whose hero type matches id,
	/// or nullptr if no map is loaded or no such hero exists.
	const CGHeroInstance * findMapHero(HeroTypeID id) const;

	/// Shared core: (re-)creates a CViewport, renders markdownText into it, handles
	/// pendingAnchor, calls applyScrollBounds + totalRedraw.
	void rebuildMarkdownViewport(std::shared_ptr<CViewport> & view,
	                             std::vector<std::shared_ptr<CIntObject>> & widgets,
	                             std::map<std::string, int> & anchorMap,
	                             const std::string & markdownText);

	/// Returns a link callback suitable for buildMarkdownContent that resolves
	/// "wiki:category/entry[#anchor]" links using BUILTIN_CATEGORY_MAP and customCategoryIds.
	std::function<void(const std::string &)> makeLinkCallback();

	/// Handles a resolved "wiki:category/entry[#anchor]" link, called from makeLinkCallback.
	void handleWikiLink(const std::string & target);

	/// Shared callback logic when a category list item is clicked.
	void onCatItemSelectedCallback(WikiListItem * clicked);

	/// Shared callback logic when an element list item is clicked.
	void onElemItemSelectedCallback(WikiListItem * clicked);

public:
	explicit WikiWindow(Style style = Style::BROWN, std::optional<WikiEntryKey> initialEntry = std::nullopt);
	~WikiWindow() override;

	void keyPressed(EShortcut key) override;
};
