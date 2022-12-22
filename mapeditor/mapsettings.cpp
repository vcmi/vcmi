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

MapSettings::MapSettings(MapController & ctrl, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MapSettings),
	controller(ctrl)
{
	ui->setupUi(this);

	assert(controller.map());

	ui->mapNameEdit->setText(tr(controller.map()->name.c_str()));
	ui->mapDescriptionEdit->setPlainText(tr(controller.map()->description.c_str()));
	
	show();
	
	
	for(int i = 0; i < controller.map()->allowedAbilities.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->skillh->objects[i]->getName()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedAbilities[i] ? Qt::Checked : Qt::Unchecked);
		ui->listAbilities->addItem(item);
	}
	for(int i = 0; i < controller.map()->allowedSpell.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->spellh->objects[i]->getName()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedSpell[i] ? Qt::Checked : Qt::Unchecked);
		ui->listSpells->addItem(item);
	}
	for(int i = 0; i < controller.map()->allowedArtifact.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->arth->objects[i]->getName()));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(controller.map()->allowedArtifact[i] ? Qt::Checked : Qt::Unchecked);
		ui->listArts->addItem(item);
	}
	for(int i = 0; i < controller.map()->allowedHeroes.size(); ++i)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(VLC->heroh->objects[i]->getName()));
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
	/*namespace EVictoryConditionType
	{
		enum EVictoryConditionType { ARTIFACT, GATHERTROOP, GATHERRESOURCE, BUILDCITY, BUILDGRAIL, BEATHERO,
			CAPTURECITY, BEATMONSTER, TAKEDWELLINGS, TAKEMINES, TRANSPORTITEM, WINSTANDARD = 255 };
	}

	namespace ELossConditionType
	{
		enum ELossConditionType { LOSSCASTLE, LOSSHERO, TIMEEXPIRES, LOSSSTANDARD = 255 };
	}*/
	//internal use, deprecated
	/*HAVE_ARTIFACT,     // type - required artifact
	HAVE_CREATURES,    // type - creatures to collect, value - amount to collect
	HAVE_RESOURCES,    // type - resource ID, value - amount to collect
	HAVE_BUILDING,     // position - town, optional, type - building to build
	CONTROL,           // position - position of object, optional, type - type of object
	DESTROY,           // position - position of object, optional, type - type of object
	TRANSPORT,         // position - where artifact should be transported, type - type of artifact

	//map format version pre 1.0
	DAYS_PASSED,       // value - number of days from start of the game
	IS_HUMAN,          // value - 0 = player is AI, 1 = player is human
	DAYS_WITHOUT_TOWN, // value - how long player can live without town, 0=instakill
	STANDARD_WIN,      // normal defeat all enemies condition
	CONST_VALUE,        // condition that always evaluates to "value" (0 = false, 1 = true) */
	const std::array<std::string, 8> conditionStringsWin = {
		"No special victory",
		"Have artifact",
		"Have creatures",
		"Have resources",
		"Have building",
		"Capture object",
		"Destroy object",
		"Transport artifact"
	};
	const std::array<std::string, 5> conditionStringsLose = {
		"No special loss",
		"Lose castle",
		"Lose hero",
		"Time expired",
		"Days without town"
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
							victoryTypeWidget->setCurrentIndex(json["objectType"].Integer());
							victoryValueWidget->setText(QString::number(json["value"].Integer()));
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
								int townIdx = getTownByPos(posFromJson(json["position"]));
								if(townIdx >= 0)
								{
									auto idx = loseTypeWidget->findData(townIdx);
									loseTypeWidget->setCurrentIndex(idx);
								}
							}
							if(json["objectType"].Integer() == Obj::HERO)
							{
								ui->loseComboBox->setCurrentIndex(2);
								//assert(loseValueWidget);
								//loseValueWidget->setText(QString::number(json["value"].Integer()));
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
							break; //ignore as always applicable for defeat conditions
						}
							
					};
				}
			}
		}
	}

	//ui8 difficulty;
	//ui8 levelLimit;

	//std::string victoryMessage;
	//std::string defeatMessage;
	//ui16 victoryIconIndex;
	//ui16 defeatIconIndex;

	//std::vector<PlayerInfo> players; /// The default size of the vector is PlayerColor::PLAYER_LIMIT.
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

		name = ctown->faction ? town->getObjectName() : town->name + ", (random)";
	}
	return name;
}

std::vector<int> MapSettings::getTownIndexes() const
{
	std::vector<int> result;
	for(int i = 0; i < controller.map()->objects.size(); ++i)
	{
		if(auto town = dynamic_cast<CGTownInstance*>(controller.map()->objects[i].get()))
		{
			result.push_back(i);
		}
	}
	return result;
}

int MapSettings::getTownByPos(const int3 & pos)
{
	for(int i = 0; i < controller.map()->objects.size(); ++i)
	{
		if(auto town = dynamic_cast<CGTownInstance*>(controller.map()->objects[i].get()))
		{
			if(town->pos == pos)
				return i;
		}
	}
	return -1;
}

void MapSettings::on_pushButton_clicked()
{
	controller.map()->name = ui->mapNameEdit->text().toStdString();
	controller.map()->description = ui->mapDescriptionEdit->toPlainText().toStdString();
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
	standardVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[5];
	standardVictory.identifier = "standardVictory";
	standardVictory.description.clear(); // TODO: display in quest window
	standardVictory.onFulfill = VLC->generaltexth->allTexts[659];
	standardVictory.trigger = EventExpression(victoryCondition);

	//Loss condition - 7 days without town
	TriggeredEvent standardDefeat;
	standardDefeat.effect.type = EventEffect::DEFEAT;
	standardDefeat.effect.toOtherMessage = VLC->generaltexth->allTexts[8];
	standardDefeat.identifier = "standardDefeat";
	standardDefeat.description.clear(); // TODO: display in quest window
	standardDefeat.onFulfill = VLC->generaltexth->allTexts[7];
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
				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[281];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[280];
				specialVictory.trigger = EventExpression(cond);
				break;
			}
				
			case 1: {
				EventCondition cond(EventCondition::HAVE_CREATURES);
				assert(victoryTypeWidget);
				cond.objectType = victoryTypeWidget->currentData().toInt();
				cond.value = victoryValueWidget->text().toInt();
				specialVictory.effect.toOtherMessage = VLC->generaltexth->allTexts[277];
				specialVictory.onFulfill = VLC->generaltexth->allTexts[276];
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
		controller.map()->defeatMessage = VLC->generaltexth->lossCondtions[1]; //TODO: get proper text
		
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
				specialDefeat.onFulfill = VLC->generaltexth->allTexts[251];
				specialDefeat.trigger = EventExpression(noneOf);
				break;
			}
				
			case 2: {
				EventCondition cond(EventCondition::DAYS_PASSED);
				assert(loseValueWidget);
				cond.value = expiredDate(loseValueWidget->text());
				specialDefeat.onFulfill = VLC->generaltexth->allTexts[254];
				specialDefeat.trigger = EventExpression(cond);
				break;
			}
				
			case 3: {
				EventCondition cond(EventCondition::DAYS_WITHOUT_TOWN);
				assert(loseValueWidget);
				cond.value = loseValueWidget->text().toInt();
				specialDefeat.onFulfill = VLC->generaltexth->allTexts[7];
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
	
	close();
}

void MapSettings::on_victoryComboBox_currentIndexChanged(int index)
{
	delete victoryTypeWidget;
	delete victoryValueWidget;
	victoryTypeWidget = nullptr;
	victoryValueWidget = nullptr;
	
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
				victoryTypeWidget->addItem(QString::fromStdString(VLC->arth->objects[i]->getName()), QVariant::fromValue(i));
			break;
		}
			
		case 1: { //EventCondition::HAVE_CREATURES
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i = 0; i < VLC->creh->objects.size(); ++i)
			victoryTypeWidget->addItem(QString::fromStdString(VLC->creh->objects[i]->getName()), QVariant::fromValue(i));
			
			victoryValueWidget = new QLineEdit;
			ui->victoryParamsLayout->addWidget(victoryValueWidget);
			victoryValueWidget->setText("1");
			break;
		}
	}
}


void MapSettings::on_loseComboBox_currentIndexChanged(int index)
{
	delete loseTypeWidget;
	delete loseValueWidget;
	loseTypeWidget = nullptr;
	loseValueWidget = nullptr;
	
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
			for(int i : getTownIndexes())
				loseTypeWidget->addItem(tr(getTownName(i).c_str()), QVariant::fromValue(i));
			break;
		}
			
		case 1: { //EventCondition::CONTROL (Obj::HERO)
			loseTypeWidget = new QComboBox;
			ui->loseParamsLayout->addWidget(loseTypeWidget);
			//for(int i = 0; i < controller.map()->allowedArtifact.size(); ++i)
				//victoryTypeWidget->addItem(QString::fromStdString(VLC->arth->objects[i]->getName()), QVariant::fromValue(i));
			break;
		}
			
		case 2: { //EventCondition::DAYS_PASSED
			loseValueWidget = new QLineEdit;
			ui->loseParamsLayout->addWidget(loseValueWidget);
			
			loseValueWidget->setText("1m 1w 1d");
			break;
		}
			
		case 3: { //EventCondition::DAYS_WITHOUT_TOWN
			loseValueWidget = new QLineEdit;
			ui->loseParamsLayout->addWidget(loseValueWidget);
			
			loseValueWidget->setInputMask("9000");
			loseValueWidget->setText("7");
			break;
		}
	}
}

