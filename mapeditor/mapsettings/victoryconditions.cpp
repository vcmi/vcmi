/*
 * victoryconditions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "victoryconditions.h"
#include "ui_victoryconditions.h"
#include "../mapcontroller.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/entities/ResourceTypeHandler.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/texts/CGeneralTextHandler.h"

#include "../inspector/townbuildingswidget.h" //to convert BuildingID to string

VictoryConditions::VictoryConditions(QWidget *parent) :
	AbstractSettings(parent),
	ui(new Ui::VictoryConditions)
{
	ui->setupUi(this);
}

void VictoryConditions::initialize(MapController & c)
{
	AbstractSettings::initialize(c);

	//victory message
	ui->victoryMessageEdit->setText(QString::fromStdString(controller->map()->victoryMessage.toString()));

	//victory conditions
	const std::array<std::string, 9> conditionStringsWin = {
		QT_TR_NOOP("No special victory"),
		QT_TR_NOOP("Capture artifact"),
		QT_TR_NOOP("Hire creatures"),
		QT_TR_NOOP("Accumulate resources"),
		QT_TR_NOOP("Construct building"),
		QT_TR_NOOP("Capture town"),
		QT_TR_NOOP("Defeat hero"),
		QT_TR_NOOP("Transport artifact"),
		QT_TR_NOOP("Kill monster")
	};

	for(auto & s : conditionStringsWin)
	{
		ui->victoryComboBox->addItem(tr(s.c_str()));
	}
	ui->standardVictoryCheck->setChecked(false);
	ui->onlyForHumansCheck->setChecked(false);

	for(auto & ev : controller->map()->triggeredEvents)
	{
		if(ev.effect.type == EventEffect::VICTORY)
		{
			if(ev.identifier == "standardVictory")
				ui->standardVictoryCheck->setChecked(true);

			if(ev.identifier == "specialVictory")
			{
				auto readjson = ev.trigger.toJson(AbstractSettings::conditionToJson);
				auto linearNodes = linearJsonArray(readjson);

				for(auto & json : linearNodes)
				{
					switch(json["condition"].Integer())
					{
						case EventCondition::HAVE_ARTIFACT: {
							ui->victoryComboBox->setCurrentIndex(1);
							assert(victoryTypeWidget);
							auto artifactId = ArtifactID::decode(json["objectType"].String());
							auto idx = victoryTypeWidget->findData(artifactId);
							victoryTypeWidget->setCurrentIndex(idx);
							break;
						}

						case EventCondition::HAVE_CREATURES: {
							ui->victoryComboBox->setCurrentIndex(2);
							assert(victoryTypeWidget);
							assert(victoryValueWidget);
							auto creatureId = CreatureID::decode(json["objectType"].String());
							auto idx = victoryTypeWidget->findData(creatureId);
							victoryTypeWidget->setCurrentIndex(idx);
							victoryValueWidget->setText(QString::number(json["value"].Integer()));
							break;
						}

						case EventCondition::HAVE_RESOURCES: {
							ui->victoryComboBox->setCurrentIndex(3);
							assert(victoryTypeWidget);
							assert(victoryValueWidget);
							auto resourceId = EGameResID::decode(json["objectType"].String());
							auto idx = victoryTypeWidget->findData(resourceId);
							victoryTypeWidget->setCurrentIndex(idx);
							victoryValueWidget->setText(QString::number(json["value"].Integer()));
							break;
						}

						case EventCondition::HAVE_BUILDING: {
							ui->victoryComboBox->setCurrentIndex(4);
							assert(victoryTypeWidget);
							assert(victorySelectWidget);
							auto buildingId = BuildingID::decode(json["objectType"].String());
							auto idx = victoryTypeWidget->findData(buildingId);
							victoryTypeWidget->setCurrentIndex(idx);
							int townIdx = getObjectByPos<const CGTownInstance>(*controller->map(), posFromJson(json["position"]));
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
							auto mapObject = MapObjectID::decode(json["objectType"].String());
							if(mapObject == Obj::TOWN)
							{
								int townIdx = getObjectByPos<const CGTownInstance>(*controller->map(), posFromJson(json["position"]));
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
							auto objectType = MapObjectID::decode(json["objectType"].String());
							if(objectType == Obj::HERO)
							{
								ui->victoryComboBox->setCurrentIndex(6);
								assert(victoryTypeWidget);
								int heroIdx = getObjectByPos<const CGHeroInstance>(*controller->map(), posFromJson(json["position"]));
								if(heroIdx >= 0)
								{
									auto idx = victoryTypeWidget->findData(heroIdx);
									victoryTypeWidget->setCurrentIndex(idx);
								}
							}
							if(objectType == Obj::MONSTER)
							{
								ui->victoryComboBox->setCurrentIndex(8);
								assert(victoryTypeWidget);
								int monsterIdx = getObjectByPos<const CGCreature>(*controller->map(), posFromJson(json["position"]));
								if(monsterIdx >= 0)
								{
									auto idx = victoryTypeWidget->findData(monsterIdx);
									victoryTypeWidget->setCurrentIndex(idx);
								}
							}
							break;
						}

						case EventCondition::TRANSPORT: {
							ui->victoryComboBox->setCurrentIndex(7);
							assert(victoryTypeWidget);
							assert(victorySelectWidget);
							auto artifactId = ArtifactID::decode(json["objectType"].String());
							auto idx = victoryTypeWidget->findData(int(artifactId));
							victoryTypeWidget->setCurrentIndex(idx);
							int townIdx = getObjectByPos<const CGTownInstance>(*controller->map(), posFromJson(json["position"]));
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
	}
}

void VictoryConditions::update()
{
	//victory messages
	bool customMessage = true;

	//victory conditions
	EventCondition victoryCondition(EventCondition::STANDARD_WIN);

	//Victory condition - defeat all
	TriggeredEvent standardVictory;
	standardVictory.effect.type = EventEffect::VICTORY;
	standardVictory.effect.toOtherMessage.appendTextID("core.genrltxt.5");
	standardVictory.identifier = "standardVictory";
	standardVictory.description.clear(); // TODO: display in quest window
	standardVictory.onFulfill.appendTextID("core.genrltxt.659");
	standardVictory.trigger = EventExpression(victoryCondition);

	//VICTORY
	if(ui->victoryComboBox->currentIndex() == 0)
	{
		controller->map()->triggeredEvents.push_back(standardVictory);
		controller->map()->victoryIconIndex = 11;
		controller->map()->victoryMessage = MetaString::createFromTextID("core.vcdesc.0");
		customMessage = false;
	}
	else
	{
		int vicCondition = ui->victoryComboBox->currentIndex() - 1;

		TriggeredEvent specialVictory;
		specialVictory.effect.type = EventEffect::VICTORY;
		specialVictory.identifier = "specialVictory";
		specialVictory.description.clear(); // TODO: display in quest window

		controller->map()->victoryIconIndex = vicCondition;
		controller->map()->victoryMessage = MetaString::createFromTextID("core.vcdesc." + std::to_string(vicCondition + 1));
		customMessage = false;

		switch(vicCondition)
		{
			case 0: {
				EventCondition cond(EventCondition::HAVE_ARTIFACT);
				assert(victoryTypeWidget);
				cond.objectType = ArtifactID(victoryTypeWidget->currentData().toInt());
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.281");
				specialVictory.onFulfill.appendTextID("core.genrltxt.280");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 1: {
				EventCondition cond(EventCondition::HAVE_CREATURES);
				assert(victoryTypeWidget);
				cond.objectType = CreatureID(victoryTypeWidget->currentData().toInt());
				cond.value = victoryValueWidget->text().toInt();
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.277");
				specialVictory.onFulfill.appendTextID("core.genrltxt.276");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 2: {
				EventCondition cond(EventCondition::HAVE_RESOURCES);
				assert(victoryTypeWidget);
				cond.objectType = GameResID(victoryTypeWidget->currentData().toInt());
				cond.value = victoryValueWidget->text().toInt();
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.279");
				specialVictory.onFulfill.appendTextID("core.genrltxt.278");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 3: {
				EventCondition cond(EventCondition::HAVE_BUILDING);
				assert(victoryTypeWidget);
				cond.objectType = BuildingID(victoryTypeWidget->currentData().toInt());
				int townIdx = victorySelectWidget->currentData().toInt();
				if(townIdx > -1)
					cond.position = controller->map()->objects[townIdx]->pos;
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.283");
				specialVictory.onFulfill.appendTextID("core.genrltxt.282");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 4: {
				EventCondition cond(EventCondition::CONTROL);
				assert(victoryTypeWidget);
				cond.objectType = Obj(Obj::TOWN);
				int townIdx = victoryTypeWidget->currentData().toInt();
				cond.position = controller->map()->objects[townIdx]->pos;
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.250");
				specialVictory.onFulfill.appendTextID("core.genrltxt.249");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 5: {
				EventCondition cond(EventCondition::DESTROY);
				assert(victoryTypeWidget);
				cond.objectType = Obj(Obj::HERO);
				int heroIdx = victoryTypeWidget->currentData().toInt();
				cond.position = controller->map()->objects[heroIdx]->pos;
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.253");
				specialVictory.onFulfill.appendTextID("core.genrltxt.252");
				specialVictory.trigger = EventExpression(cond);
				break;
			}

			case 6: {
				EventCondition cond(EventCondition::TRANSPORT);
				assert(victoryTypeWidget);
				cond.objectType = ArtifactID(victoryTypeWidget->currentData().toInt());
				int townIdx = victorySelectWidget->currentData().toInt();
				if(townIdx > -1)
					cond.position = controller->map()->objects[townIdx]->pos;
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.293");
				specialVictory.onFulfill.appendTextID("core.genrltxt.292");
				specialVictory.trigger = EventExpression(cond);
				break;
			}
				
			case 7: {
				EventCondition cond(EventCondition::DESTROY);
				assert(victoryTypeWidget);
				cond.objectType = Obj(Obj::MONSTER);
				int monsterIdx = victoryTypeWidget->currentData().toInt();
				cond.position = controller->map()->objects[monsterIdx]->pos;
				specialVictory.effect.toOtherMessage.appendTextID("core.genrltxt.287");
				specialVictory.onFulfill.appendTextID("core.genrltxt.286");
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
			controller->map()->victoryMessage.appendRawString(" / ");
			controller->map()->victoryMessage.appendTextID("core.vcdesc.0");
			controller->map()->triggeredEvents.push_back(standardVictory);
			customMessage = false;
		}
		controller->map()->triggeredEvents.push_back(specialVictory);
	}
	
	if(customMessage)
	{
		controller->map()->victoryMessage = MetaString::createFromTextID(mapRegisterLocalizedString("map", *controller->map(), TextIdentifier("header", "victoryMessage"), ui->victoryMessageEdit->text().toStdString()));
	}
}

VictoryConditions::~VictoryConditions()
{
	delete ui;
}

void VictoryConditions::on_victoryComboBox_currentIndexChanged(int index)
{
	delete victoryTypeWidget;
	delete victoryValueWidget;
	delete victorySelectWidget;
	delete pickObjectButton;
	victoryTypeWidget = nullptr;
	victoryValueWidget = nullptr;
	victorySelectWidget = nullptr;
	pickObjectButton = nullptr;

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
			for(int i = 0; i < controller->map()->allowedArtifact.size(); ++i)
				victoryTypeWidget->addItem(QString::fromStdString(LIBRARY->arth->objects[i]->getNameTranslated()), QVariant::fromValue(i));
			break;
		}

		case 1: { //EventCondition::HAVE_CREATURES
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i = 0; i < LIBRARY->creh->objects.size(); ++i)
				victoryTypeWidget->addItem(QString::fromStdString(LIBRARY->creh->objects[i]->getNamePluralTranslated()), QVariant::fromValue(i));

			victoryValueWidget = new QLineEdit;
			ui->victoryParamsLayout->addWidget(victoryValueWidget);
			victoryValueWidget->setText("1");
			break;
		}

		case 2: { //EventCondition::HAVE_RESOURCES
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			{
				for(auto & resType : LIBRARY->resourceTypeHandler->getAllObjects())
				{
					MetaString str;
					str.appendName(GameResID(resType));
					auto resName = QString::fromStdString(str.toString());
					victoryTypeWidget->addItem(resName, QVariant::fromValue(resType.getNum()));
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
			auto * ctown = LIBRARY->townh->randomFaction->town.get();
			for(int bId : ctown->getAllBuildings())
				victoryTypeWidget->addItem(QString::fromStdString(defaultBuildingIdConversion(BuildingID(bId))), QVariant::fromValue(bId));

			victorySelectWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victorySelectWidget);
			victorySelectWidget->addItem(tr("Any town"), QVariant::fromValue(-1));
			for(int i : getObjectIndexes<const CGTownInstance>(*controller->map()))
				victorySelectWidget->addItem(getTownName(*controller->map(), i).c_str(), QVariant::fromValue(i));
			
			pickObjectButton = new QToolButton;
			connect(pickObjectButton, &QToolButton::clicked, this, &VictoryConditions::onObjectSelect);
			ui->victoryParamsLayout->addWidget(pickObjectButton);
			break;
		}

		case 4: { //EventCondition::CONTROL (Obj::TOWN)
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i : getObjectIndexes<const CGTownInstance>(*controller->map()))
				victoryTypeWidget->addItem(tr(getTownName(*controller->map(), i).c_str()), QVariant::fromValue(i));
			pickObjectButton = new QToolButton;
			connect(pickObjectButton, &QToolButton::clicked, this, &VictoryConditions::onObjectSelect);
			ui->victoryParamsLayout->addWidget(pickObjectButton);
			break;
		}

		case 5: { //EventCondition::DESTROY (Obj::HERO)
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i : getObjectIndexes<const CGHeroInstance>(*controller->map()))
				victoryTypeWidget->addItem(tr(getHeroName(*controller->map(), i).c_str()), QVariant::fromValue(i));
			pickObjectButton = new QToolButton;
			connect(pickObjectButton, &QToolButton::clicked, this, &VictoryConditions::onObjectSelect);
			ui->victoryParamsLayout->addWidget(pickObjectButton);
			break;
		}

		case 6: { //EventCondition::TRANSPORT (Obj::ARTEFACT)
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i = 0; i < controller->map()->allowedArtifact.size(); ++i)
				victoryTypeWidget->addItem(QString::fromStdString(LIBRARY->arth->objects[i]->getNameTranslated()), QVariant::fromValue(i));
			
			victorySelectWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victorySelectWidget);
			for(int i : getObjectIndexes<const CGTownInstance>(*controller->map()))
				victorySelectWidget->addItem(tr(getTownName(*controller->map(), i).c_str()), QVariant::fromValue(i));
			pickObjectButton = new QToolButton;
			connect(pickObjectButton, &QToolButton::clicked, this, &VictoryConditions::onObjectSelect);
			ui->victoryParamsLayout->addWidget(pickObjectButton);
			break;
		}
			
		case 7: { //EventCondition::DESTROY (Obj::MONSTER)
			victoryTypeWidget = new QComboBox;
			ui->victoryParamsLayout->addWidget(victoryTypeWidget);
			for(int i : getObjectIndexes<const CGCreature>(*controller->map()))
				victoryTypeWidget->addItem(getMonsterName(*controller->map(), i).c_str(), QVariant::fromValue(i));
			pickObjectButton = new QToolButton;
			connect(pickObjectButton, &QToolButton::clicked, this, &VictoryConditions::onObjectSelect);
			ui->victoryParamsLayout->addWidget(pickObjectButton);
			break;
		}
	}
}


void VictoryConditions::onObjectSelect()
{
	int vicConditions = ui->victoryComboBox->currentIndex() - 1;

	for(MapScene * level : controller->getScenes())
	{
		auto & l = level->objectPickerView;
		switch(vicConditions)
		{
			case 3: { //EventCondition::HAVE_BUILDING
				l.highlight<const CGTownInstance>();
				break;
			}
				
			case 4: { //EventCondition::CONTROL (Obj::TOWN)
				l.highlight<const CGTownInstance>();
				break;
			}
				
			case 5: { //EventCondition::DESTROY (Obj::HERO)
				l.highlight<const CGHeroInstance>();
				break;
			}
				
			case 6: { //EventCondition::TRANSPORT (Obj::ARTEFACT)
				l.highlight<const CGTownInstance>();
				break;
			}
				
			case 7: { //EventCondition::DESTROY (Obj::MONSTER)
				l.highlight<const CGCreature>();
				break;
			}
			default:
				return;
		}
		l.update();
		QObject::connect(&l, &ObjectPickerLayer::selectionMade, this, &VictoryConditions::onObjectPicked);
	}
	
	controller->settingsDialog->hide();
}

void VictoryConditions::onObjectPicked(const CGObjectInstance * obj)
{
	controller->settingsDialog->show();
	
	for(MapScene * level : controller->getScenes())
	{
		auto & l = level->objectPickerView;
		l.clear();
		l.update();
		QObject::disconnect(&l, &ObjectPickerLayer::selectionMade, this, &VictoryConditions::onObjectPicked);
	}
	
	if(!obj) //discarded
		return;
	
	int vicConditions = ui->victoryComboBox->currentIndex() - 1;
	QComboBox * w = victoryTypeWidget;
	if(vicConditions == 3 || vicConditions == 6)
		w = victorySelectWidget;
	
	for(int i = 0; i < w->count(); ++i)
	{
		if(w->itemData(i).toInt() < 0)
			continue;
		
		auto data = controller->map()->objects.at(w->itemData(i).toInt());
		if(data.get() == obj)
		{
			w->setCurrentIndex(i);
			break;
		}
	}
}
