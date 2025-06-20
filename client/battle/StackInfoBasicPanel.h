/*
 * StackInfoBasicPanel.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

VCMI_LIB_NAMESPACE_BEGIN
class CStack;
VCMI_LIB_NAMESPACE_END

class CLabel;
class CMultiLineLabel;
class CAnimImage;

class StackInfoBasicPanel : public CIntObject
{
private:
	std::shared_ptr<CPicture> background;
	std::shared_ptr<CPicture> background2;
	std::vector<std::shared_ptr<CLabel>> labels;
	std::vector<std::shared_ptr<CMultiLineLabel>> labelsMultiline;
	std::vector<std::shared_ptr<CAnimImage>> icons;

public:
	StackInfoBasicPanel(const CStack * stack, bool initializeBackground);

	void show(Canvas & to) override;

	void initializeData(const CStack * stack);
	void update(const CStack * updatedInfo);
};
