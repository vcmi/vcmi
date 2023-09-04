#ifndef ABSTRACTSETTINGS_H
#define ABSTRACTSETTINGS_H

#include <QWidget>
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/CGHeroInstance.h"

//parses date for lose condition (1m 1w 1d)
int expiredDate(const QString & date);
QString expiredDate(int date);
int3 posFromJson(const JsonNode & json);
std::vector<JsonNode> linearJsonArray(const JsonNode & json);

class AbstractSettings : public QWidget
{
	Q_OBJECT
public:
	explicit AbstractSettings(QWidget *parent = nullptr);
	virtual ~AbstractSettings() = default;

	virtual void initialize(const CMap & map) = 0;
	virtual void update(CMap & map) = 0;

	std::string getTownName(const CMap & map, int objectIdx);
	std::string getHeroName(const CMap & map, int objectIdx);
	std::string getMonsterName(const CMap & map, int objectIdx);

	static JsonNode conditionToJson(const EventCondition & event);

	template<class T>
	std::vector<int> getObjectIndexes(const CMap & map) const
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
	int getObjectByPos(const CMap & map, const int3 & pos)
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

signals:

};

#endif // ABSTRACTSETTINGS_H
