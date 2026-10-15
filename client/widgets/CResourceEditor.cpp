/*
 * CResourceEditor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CResourceEditor.h"

#include "CNumberInput.h"
#include "TextControls.h"
#include "UiTheme.h"

#include "../GameInstance.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/entities/ResourceTypeHandler.h"
#include "../../lib/texts/MetaString.h"

namespace
{
constexpr int ROW_GAP = 4;
constexpr int MAX_AMOUNT = 999999999;
}

int CResourceEditor::getHeight(int columns)
{
	const int count = static_cast<int>(LIBRARY->resourceTypeHandler->getAllObjects().size());
	const int rows = (count + columns - 1) / columns;
	return rows * (UiTheme::CONTROL_HEIGHT + ROW_GAP);
}

CResourceEditor::CResourceEditor(const Rect & area, const ResourceSet & value, const std::function<void(const ResourceSet &)> & callback, int columns)
	: resources(LIBRARY->resourceTypeHandler->getAllObjects())
	, callback(callback)
{
	pos += area.topLeft();
	pos.w = area.w;
	pos.h = getHeight(columns);

	OBJECT_CONSTRUCTION;

	const int columnWidth = area.w / columns;
	const int labelWidth = columnWidth * 2 / 5;

	for(size_t i = 0; i < resources.size(); ++i)
	{
		const int column = static_cast<int>(i) % columns;
		const int row = static_cast<int>(i) / columns;
		const int x = column * columnWidth;
		const int y = row * (UiTheme::CONTROL_HEIGHT + ROW_GAP);

		MetaString name;
		name.appendName(resources[i]);

		labels.push_back(std::make_shared<CLabel>(x, y + UiTheme::CONTROL_HEIGHT / 2, UiTheme::FONT, ETextAlignment::CENTERLEFT, UiTheme::TEXT, name.toString(&GAME->translator()), labelWidth - UiTheme::SPACING));
		inputs.push_back(std::make_shared<CNumberInput>(Rect(x + labelWidth, y, columnWidth - labelWidth - UiTheme::SPACING, UiTheme::CONTROL_HEIGHT), value[resources[i]], -MAX_AMOUNT, MAX_AMOUNT, [this](int)
		{
			if(this->callback)
				this->callback(getValue());
		}, 1));
	}
}

ResourceSet CResourceEditor::getValue() const
{
	ResourceSet result;
	for(size_t i = 0; i < resources.size(); ++i)
		result[resources[i]] = inputs[i]->getValue();
	return result;
}

void CResourceEditor::setValue(const ResourceSet & value)
{
	for(size_t i = 0; i < resources.size(); ++i)
		inputs[i]->setValue(value[resources[i]]);
}
