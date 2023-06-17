/*
 * mapsettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "mapsettings.h"
#include "ui_mapsettings.h"
#include "mainwindow.h"

#include "../lib/CSkillHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CModHandler.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CGCreature.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/StringConstants.h"
#include "inspector/townbulidingswidget.h" //to convert BuildingID to string

//parses date for lose condition (1m 1w 1d)
int expiredDate(const QString & date)
{
	int result = 0;
	for(auto component : date.split(" "))
	{
		int days = component.left(component.lastIndexOf('d')).toInt();
		int weeks = component.left(component.lastIndexOf('w')).toInt();
		int months = component.left(component.lastIndexOf('m')).toInt();
		result += days > 0 ? days - 1 : 0;
		result += (weeks > 0 ? weeks - 1 : 0) * 7;
		result += (months > 0 ? months - 1 : 0) * 28;
	}
	return result;
}

QString expiredDate(int date)
{
	QString result;
	int m = date / 28;
	int w = (date % 28) / 7;
	int d = date % 7;
	if(m)
		result += QString::number(m) + "m";
	if(w)
	{
		if(!result.isEmpty())
			result += " ";
		result += QString::number(w) + "w";
	}
	if(d)
	{
		if(!result.isEmpty())
			result += " ";
		result += QString::number(d) + "d";
	}
	return result;
}

int3 posFromJson(const JsonNode & json)
{
	return int3(json.Vector()[0].Integer(), json.Vector()[1].Integer(), json.Vector()[2].Integer());
}

std::vector<JsonNode> linearJsonArray(const JsonNode & json)
{
	std::vector<JsonNode> result;
	if(json.getType() == JsonNode::JsonType::DATA_STRUCT)
		result.push_back(json);
	if(json.getType() == JsonNode::JsonType::DATA_VECTOR)
	{
		for(auto & node : json.Vector())
		{
			auto subvector = linearJsonArray(node);
			result.insert(result.end(), subvector.begin(), subvector.end());
		}
	}
	return result;
}

void traverseNode(QTreeWidgetItem * item, std::function<void(QTreeWidgetItem*)> action)
{
	// Do something with item
	action(item);
	for (int i = 0; i < item->childCount(); ++i)
		traverseNode(item->child(i), action);
}

MapSettings::MapSettings(MapController & ctrl, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MapSettings),
	controller(ctrl)
{
	ui->setupUi(this);

	assert(controller.map());

	ui->mapNameEdit->setText(tr(controller.map()->name.c_str()));
	ui->mapDescriptionEdit->setPlainText(tr(controller.map()->description.c_str()));
	ui->heroLevelLimit->setValue(controller.map()->levelLimit);
	ui->heroLevelLimitCheck->setChecked(controller.map()->levelLimit);
	
	show();
	
	for(int i = 0; i < controller.map()->allowedAbilities.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->skillh->objects[i]->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedAbilities[i] ? Qt::Checked : Qt::Unchecked);
		ui->listAbilities->addItem(item);
	}
	for(int i = 0; i < controller.map()->allowedSpell.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->spellh->objects[i]->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedSpell[i] ? Qt::Checked : Qt::Unchecked);
		ui->listSpells->addItem(item);
	}
	for(int i = 0; i < controller.map()->allowedArtifact.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->arth->objects[i]->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedArtifact[i] ? Qt::Checked : Qt::Unchecked);
		ui->listArts->addItem(item);
	}
	for(int i = 0; i < controller.map()->allowedHeroes.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->heroh->objects[i]->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedHeroes[i] ? Qt::Checked : Qt::Unchecked);
		ui->listHeroes->addItem(item);
	}
	
	//set difficulty
	switch(controller.map()->difficulty)
	{
		case 0:
			ui->diffRadio1->setChecked(true);
			break;
			
		case 1:
			ui->diffRadio2->setChecked(true);
			break;
			
		case 2:
			ui->diffRadio3->setChecked(true);
			break;
			
		case 3:
			ui->diffRadio4->setChecked(true);
			break;
			
		case 4:
			ui->diffRadio5->setChecked(true);
			break;
	};
	
	//victory & loss messages
	ui->victoryMessageEdit->setText(QString::fromStdString(controller.map()->victoryMessage));
	ui->defeatMessageEdit->setText(QString::fromStdString(controller.map()->defeatMessage));
	
	//victory & loss conditions
	const std::array<std::string, 8> conditionStringsWin = {
		QT_TR_NOOP("No special victory"),
		QT_TR_NOOP("Capture artifact"),
		QT_TR_NOOP("Hire creatures"),
		QT_TR_NOOP("Accumulate resources"),
		QT_TR_NOOP("Construct building"),
		QT_TR_NOOP("Capture town"),
		QT_TR_NOOP("Defeat hero"),
		QT_TR_NOOP("Transport artifact")
	};
	const std::array<std::string, 5> conditionStringsLose = {
		QT_TR_NOOP("No special loss"),
		QT_TR_NOOP("Lose castle"),
		QT_TR_NOOP("Lose hero"),
		QT_TR_NOOP("Time expired"),
		QT_TR_NOOP("Days without town")
	};
	
	for(auto & s : conditionStringsWin)
	{
		ui->victoryComboBox->addItem(QString::fromStdString(s));
	}
	ui->standardVictoryCheck->setChecked(false);
	ui->onlyForHumansCheck->setChecked(false);
	
	for(auto & s : conditionStringsLose)
	{
		ui->loseComboBox->addItem(QString::fromStdString(s));
	}
	ui->standardLoseCheck->setChecked(false);
	
	auto conditionToJson = [](const EventCondition & event) -> JsonNode
	{
		JsonNode result;
		result["condition"].Integer() = event.condition;
		result["value"].Integer() = event.value;
		result["objectType"].Integer() = event.objectType;
		result["objectSubytype"].Integer() = event.objectSubtype;
		result["objectInstanceName"].String() = event.objectInstanceName;
		result["metaType"].Integer() = (ui8)event.metaType;
		{
			auto & position = result["position"].Vector();
			position.resize(3);
			position[0].Float() = event.position.x;
			position[1].Float() = event.position.y;
			position[2].Float() = event.position.z;
		}
		return result;
	};
	
	for(auto & ev : controller.map()->triggeredEvents)
	{
		if(ev.effect.type == EventEffect::VICTORY)
		{
			if(ev.identifier == "standardVictory")
				ui->standardVictoryCheck->setChecked(true);

			if(ev.identifier == "specialVictory")
			{
				auto readjson = ev.trigger.toJson(conditionToJson);
				auto linearNodes = linearJsonArray(readjson);
				
				for(auto & json : linearNodes)
				{
					switch(json["condition"].Integer())
					{
						case EventCondition::HAVE_ARTIFACT: {
							ui->victoryComboBox->setCurrentIndex(1);
							assert(victoryTypeWidget);
							victoryTypeWidget->setCurrentIndex(json["objectType"].Integer());
							break;
						}
							
						case EventCondition::HAVE_CREATURES: {
							ui->victoryComboBox->setCurrentIndex(2);
							assert(victoryTypeWidget);
							assert(victoryValueWidget);
							auto idx = victoryTypeWidget->findData(int(json["objectType"].Integer()));
							victoryTypeWidget->setCurrentIndex(idx);
							victoryValueWidget->setText(QString::number(json["value"].Integer()));
							break;
						}
							
						case EventCondition::HAVE_RESOURCES: {
							ui->victoryComboBox->setCurrentIndex(3);
							assert(victoryTypeWidget);
							assert(victoryValueWidget);
							auto idx = victoryTypeWidget->findData(int(json["objectType"].Integer()));
							victoryTypeWidget->setCurrentIndex(idx);
							victoryValueWidget->setText(QString::number(json["value"].Integer()));
							break;
						}
							
						case EventCondition::HAVE_BUILDING: {
							ui->victoryComboBox->setCurrentIndex(4);
							assert(victoryTypeWidget);
							assert(victorySelectWidget);
							auto idx = victoryTypeWidget->findData(int(json["objectType"].Integer()));
							victoryTypeWidget->setCurrentIndex(idx);
							int townIdx = getObjectByPos<CGTownInstance>(posFromJson(json["position"]));
							if(townIdx >= 0)
							{
								auto idx = victorySelectWidget->findData(townIdx);
								victorySelectWidget->setCurrentIndex(idx);
							}
							break;
						}
							
						case EventCondition::CONTROL: {
							ui->victoryComboBox->setCurrentIndex(5);
							assert(victoryTypeWidget);
							if(json["objectType"].Integer() == Obj::TOWN)
							{
								int townIdx = getObjectByPos<CGTownInstance>(posFromJson(json["position"]));
								if(townIdx >= 0)
								{
									auto idx = victoryTypeWidget->findData(townIdx);
									victoryTypeWidget->setCurrentIndex(idx);
								}
							}
							//TODO: support control other objects (dwellings, mines)
							break;
						}
							
						case EventCondition::DESTROY: {
							ui->victoryComboBox->setCurrentIndex(6);
							assert(victoryTypeWidget);
							if(json["objectType"].Integer() == Obj::HERO)
							{
								int heroIdx = getObjectByPos<CGHeroInstance>(posFromJson(json["position"]));
								if(heroIdx >= 0)
								{
									auto idx = victoryTypeWidget->findData(heroIdx);
									victoryTypeWidget->setCurrentIndex(idx);
								}
							}
							//TODO: support control other objects (monsters)
							break;
						}
							
						case EventCondition::TRANSPORT: {
							ui->victoryComboBox->setCurrentIndex(7);
							assert(victoryTypeWidget);
							assert(victorySelectWidget);
							victoryTypeWidget->setCurrentIndex(json["objectType"].Integer());
							int townIdx = getObjectByPos<CGTownInstance>(posFromJson(json["position"]));
							if(townIdx >= 0)
							{
								auto idx = victorySelectWidget->findData(townIdx);
								victorySelectWidget->setCurrentIndex(idx);
							}
							break;
						}
							
						case EventCondition::IS_HUMAN: {
							ui->onlyForHumansCheck->setChecked(true);
							break;
						}
					};
				}
			}
		}
		
		if(ev.effect.type == EventEffect::DEFEAT)
		{
			if(ev.identifier == "standardDefeat")
				ui->standardLoseCheck->setChecked(true);
			
			if(ev.identifier == "specialDefeat")
			{
				auto readjson = ev.trigger.toJson(conditionToJson);
				auto linearNodes = linearJsonArray(readjson);
				
				for(auto & json : linearNodes)
				{
					switch(json["condition"].Integer())
					{
						case EventCondition::CONTROL: {
							if(json["objectType"].Integer() == Obj::TOWN)
							{
								ui->loseComboBox->setCurrentIndex(1);
								assert(loseTypeWidget);
								int townIdx = getObjectByPos<CGTownInstance>(posFromJson(json["position"]));
								if(townIdx >= 0)
								{
									auto idx = loseTypeWidget->findData(townIdx);
									loseTypeWidget->setCurrentIndex(idx);
								}
							}
							if(json["objectType"].Integer() == Obj::HERO)
							{
								ui->loseComboBox->setCurrentIndex(2);
								assert(loseTypeWidget);
								int heroIdx = getObjectByPos<CGHeroInstance>(posFromJson(json["position"]));
								if(heroIdx >= 0)
								{
									auto idx = loseTypeWidget->findData(heroIdx);
									loseTypeWidget->setCurrentIndex(idx);
								}
							}
							
							break;
						}
							
						case EventCondition::DAYS_PASSED: {
							ui->loseComboBox->setCurrentIndex(3);
							assert(loseValueWidget);
							loseValueWidget->setText(expiredDate(json["value"].Integer()));
							break;
						}
						
						case EventCondition::DAYS_WITHOUT_TOWN: {
							ui->loseComboBox->setCurrentIndex(4);
							assert(loseValueWidget);
							loseValueWidget->setText(QString::number(json["value"].Integer()));
							break;
							
						case EventCondition::IS_HUMAN:
							break; //ignore because always applicable for defeat conditions
						}
							
					};
				}
			}
		}
	}
	
	//mods management
	//collect all active mods
	QMap<QString, QTreeWidgetItem*> addedMods;
	QSet<QString> modsToProcess;
	ui->treeMods->blockSignals(true);
	
	auto createModTreeWidgetItem = [&](QTreeWidgetItem * parent, const CModInfo & modInfo)
	{
		auto item = new QTreeWidgetItem(parent, {QString::fromStdString(modInfo.name), QString::fromStdString(modInfo.version.toString())});
		item->setData(0, Qt::UserRole, QVariant(QString::fromStdString(modInfo.identifier)));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(0, controller.map()->mods.count(modInfo.identifier) ? Qt::Checked : Qt::Unchecked);
		//set parent check
		if(parent && item->checkState(0) == Qt::Checked)
			parent->setCheckState(0, Qt::Checked);
		return item;
	};
	
	for(const auto & modName : VLC->modh->getActiveMods())
	{
		QString qmodName = QString::fromStdString(modName);
		if(qmodName.split(".").size() == 1)
		{
			const auto & modInfo = VLC->modh->getModInfo(modName);
			addedMods[qmodName] = createModTreeWidgetItem(nullptr, modInfo);
			ui->treeMods->addTopLevelItem(addedMods[qmodName]);
		}
		else
		{
			modsToProcess.insert(qmodName);
		}
	}
	
	for(auto qmodIter = modsToProcess.begin(); qmodIter != modsToProcess.end();)
	{
		auto qmodName = *qmodIter;
		auto pieces = qmodName.split(".");
		assert(pieces.size() > 1);
		
		QString qs;
		for(int i = 0; i < pieces.size() - 1; ++i)
			qs += pieces[i];
		
		if(addedMods.count(qs))
		{
			const auto & modInfo = VLC->modh->getModInfo(qmodName.toStdString());
			addedMods[qmodName] = createModTreeWidgetItem(addedMods[qs], modInfo);
			modsToProcess.erase(qmodIter);
			qmodIter = modsToProcess.begin();
		}
		else
			++qmodIter;
	}
	ui->treeMods->blockSignals(false);
}

MapSettings::~MapSettings()
{
	delete ui;
}

std::string MapSettings::getTownName(int townObjectIdx)
{
	std::string name;
	if(auto town = dynamic_cast<CGTownInstance*>(controller.map()->objects[townObjectIdx].get()))
	{
		auto * ctown = town->town;
		if(!ctown)
			ctown = VLC->townh->randomTown;

		name = ctown->faction ? town->getObjectName() : town->getNameTranslated() + ", (random)";
	}
	return name;
}

std::string MapSettings::getHeroName(int townObjectIdx)
{
	std::string name;
	if(auto hero = dynamic_cast<CGHeroInstance*>(controller.map()->objects[townObjectIdx].get()))
	{
		name = hero->getNameTranslated();
	}
	return name;
}

std::string MapSettings::getMonsterName(int monsterObjectIdx)
{
	std::string name;
	[[maybe_unused]] auto monster = dynamic_cast<CGCreature*>(controller.map()->objects[monsterObjectIdx].get());
	if(monster)
	{
		//TODO: get proper name
		//name = hero->name;
	}
	return name;
}

void MapSettings::updateModWidgetBasedOnMods(const ModCompatibilityInfo & mods)
{
	//Mod management
	auto widgetAction = [&](QTreeWidgetItem * item)
	{
		auto modName = item->data(0, Qt::UserRole).toString().toStdString();
		item->setCheckState(0, mods.count(modName) ? Qt::Checked : Qt::Unchecked);
	};
	
	for (int i = 0; i < ui->treeMods->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *item = ui->treeMods->topLevelItem(i);
		traverseNode(item, widgetAction);
	}
}

void MapSettings::on_pushButton_clicked()
{
	controller.map()->name = ui->mapNameEdit->text().toStdString();
	controller.map()->description = ui->mapDescriptionEdit->toPlainText().toStdString();
	if(ui->heroLevelLimitCheck->isChecked())
		controller.map()->levelLimit = ui->heroLevelLimit->value();
	else
		controller.map()->levelLimit = 0;
	controller.commitChangeWithoutRedraw();
	
	for(int i = 0; i < controller.map()->allowedAbilities.size(); ++i)
	{
		auto * item = ui->listAbilities->item(i);
		controller.map()->allowedAbilities[i] = item->checkState() == Qt::Checked;
	}
	for(int i = 0; i < controller.map()->allowedSpell.size(); ++i)
	{
		auto * item = ui->listSpells->item(i);
		controller.map()->allowedSpell[i] = item->checkState() == Qt::Checked;
	}
	for(int i = 0; i < controller.map()->allowedArtifact.size(); ++i)
	{
		auto * item = ui->listArts->item(i);
		controller.map()->allowedArtifact[i] = item->checkState() == Qt::Checked;
	}
	for(int i = 0; i < controller.map()->allowedHeroes.size(); ++i)
	{
		auto * item = ui->listHeroes->item(i);
		controller.map()->allowedHeroes[i] = item->checkState() == Qt::Checked;
	}
	
	//set difficulty
	if(ui->diffRadio1->isChecked()) controller.map()->difficulty = 0;
	if(ui->diffRadio2->isChecked()) controller.map()->difficulty = 1;
	if(ui->diffRadio3->isChecked()) controller.map()->difficulty = 2;
	if(ui->diffRadio4->isChecked()) controller.map()->difficulty = 3;
	if(ui->diffRadio5->isChecked()) controller.map()->difficulty = 4;
	
	//victory & loss messages
	
	controller.map()->victoryMessage = ui->victoryMessageEdit->text().toStdString();
	controller.map()->defeatMessage = ui->defeatMessageEdit->text().toStdString();
	
	//victory & loss conditions
	EventCondition victoryCondition(EventCondition::STANDARD_WIN);
	EventCondition defeatCondition(EventCondition::DAYS_WITHOUT_TOWN);
	defeatCondition.value = 7;

	//Victory condition - defeat all
	TriggeredEvent standardVictory;
	standardVictory.effect.type = EventEffect::VICTORY;
	standardVictory.effect.toOtherMessage = "core.genrltxt.5";
	standardVictory.identifier = "standardVictory";
	standardVictory.description.clear(); // TODO: display in quest window
	standardVictory.onFulfill = "core.genrltxt.659";
	standardVictory.trigger = EventExpression(victoryCondition);

	//Loss condition - 7 days without town
	TriggeredEvent standardDefeat;
	standardDefeat.effect.type = EventEffect::DEFEAT;
	standardDefeat.effect.toOtherMessage = "core.genrltxt.8";
	standardDefeat.identifier = "standardDefeat";
	standardDefeat.description.clear(); // TODO: display in quest window
	standardDefeat.onFulfill = "core.genrltxt.7";
	standardDefeat.trigger = EventExpression(defeatCondition);
	
	controller.map()->triggeredEvents.clear();
	
	//VICTORY
	if(ui->victoryComboBox->currentIndex() == 0)
	{
		controller.map()->triggeredEvents.push_back(standardVictory);
		controller.map()->victoryIconIndex = 11;
		controller.map()->victoryMessage = VLC->generaltexth->victoryConditions[0];
	}
	else
	{
		int vicCondition = ui->victoryComboBox->currentIndex() - 1;
		
		TriggeredEvent specialVictory;
		specialVictory.effect.type = EventEffect::VICTORY;
		specialVictory.identifier = "specialVictory";
		specialVictory.description.clear(); // TODO: display in quest window
		
		controller.map()->victoryIconIndex = vicCondition;
		controller.map()->victoryMessage = VLC->generaltexth->victoryConditions[size_t(vicCondition) + 1];
		
		switch(vicCondition)
		{
			case 0: {
				EventCondition cond(EventCondition::HAVE_ARTIFACT);
				assert(victoryTypeWidget);
				cond.objectType = victoryTypeWidget->currentData().toInt();
				specialVictory.effect.toOtherMessage = "core.genrltxt.281";
				specialVictory.onFulfill = "core.genrltxt.280";
				specialVictory.trigger = EventExpression(cond);
				break;
			}
				
			case 1: {
				EventCondition cond(EventCondition::HAVE_CREATURES);
				assert(victoryTypeWidget);
				cond.objectType = victoryTypeWidget->currentData().toInt();
				cond.value = victoryValueWidget->text().toInt();
				specialVictory.effect.toOtherMessage = "core.genrltxt.277";
				specialVictory.onFulfill = "core.genrltxt.276";
				specialVictory.trigger = EventExpression(cond);
				break;
			}
				
			case 2: {
				EventCondition cond(EventCondition::HAVE_RESOURCES);
				assert(victoryTypeWidget);
				cond.objectType = victoryTypeWidget->currentData().toInt();
				cond.value = victoryValueWidget->text().toInt();
				specialVictory.effect.toOtherMessage = "core.genrltxt.279";
				specialVictory.onFulfill = "core.genrltxt.278";
				specialVictory.trigger = EventExpression(cond);
				break;
			}
				
			case 3: {
				EventCondition cond(EventCondition::HAVE_BUILDING);
				assert(victoryTypeWidget);
				cond.objectType = victoryTypeWidget->currentData().toInt();
				int townIdx = victorySelectWidget->currentData().toInt();
				if(townIdx > -1)
					cond.position = controller.map()->objects[townIdx]->pos;
				specialVictory.effect.toOtherMessage = "core.genrltxt.283";
				specialVictory.onFulfill = "core.genrltxt.282";
				specialVictory.trigger = EventExpression(cond);
				break;
			}
				
			case 4: {
				EventCondition cond(EventCondition::CONTROL);
				assert(victoryTypeWidget);
				cond.objectType = Obj::TOWN;
				int townIdx = victoryTypeWidget->currentData().toInt();
				cond.position = controller.map()->objects[townIdx]->pos;
				specialVictory.effect.toOtherMessage = "core.genrltxt.250";
				specialVictory.onFulfill = "core.genrltxt.249";
				specialVictory.trigger = EventExpression(cond);
				break;
			}
				
			case 5: {
				EventCondition cond(EventCondition::DESTROY);
				assert(victoryTypeWidget);
				cond.objectType = Obj::HERO;
				int heroIdx = victoryTypeWidget->currentData().toInt();
				cond.position = controller.map()->objects[heroIdx]->pos;
				specialVictory.effect.toOtherMessage = "core.genrltxt.253";
				specialVictory.onFulfill = "core.genrltxt.252";
				specialVictory.trigger = EventExpression(cond);
				break;
			}
				
			case 6: {
				EventCondition cond(EventCondition::TRANSPORT);
				assert(victoryTypeWidget);
				cond.objectType = victoryTypeWidget->currentData().toInt();
				int townIdx = victorySelectWidget->currentData().toInt();
				if(townIdx > -1)
					cond.position = controller.map()->objects[townIdx]->pos;
				specialVictory.effect.toOtherMessage = "core.genrltxt.293";
				specialVictory.onFulfill = "core.genrltxt.292";
				specialVictory.trigger = EventExpression(cond);
				break;
			}
				
		}
		
		// if condition is human-only turn it into following construction: AllOf(human, condition)
		if(ui->onlyForHumansCheck->isChecked())
		{
			EventExpression::OperatorAll oper;
			EventCondition notAI(EventCondition::IS_HUMAN);
			notAI.value = 1;
			oper.expressions.push_back(notAI);
			oper.expressions.push_back(specialVictory.trigger.get());
			specialVictory.trigger = EventExpression(oper);
		}

		// if normal victory allowed - add one more quest
		if(ui->standardVictoryCheck->isChecked())
		{
			controller.map()->victoryMessage += " / ";
			controller.map()->victoryMessage += VLC->generaltexth->victoryConditions[0];
			controller.map()->triggeredEvents.push_back(standardVictory);
		}
		controller.map()->triggeredEvents.push_back(specialVictory);
	}
	
	//DEFEAT
	if(ui->loseComboBox->currentIndex() == 0)
	{
		controller.map()->triggeredEvents.push_back(standardDefeat);
		controller.map()->defeatIconIndex = 3;
		controller.map()->defeatMessage = VLC->generaltexth->lossCondtions[0];
	}
	else
	{
		int lossCondition = ui->loseComboBox->currentIndex() - 1;
		
		TriggeredEvent specialDefeat;
		specialDefeat.effect.type = EventEffect::DEFEAT;
		specialDefeat.identifier = "specialDefeat";
		specialDefeat.description.clear(); // TODO: display in quest window
		
		controller.map()->defeatIconIndex = lossCondition;
		controller.map()->defeatMessage = VLC->generaltexth->lossCondtions[size_t(lossCondition) + 1];
		
		switch(lossCondition)
		{
			case 0: {
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::TOWN;
				assert(loseTypeWidget);
				int townIdx = loseTypeWidget->currentData().toInt();
				cond.position = controller.map()->objects[townIdx]->pos;
				noneOf.expressions.push_back(cond);
				specialDefeat.onFulfill = "core.genrltxt.251";
				specialDefeat.trigger = EventExpression(noneOf);
				break;
			}
				
			case 1: {
				EventExpression::OperatorNone noneOf;
				EventCondition cond(EventCondition::CONTROL);
				cond.objectType = Obj::HERO;
				assert(loseTypeWidget);
				int townIdx = loseTypeWidget->currentData().toInt();
				cond.position = controller.map()->objects[townIdx]->pos;
				noneOf.expressions.push_back(cond);
				specialDefeat.onFulfill = "core.genrltxt.253";
				specialDefeat.trigger = EventExpression(noneOf);
				break;
			}
				
			case 2: {
				EventCondition cond(EventCondition::DAYS_PASSED);
				assert(loseValueWidget);
				cond.value = expiredDate(loseValueWidget->text());
				specialDefeat.onFulfill = "core.genrltxt.254";
				specialDefeat.trigger = EventExpression(cond);
				break;
			}
				
			case 3: {
				EventCondition cond(EventCondition::DAYS_WITHOUT_TOWN);
				assert(loseValueWidget);
				cond.value = loseValueWidget->text().toInt();
				specialDefeat.onFulfill = "core.genrltxt.7";
				specialDefeat.trigger = EventExpression(cond);
				break;
			}
		}
		
		EventExpression::OperatorAll allOf;
		EventCondition isHuman(EventCondition::IS_HUMAN);
		isHuman.value = 1;

		allOf.expressions.push_back(specialDefeat.trigger.get());
		allOf.expressions.push_back(isHuman);
		specialDefeat.trigger = EventExpression(allOf);

		if(ui->standardLoseCheck->isChecked())
		{
			controller.map()->triggeredEvents.push_back(standardDefeat);
		}
		controller.map()->triggeredEvents.push_back(specialDefeat);
	}
	
	//Mod management
	auto widgetAction = [&](QTreeWidgetItem * item)
	{
		if(item->checkState(0) == Qt::Checked)
		{
			auto modName = item->data(0, Qt::UserRole).toString().toStdString();
			controller.map()->mods[modName] = VLC->modh->getModInfo(modName).version;
		}
	};
	
	controller.map()->mods.clear();
	for (int i = 0; i < ui->treeMods->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *item = ui->treeMods->topLevelItem(i);
		traverseNode(item, widgetAction);
	}
	
	close();
}

void MapSettings::on_victoryComboBox_currentIndexChanged(int index)
{
	delete victoryTypeWidget;
	delete victoryValueWidget;
	delete victorySelectWidget;
	victoryTypeWidget = nullptr;
	victoryValueWidget = nullptr;
	victorySelectWidget = nullptr;
	
	if(index == 0)
	{
		ui->standardVictoryCheck->setChecked(true);
		ui->standardVictoryCheck->setEnabled(false);
		ui->onlyForHumansCheck->setChecked(false);
		ui->onlyForHumansCheck->setEnabled(false);
		return;
	}
	ui->onlyForHumansCheck->setEnabled(true);
	ui->standardVictoryCheck->setEnabled(true);
	
	int vicCondition = index - 1;
	switch(vicCondition)
	{
		case 0: { //EventCondition::HAVE_ARTIFACT
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i = 0; i < controller.map()->allowedArtifact.size(); ++i)
				victoryTypeWidget->addItem(QString::fromStdString(VLC->arth->objects[i]->getNameTranslated()), QVariant::fromValue(i));
			break;
		}
			
		case 1: { //EventCondition::HAVE_CREATURES
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i = 0; i < VLC->creh->objects.size(); ++i)
				victoryTypeWidget->addItem(QString::fromStdString(VLC->creh->objects[i]->getNamePluralTranslated()), QVariant::fromValue(i));
			
			victoryValueWidget = new QLineEdit;
			ui->victoryParamsLayout->addWidget(victoryValueWidget);
			victoryValueWidget->setText("1");
			break;
		}
			
		case 2: { //EventCondition::HAVE_RESOURCES
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			{
				for(int resType = 0; resType < GameConstants::RESOURCE_QUANTITY; ++resType)
				{
					auto resName = QString::fromStdString(GameConstants::RESOURCE_NAMES[resType]);
					victoryTypeWidget->addItem(resName, QVariant::fromValue(resType));
				}
			}
			
			victoryValueWidget = new QLineEdit;
			ui->victoryParamsLayout->addWidget(victoryValueWidget);
			victoryValueWidget->setText("1");
			break;
		}
			
		case 3: { //EventCondition::HAVE_BUILDING
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			auto * ctown = VLC->townh->randomTown;
			for(int bId : ctown->getAllBuildings())
				victoryTypeWidget->addItem(QString::fromStdString(defaultBuildingIdConversion(BuildingID(bId))), QVariant::fromValue(bId));
			
			victorySelectWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victorySelectWidget);
			victorySelectWidget->addItem("Any town", QVariant::fromValue(-1));
			for(int i : getObjectIndexes<CGTownInstance>())
				victorySelectWidget->addItem(getTownName(i).c_str(), QVariant::fromValue(i));
			break;
		}
			
		case 4: { //EventCondition::CONTROL (Obj::TOWN)
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i : getObjectIndexes<CGTownInstance>())
				victoryTypeWidget->addItem(tr(getTownName(i).c_str()), QVariant::fromValue(i));
			break;
		}
			
		case 5: { //EventCondition::DESTROY (Obj::HERO)
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i : getObjectIndexes<CGHeroInstance>())
				victoryTypeWidget->addItem(tr(getHeroName(i).c_str()), QVariant::fromValue(i));
			break;
		}
			
		case 6: { //EventCondition::TRANSPORT (Obj::ARTEFACT)
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i = 0; i < controller.map()->allowedArtifact.size(); ++i)
				victoryTypeWidget->addItem(QString::fromStdString(VLC->arth->objects[i]->getNameTranslated()), QVariant::fromValue(i));
			
			victorySelectWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victorySelectWidget);
			for(int i : getObjectIndexes<CGTownInstance>())
				victorySelectWidget->addItem(tr(getTownName(i).c_str()), QVariant::fromValue(i));
			break;
		}
			
			
		//TODO: support this vectory type
		// in order to do that, need to implement finding creature by position
		// selecting from map would be the best user experience
		/*case 7: { //EventCondition::DESTROY (Obj::MONSTER)
			victoryTypeWidget = new QComboBox;
			ui->loseParamsLayout->addWidget(victoryTypeWidget);
			for(int i : getObjectIndexes<CGCreature>())
				victoryTypeWidget->addItem(tr(getMonsterName(i).c_str()), QVariant::fromValue(i));
			break;
		}*/
			
			
	}
}


void MapSettings::on_loseComboBox_currentIndexChanged(int index)
{
	delete loseTypeWidget;
	delete loseValueWidget;
	delete loseSelectWidget;
	loseTypeWidget = nullptr;
	loseValueWidget = nullptr;
	loseSelectWidget = nullptr;
	
	if(index == 0)
	{
		ui->standardLoseCheck->setChecked(true);
		ui->standardLoseCheck->setEnabled(false);
		return;
	}
	ui->standardLoseCheck->setEnabled(true);
	
	int loseCondition = index - 1;
	switch(loseCondition)
	{
		case 0: {  //EventCondition::CONTROL (Obj::TOWN)
			loseTypeWidget = new QComboBox;
			ui->loseParamsLayout->addWidget(loseTypeWidget);
			for(int i : getObjectIndexes<CGTownInstance>())
				loseTypeWidget->addItem(tr(getTownName(i).c_str()), QVariant::fromValue(i));
			break;
		}
			
		case 1: { //EventCondition::CONTROL (Obj::HERO)
			loseTypeWidget = new QComboBox;
			ui->loseParamsLayout->addWidget(loseTypeWidget);
			for(int i : getObjectIndexes<CGHeroInstance>())
				loseTypeWidget->addItem(tr(getHeroName(i).c_str()), QVariant::fromValue(i));
			break;
		}
			
		case 2: { //EventCondition::DAYS_PASSED
			loseValueWidget = new QLineEdit;
			ui->loseParamsLayout->addWidget(loseValueWidget);
			loseValueWidget->setText("2m 1w 1d");
			break;
		}
			
		case 3: { //EventCondition::DAYS_WITHOUT_TOWN
			loseValueWidget = new QLineEdit;
			ui->loseParamsLayout->addWidget(loseValueWidget);
			loseValueWidget->setText("7");
			break;
		}
	}
}


void MapSettings::on_heroLevelLimitCheck_toggled(bool checked)
{
	ui->heroLevelLimit->setEnabled(checked);
}

void MapSettings::on_modResolution_map_clicked()
{
	updateModWidgetBasedOnMods(MapController::modAssessmentMap(*controller.map()));
}


void MapSettings::on_modResolution_full_clicked()
{
	updateModWidgetBasedOnMods(MapController::modAssessmentAll());
}

void MapSettings::on_treeMods_itemChanged(QTreeWidgetItem *item, int column)
{
	//set state for children
	for (int i = 0; i < item->childCount(); ++i)
		item->child(i)->setCheckState(0, item->checkState(0));
	
	//set state for parent
	ui->treeMods->blockSignals(true);
	if(item->checkState(0) == Qt::Checked)
	{
		while(item->parent())
		{
			item->parent()->setCheckState(0, Qt::Checked);
			item = item->parent();
		}
	}
	ui->treeMods->blockSignals(false);
}

