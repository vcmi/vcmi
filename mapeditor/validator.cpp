/*
 * validator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "validator.h"
#include "mapcontroller.h"
#include "ui_validator.h"
#include "../lib/mapObjects/MapObjects.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CModHandler.h"

Validator::Validator(const CMap * map, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::Validator)
{
	ui->setupUi(this);

	show();
	
	setAttribute(Qt::WA_DeleteOnClose);

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
		//check player settings
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

		std::set<CHero*> allHeroesOnMap; //used to find hero duplicated
		
		//checking all objects in the map
		for(auto o : map->objects)
		{
			//owners for objects
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
			if(o->getOwner() != PlayerColor::NEUTRAL && o->getOwner().getNum() < map->players.size())
			{
				if(!map->players[o->getOwner().getNum()].canAnyonePlay())
					issues.emplace_back(QString("Object %1 is assinged to non-playable player %2").arg(o->instanceName.c_str(), o->getOwner().getStr().c_str()), true);
			}
			//checking towns
			if(auto * ins = dynamic_cast<CGTownInstance*>(o.get()))
			{
				bool has = amountOfCastles.count(ins->getOwner().getNum());
				if(!has && ins->getOwner() != PlayerColor::NEUTRAL)
					issues.emplace_back(tr("Town %1 has undefined owner %2").arg(ins->instanceName.c_str(), ins->getOwner().getStr().c_str()), true);
				if(has)
					++amountOfCastles[ins->getOwner().getNum()];
			}
			//checking heroes and prisons
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
				}
				if(ins->type)
				{
					if(!map->allowedHeroes[ins->type->getId().getNum()])
						issues.emplace_back(QString("Hero %1 is prohibited by map settings").arg(ins->type->getNameTranslated().c_str()), false);
					
					if(!allHeroesOnMap.insert(ins->type).second)
						issues.emplace_back(QString("Hero %1 has duplicate on map").arg(ins->type->getNameTranslated().c_str()), false);
				}
				else
					issues.emplace_back(QString("Hero %1 has an empty type and must be removed").arg(ins->instanceName.c_str()), true);
			}
			
			//checking for arts
			if(auto * ins = dynamic_cast<CGArtifact*>(o.get()))
			{
				if(ins->ID == Obj::SPELL_SCROLL)
				{
					if(ins->storedArtifact)
					{
						if(!map->allowedSpell[ins->storedArtifact->id.getNum()])
							issues.emplace_back(QString("Spell scroll %1 is prohibited by map settings").arg(ins->getObjectName().c_str()), false);
					}
					else
						issues.emplace_back(QString("Spell scroll %1 doesn't have instance assigned and must be removed").arg(ins->instanceName.c_str()), true);
				}
				else
				{
					if(ins->ID == Obj::ARTIFACT && !map->allowedArtifact[ins->subID])
					{
						issues.emplace_back(QString("Artifact %1 is prohibited by map settings").arg(ins->getObjectName().c_str()), false);
					}
				}
			}
		}

		//verification of starting towns
		for(auto & mp : amountOfCastles)
			if(mp.second == 0)
				issues.emplace_back(QString("Player %1 doesn't have any starting town").arg(mp.first), false);

		//verification of map name and description
		if(map->name.empty())
			issues.emplace_back("Map name is not specified", false);
		if(map->description.empty())
			issues.emplace_back("Map description is not specified", false);
		
		//verificationfor mods
		for(auto & mod : MapController::modAssessmentMap(*map))
		{
			if(!map->mods.count(mod.first))
			{
				issues.emplace_back(QString("Map contains object from mod \"%1\", but doesn't require it").arg(QString::fromStdString(VLC->modh->getModInfo(mod.first).name)), true);
			}
		}
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
