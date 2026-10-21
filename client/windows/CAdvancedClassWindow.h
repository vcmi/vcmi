/*
 * CAdvancedClassWindow.h, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 */
#pragma once

#include "CWindowObject.h"

struct AdvancedClassDefinition;
class CGHeroInstance;
class CButton;
class CLabel;
class CListBox;
class CTextBox;
class CViewport;
class CFilledTexture;
class TransparentFilledRectangle;

class CAdvancedClassWindow final : public CWindowObject
{
	const CGHeroInstance * hero;
	std::vector<const AdvancedClassDefinition *> candidates;
	const AdvancedClassDefinition * selected = nullptr;
	bool selectionPending = false;

	std::shared_ptr<CFilledTexture> backgroundTexture;
	std::shared_ptr<TransparentFilledRectangle> candidateBackground;
	std::shared_ptr<TransparentFilledRectangle> treeBackground;
	std::shared_ptr<TransparentFilledRectangle> detailBackground;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CLabel> candidateHeader;
	std::shared_ptr<CLabel> treeHeader;
	std::shared_ptr<CListBox> candidateList;
	std::shared_ptr<CViewport> treeViewport;
	std::shared_ptr<CTextBox> detailText;
	std::shared_ptr<CButton> selectButton;
	std::shared_ptr<CButton> closeButton;
	std::vector<std::shared_ptr<CIntObject>> treeWidgets;

	void rebuildCandidates();
	void selectDefinition(const AdvancedClassDefinition * definition);
	void rebuildTree();
	void updateDetails();
	void confirmSelection();

public:
	explicit CAdvancedClassWindow(const CGHeroInstance * hero);
	void refresh();
};
