/*
 * entitiesselector.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QWidget>

#include "../StdInc.h"
#include "../../lib/constants/EntityIdentifiers.h"

namespace Ui {
class EntitiesSelector;
}

using EntityIds = std::variant<std::reference_wrapper<std::set<TerrainId>>, std::reference_wrapper<std::set<SpellID>>, std::reference_wrapper<std::set<ArtifactID>>, std::reference_wrapper<std::set<SecondarySkill>>, std::reference_wrapper<std::set<HeroTypeID>>>;

class EntitiesSelector : public QDialog
{
	Q_OBJECT

public:
	explicit EntitiesSelector(EntityIds & entities);

	static void showEntitiesSelector(EntityIds & entities);

private slots:
	void on_buttonBoxResult_accepted();
    void on_buttonBoxResult_rejected();

private:
	Ui::EntitiesSelector *ui;

	EntityIds & entitiesSelected;
};
