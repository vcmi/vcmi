/*
 * CPlayerSetEditor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CPlayerSetEditor.h"

#include "CCheckBox.h"
#include "UiTheme.h"

#include "../GameInstance.h"

#include "../../lib/texts/MetaString.h"

int CPlayerSetEditor::getHeight(int columns)
{
	const int rows = (PlayerColor::PLAYER_LIMIT_I + columns - 1) / columns;
	return rows * UiTheme::CONTROL_HEIGHT;
}

CPlayerSetEditor::CPlayerSetEditor(const Rect & area, const std::set<PlayerColor> & value, const std::function<void(const std::set<PlayerColor> &)> & callback, int columns)
	: callback(callback)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = getHeight(columns);

	OBJECT_CONSTRUCTION;

	const int columnWidth = area.w / columns;

	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		MetaString name;
		name.appendName(PlayerColor(i));

		const int column = i % columns;
		const int row = i / columns;

		checks.push_back(std::make_shared<CCheckBox>(Rect(column * columnWidth, row * UiTheme::CONTROL_HEIGHT, columnWidth, UiTheme::CONTROL_HEIGHT),
			name.toString(&GAME->translator()), value.count(PlayerColor(i)) != 0, [this](bool)
		{
			if(this->callback)
				this->callback(getValue());
		}));
	}
}

void CPlayerSetEditor::setValue(const std::set<PlayerColor> & value)
{
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
		checks[i]->setChecked(value.count(PlayerColor(i)) != 0);
}

void CPlayerSetEditor::setLocked(const std::set<PlayerColor> & locked)
{
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		const bool isLocked = locked.count(PlayerColor(i)) != 0;
		if(isLocked)
			checks[i]->setChecked(true);

		checks[i]->block(isLocked);
	}
}

std::set<PlayerColor> CPlayerSetEditor::getValue() const
{
	std::set<PlayerColor> result;
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
		if(checks[i]->isChecked())
			result.insert(PlayerColor(i));
	return result;
}
