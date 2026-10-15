/*
 * CResourceEditor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

#include "../../lib/ResourceSet.h"

class CLabel;
class CNumberInput;

/// Number input for every resource
class CResourceEditor : public CIntObject
{
	std::vector<std::shared_ptr<CLabel>> labels;
	std::vector<std::shared_ptr<CNumberInput>> inputs;
	std::vector<GameResID> resources;
	std::function<void(const ResourceSet &)> callback;

public:
	/// Height of the editor with the given number of columns
	static int getHeight(int columns);

	CResourceEditor(const Rect & area, const ResourceSet & value, const std::function<void(const ResourceSet &)> & callback = {}, int columns = 1);

	ResourceSet getValue() const;
	/// Changes the shown amounts without calling the callback
	void setValue(const ResourceSet & value);
};
