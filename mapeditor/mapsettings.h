/*
 * mapsettings.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QDialog>
#include "mapcontroller.h"
#include "../lib/mapping/CMap.h"

namespace Ui {
class MapSettings;
}

class MapSettings : public QDialog
{
	Q_OBJECT

public:
	explicit MapSettings(MapController & controller, QWidget *parent = nullptr);
	~MapSettings();

private slots:
	void on_pushButton_clicked();

	void on_victoryComboBox_currentIndexChanged(int index);

	void on_loseComboBox_currentIndexChanged(int index);

	void on_heroLevelLimitCheck_toggled(bool checked);

	void on_modResolution_map_clicked();

	void on_modResolution_full_clicked();

	void on_treeMods_itemChanged(QTreeWidgetItem *item, int column);

private:
	
	std::string getTownName(int townObjectIdx);
	std::string getHeroName(int townObjectIdx);
	std::string getMonsterName(int townObjectIdx);
	
	void updateModWidgetBasedOnMods(const ModCompatibilityInfo & mods);
	
	template<class T>
	std::vector<int> getObjectIndexes() const
	{
		std::vector<int> result;
		for(int i = 0; i < controller.map()->objects.size(); ++i)
		{
			if(auto town = dynamic_cast<T*>(controller.map()->objects[i].get()))
				result.push_back(i);
		}
		return result;
	}
	
	template<class T>
	int getObjectByPos(const int3 & pos)
	{
		for(int i = 0; i < controller.map()->objects.size(); ++i)
		{
			if(auto town = dynamic_cast<T*>(controller.map()->objects[i].get()))
			{
				if(town->pos == pos)
					return i;
			}
		}
		return -1;
	}
	
	Ui::MapSettings *ui;
	MapController & controller;
	
	QComboBox * victoryTypeWidget = nullptr, * loseTypeWidget = nullptr;
	QComboBox * victorySelectWidget = nullptr, * loseSelectWidget = nullptr;
	QLineEdit * victoryValueWidget = nullptr, * loseValueWidget = nullptr;
};
