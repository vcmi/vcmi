/*
 * EditorValidator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CDialogWindow.h"

class CMap;
class CFormView;

/// Checks of the map that is edited
namespace EditorValidator
{
	struct Issue
	{
		std::string message;
		bool critical;

		bool operator<(const Issue & other) const
		{
			return message < other.message;
		}
	};

	std::set<Issue> validate(const CMap * map);
}

/// Window that lists the problems of the map
class EditorValidatorWindow : public CDialogWindow
{
	std::shared_ptr<CFormView> list;

public:
	explicit EditorValidatorWindow(const CMap * map);

	static void show(const CMap * map);
};
