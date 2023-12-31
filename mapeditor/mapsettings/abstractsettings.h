/*
 * abstractsettings.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QWidget>
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

//parses date for lose condition (1m 1w 1d)
int expiredDate(const QString & date);
QString expiredDate(int date);
int3 posFromJson(const JsonNode & json);
std::vector<JsonNode> linearJsonArray(const JsonNode & json);

class MapController;

class AbstractSettings : public QWidget
{
	Q_OBJECT
public:
	explicit AbstractSettings(QWidget *parent = nullptr);
	virtual ~AbstractSettings() = default;

	virtual void initialize(MapController & controller);
	virtual void update() = 0;

	static std::string getTownName(const CMap & map, int objectIdx);
	static std::string getHeroName(const CMap & map, int objectIdx);
	static std::string getMonsterName(const CMap & map, int objectIdx);

	static JsonNode conditionToJson(const EventCondition & event);

	template<class T>
	static std::vector<int> getObjectIndexes(const CMap & map)
	{
		std::vector<int> result;
		for(int i = 0; i < map.objects.size(); ++i)
		{
			if(auto obj = dynamic_cast<T*>(map.objects[i].get()))
				result.push_back(i);
		}
		return result;
	}

	template<class T>
	static int getObjectByPos(const CMap & map, const int3 & pos)
	{
		for(int i = 0; i < map.objects.size(); ++i)
		{
			if(auto obj = dynamic_cast<T*>(map.objects[i].get()))
			{
				if(obj->pos == pos)
					return i;
			}
		}
		return -1;
	}

protected:
	MapController * controller = nullptr;

};
