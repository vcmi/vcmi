#include "StdInc.h"
#include "validator.h"
#include "ui_validator.h"
#include "../lib/mapObjects/MapObjects.h"

Validator::Validator(const CMap * map, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::Validator)
{
	ui->setupUi(this);

	show();

	std::array<QString, 2> icons{"mapeditor/icons/mod-update.png", "mapeditor/icons/mod-delete.png"};

	for(auto & issue : Validator::validate(map))
	{
		auto * item = new QListWidgetItem(QIcon(icons[issue.critical ? 1 : 0]), issue.message);
		ui->listWidget->addItem(item);
	}
}

Validator::~Validator()
{
	delete ui;
}

std::list<Validator::Issue> Validator::validate(const CMap * map)
{
	std::list<Validator::Issue> issues;
	
	if(!map)
	{
		issues.emplace_back("Map is not loaded", true);
		return issues;
	}
	
	try
	{
		int hplayers = 0;
		int cplayers = 0;
		std::map<int, int> amountOfCastles;
		for(int i = 0; i < map->players.size(); ++i)
		{
			auto & p = map->players[i];
			if(p.canAnyonePlay())
				amountOfCastles[i] = 0;
			if(p.canComputerPlay)
				++cplayers;
			if(p.canHumanPlay)
				++hplayers;
			if(p.allowedFactions.empty())
				issues.emplace_back(QString("No factions allowed for player %1").arg(i), true);
		}
		if(hplayers + cplayers == 0)
			issues.emplace_back("No players allowed to play this map", true);
		if(hplayers + cplayers == 1)
			issues.emplace_back("Map is allowed for one player and cannot be started", true);
		if(!hplayers)
			issues.emplace_back("No human players allowed to play this map", true);

		for(auto o : map->objects)
		{
			if(o->getOwner() == PlayerColor::UNFLAGGABLE)
			{
				if(dynamic_cast<CGMine*>(o.get()) ||
				   dynamic_cast<CGDwelling*>(o.get()) ||
				   dynamic_cast<CGTownInstance*>(o.get()) ||
				   dynamic_cast<CGGarrison*>(o.get()) ||
				   dynamic_cast<CGHeroInstance*>(o.get()))
				{
					issues.emplace_back(QString("Armored instance %1 is UNFLAGGABLE but must have NEUTRAL or player owner").arg(o->instanceName.c_str()), true);
				}
			}
			if(auto * ins = dynamic_cast<CGTownInstance*>(o.get()))
			{
				bool has = amountOfCastles.count(ins->getOwner().getNum());
				if(!has && ins->getOwner() != PlayerColor::NEUTRAL)
					issues.emplace_back(QString("Town %1 has undefined owner %s").arg(ins->instanceName.c_str(), ins->getOwner().getStr().c_str()), true);
				if(has)
					++amountOfCastles[ins->getOwner().getNum()];
			}
			if(auto * ins = dynamic_cast<CGHeroInstance*>(o.get()))
			{
				if(ins->ID == Obj::PRISON)
				{
					if(ins->getOwner() != PlayerColor::NEUTRAL)
						issues.emplace_back(QString("Prison %1 must be a NEUTRAL").arg(ins->instanceName.c_str()), true);
				}
				else
				{
					bool has = amountOfCastles.count(ins->getOwner().getNum());
					if(!has)
						issues.emplace_back(QString("Hero %1 must have an owner").arg(ins->instanceName.c_str()), true);
					else
						issues.emplace_back(QString("Hero %1: heroes on map are not supported in current version").arg(ins->instanceName.c_str()), false);
				}
			}
		}

		for(auto & mp : amountOfCastles)
			if(mp.second == 0)
				issues.emplace_back(QString("Player %1 doesn't have any starting town").arg(mp.first), false);

		bool roadWarning = false;
		bool riverWarning = false;
		for(int k : {0, 1})
		{
			if(k && !map->twoLevel)
				break;
			for(int j = 0; j < map->height; ++j)
			{
				for(int i = 0; i < map->width; ++i)
				{
					if(map->getTile(int3(i, j, k)).roadType != ROAD_NAMES[0])
						roadWarning = true;
					if(map->getTile(int3(i, j, k)).riverType != RIVER_NAMES[0])
						riverWarning = true;
				}
			}
		}

		if(roadWarning)
			issues.emplace_back("Roads are presented on the map but not supported by Map Editor", false);
		if(riverWarning)
			issues.emplace_back("Rivers are presented on the map but not supported by Map Editor", false);

		if(map->name.empty())
			issues.emplace_back("Map name is not specified", false);
		if(map->description.empty())
			issues.emplace_back("Map description is not specified", false);
	}
	catch(const std::exception & e)
	{
		issues.emplace_back(QString("Exception occurs during validation: %1").arg(e.what()), true);
	}
	catch(...)
	{
		issues.emplace_back("Unknown exception occurs during validation", true);
	}
	
	return issues;
}
