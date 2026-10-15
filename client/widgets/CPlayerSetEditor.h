/*
 * CPlayerSetEditor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

#include "../../lib/constants/EntityIdentifiers.h"

class CCheckBox;

/// Check box for every player
class CPlayerSetEditor : public CIntObject
{
	std::vector<std::shared_ptr<CCheckBox>> checks;
	std::function<void(const std::set<PlayerColor> &)> callback;

public:
	static int getHeight(int columns);

	CPlayerSetEditor(const Rect & area, const std::set<PlayerColor> & value, const std::function<void(const std::set<PlayerColor> &)> & callback = {}, int columns = 2);

	std::set<PlayerColor> getValue() const;
	/// Changes the checked players without calling the callback
	void setValue(const std::set<PlayerColor> & value);
	/// The players that can not be changed. They are shown as checked
	void setLocked(const std::set<PlayerColor> & locked);
};
