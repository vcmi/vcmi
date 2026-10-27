/*
 * questwidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "questwidget.h"
#include "StdInc.h"
#include "ui_questwidget.h"
#include "../../translator.h"
#include "../../mapview.h"
#include "../../mapcontroller.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/CSkillHandler.h"
#include "../../../lib/CBonusTypeHandler.h"
#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/entities/artifact/CArtifact.h"
#include "../../../lib/entities/ResourceTypeHandler.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/spells/CSpellHandler.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/modding/IdentifierStorage.h"
#include "../../../lib/modding/ModScope.h"
#include "../../../lib/rewardable/Configuration.h"
#include "../../../lib/mapObjects/Quest.h"
#include "../../../lib/mapObjects/CGCreature.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"

#include <vcmi/ArtifactService.h>
#include <vcmi/HeroTypeService.h>
#include <vcmi/HeroType.h>
#include <vcmi/HeroClassService.h>
#include <vcmi/HeroClass.h>
#include <vcmi/spells/Service.h>
#include <vcmi/spells/Spell.h>

QuestWidget::QuestWidget(MapController & controller, QWidget *parent) :
	QWidget(parent),
	controller(controller),
	ui(new Ui::QuestWidget)
{
	setAttribute(Qt::WA_DeleteOnClose, true);
	ui->setupUi(this);
	ui->dayOfWeek->addItem(tr("None"));
	for(int i = 1; i <= 7; ++i)
		ui->dayOfWeek->addItem(tr("Day %1").arg(i));

	//fill resources
	ui->resources->setRowCount(LIBRARY->resourceTypeHandler->getAllObjects().size());
	for(auto & i : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		MetaString str;
		str.appendName(GameResID(i));
		auto * item = new QTableWidgetItem(QString::fromStdString(str.toString(&Translator::instance())));
		item->setData(Qt::UserRole, QVariant::fromValue(i.getNum()));
		ui->resources->setItem(i, 0, item);
		auto * spinBox = new QSpinBox;
		spinBox->setMaximum(i == GameResID::GOLD ? 999999 : 999);
		ui->resources->setCellWidget(i, 1, spinBox);
	}

	//fill artifacts
	for(const auto & artifactPtr : LIBRARY->arth->objects)
	{
		auto artifactIndex = artifactPtr->getIndex();
		auto * item = new QListWidgetItem(QString::fromStdString(artifactPtr->getNameTranslated()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		if(controller.map()->allowedArtifact.count(artifactIndex) == 0)
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		ui->artifacts->addItem(item);
	}

	//fill spells
	for(const auto & spellPtr : LIBRARY->spellh->objects)
	{
		auto spellIndex = spellPtr->getIndex();
		auto * item = new QListWidgetItem(QString::fromStdString(spellPtr->getNameTranslated()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		if(controller.map()->allowedSpells.count(spellIndex) == 0)
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		ui->spells->addItem(item);
	}

	//fill skills
	ui->skills->setRowCount(LIBRARY->skillh->objects.size());
	for(const auto & skillPtr : LIBRARY->skillh->objects)
	{
		auto skillIndex = skillPtr->getIndex();
		auto * item = new QTableWidgetItem(QString::fromStdString(skillPtr->getNameTranslated()));

		auto * widget = new QComboBox;
		for(const auto & s : NSecondarySkill::levels)
			widget->addItem(QString::fromStdString(s));

		if(controller.map()->allowedAbilities.count(skillIndex) == 0)
		{
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
			widget->setEnabled(false);
		}

		ui->skills->setItem(skillIndex, 0, item);
		ui->skills->setCellWidget(skillIndex, 1, widget);
	}

	//fill creatures
	for(auto & creature : LIBRARY->creh->objects)
	{
		ui->creatureId->addItem(QString::fromStdString(creature->getNameSingularTranslated()));
		ui->creatureId->setItemData(ui->creatureId->count() - 1, creature->getIndex());
	}

	//fill heroes
	LIBRARY->heroTypes()->forEach([this](const HeroType * hero, bool &)
								  {
									  auto * item = new QListWidgetItem(QString::fromStdString(hero->getNameTranslated()));
									  item->setData(Qt::UserRole, QVariant::fromValue(hero->getId().getNum()));
									  item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
									  item->setCheckState(Qt::Unchecked);
									  ui->heroes->addItem(item);
								  });

	//fill hero classes
	LIBRARY->heroClasses()->forEach([this](const HeroClass * heroClass, bool &)
									{
										auto * item = new QListWidgetItem(QString::fromStdString(heroClass->getNameTranslated()));
										item->setData(Qt::UserRole, QVariant::fromValue(heroClass->getId().getNum()));
										item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
										item->setCheckState(Qt::Unchecked);
										ui->heroClasses->addItem(item);
									});

	//fill players
	for(auto color = PlayerColor(0); color < PlayerColor::PLAYER_LIMIT; ++color)
	{
		MetaString str;
		str.appendName(color);
		auto * item = new QListWidgetItem(QString::fromStdString(str.toString(&Translator::instance())));
		item->setData(Qt::UserRole, QVariant::fromValue(color.getNum()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->players->addItem(item);
	}

	ui->targetContainer->hide();
}

QuestWidget::~QuestWidget()
{
	delete ui;
}

void QuestWidget::loadQuest(Quest * quest)
{
	this->quest = quest;
	showData(this->quest);
}

void QuestWidget::showData(Quest * quest)
{
	if(!quest)
		return;

	ui->dayOfWeek->setCurrentIndex(quest->mission.dayOfWeek);
	ui->daysPassed->setValue(quest->mission.daysPassed);
	ui->heroLevel->setValue(quest->mission.heroLevel);
	ui->heroExperience->setValue(quest->mission.heroExperience);
	ui->manaPoints->setValue(quest->mission.manaPoints);
	ui->manaPercentage->setValue(quest->mission.manaPercentage);
	ui->attack->setValue(quest->mission.primary[0]);
	ui->defense->setValue(quest->mission.primary[1]);
	ui->power->setValue(quest->mission.primary[2]);
	ui->knowledge->setValue(quest->mission.primary[3]);

	auto uncheckListWidget = [](QListWidget * listWidget) {
		for (int i = 0; i < listWidget->count(); ++i)
			listWidget->item(i)->setCheckState(Qt::Unchecked);
	};
	uncheckListWidget(ui->artifacts);
	uncheckListWidget(ui->spells);
	for (int i = 0; i < ui->skills->rowCount(); ++i)
		qobject_cast<QComboBox*>(ui->skills->cellWidget(i, 1))->setCurrentIndex(0);
	ui->creatures->setRowCount(0);
	uncheckListWidget(ui->heroes);
	uncheckListWidget(ui->heroClasses);
	uncheckListWidget(ui->players);

	for(int i = 0; i < ui->resources->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QSpinBox*>(ui->resources->cellWidget(i, 1)))
			widget->setValue(quest->mission.resources[i]);
	}
	for(auto i : quest->mission.artifacts)
		ui->artifacts->item(LIBRARY->artifacts()->getById(i)->getIndex())->setCheckState(Qt::Checked);
	for(auto i : quest->mission.spells)
		ui->spells->item(LIBRARY->spells()->getById(i)->getIndex())->setCheckState(Qt::Checked);
	for(auto & i : quest->mission.secondary)
	{
		int index = LIBRARY->skills()->getById(i.first)->getIndex();
		if(auto * widget = qobject_cast<QComboBox*>(ui->skills->cellWidget(index, 1)))
			widget->setCurrentIndex(i.second);
	}
	for(auto & i : quest->mission.creatures)
	{
		int index = i.getType()->getIndex();
		ui->creatureId->setCurrentIndex(index);
		ui->creatureAmount->setValue(i.getCount());
		onCreatureAdd(ui->creatures, ui->creatureId, ui->creatureAmount);
	}
	for(auto & i : quest->mission.heroes)
	{
		for(int e = 0; e < ui->heroes->count(); ++e)
		{
			if(ui->heroes->item(e)->data(Qt::UserRole).toInt() == i.getNum())
			{
				ui->heroes->item(e)->setCheckState(Qt::Checked);
				break;
			}
		}
	}
	for(auto & i : quest->mission.heroClasses)
	{
		for(int e = 0; e < ui->heroClasses->count(); ++e)
		{
			if(ui->heroClasses->item(e)->data(Qt::UserRole).toInt() == i.getNum())
			{
				ui->heroClasses->item(e)->setCheckState(Qt::Checked);
				break;
			}
		}
	}
	for(auto & i : quest->mission.players)
	{
		for(int e = 0; e < ui->players->count(); ++e)
		{
			if(ui->players->item(e)->data(Qt::UserRole).toInt() == i.getNum())
			{
				ui->players->item(e)->setCheckState(Qt::Checked);
				break;
			}
		}
	}

	ObjectInstanceID killTarget = quest->mission.destroyedObjects.empty() ? ObjectInstanceID::NONE : quest->mission.destroyedObjects.front();
	if(killTarget != ObjectInstanceID::NONE && killTarget < controller.map()->objects.size())
		ui->killTarget->setText(QString::fromStdString(controller.map()->objects[killTarget]->instanceName));
	else
	{
		quest->mission.destroyedObjects.clear();
		ui->killTarget->clear();
	}

	ui->repeatableCheckbox->setChecked(quest->repeatedQuest);
	ui->deadlineCheckbox->setChecked(quest->lastDay >= 0);
	ui->deadlineSpinBox->setDisabled(quest->lastDay < 0);
	ui->deadlineSpinBox->setValue(quest->lastDay);

	printQuestInformation();
}

void QuestWidget::commit()
{
	if (!quest)
		return;

	quest->mission.dayOfWeek = ui->dayOfWeek->currentIndex();
	quest->mission.daysPassed = ui->daysPassed->value();
	quest->mission.heroLevel = ui->heroLevel->value();
	quest->mission.heroExperience = ui->heroExperience->value();
	quest->mission.manaPoints = ui->manaPoints->value();
	quest->mission.manaPercentage = ui->manaPercentage->value();
	quest->mission.primary[0] = ui->attack->value();
	quest->mission.primary[1] = ui->defense->value();
	quest->mission.primary[2] = ui->power->value();
	quest->mission.primary[3] = ui->knowledge->value();
	for(int i = 0; i < ui->resources->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QSpinBox*>(ui->resources->cellWidget(i, 1)))
			quest->mission.resources[i] = widget->value();
	}

	quest->mission.artifacts.clear();
	for(int i = 0; i < ui->artifacts->count(); ++i)
	{
		if(ui->artifacts->item(i)->checkState() == Qt::Checked)
			quest->mission.artifacts.push_back(LIBRARY->artifacts()->getByIndex(i)->getId());
	}
	quest->mission.spells.clear();
	for(int i = 0; i < ui->spells->count(); ++i)
	{
		if(ui->spells->item(i)->checkState() == Qt::Checked)
			quest->mission.spells.push_back(LIBRARY->spells()->getByIndex(i)->getId());
	}

	quest->mission.secondary.clear();
	for(int i = 0; i < ui->skills->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QComboBox*>(ui->skills->cellWidget(i, 1)))
		{
			if(widget->currentIndex() > 0)
				quest->mission.secondary[LIBRARY->skills()->getByIndex(i)->getId()] = widget->currentIndex();
		}
	}

	quest->mission.creatures.clear();
	for(int i = 0; i < ui->creatures->rowCount(); ++i)
	{
		int index = ui->creatures->item(i, 0)->data(Qt::UserRole).toInt();
		if(auto * widget = qobject_cast<QSpinBox*>(ui->creatures->cellWidget(i, 1)))
			if(widget->value())
				quest->mission.creatures.emplace_back(LIBRARY->creatures()->getByIndex(index)->getId(), widget->value());
	}

	quest->mission.heroes.clear();
	for(int i = 0; i < ui->heroes->count(); ++i)
	{
		if(ui->heroes->item(i)->checkState() == Qt::Checked)
			quest->mission.heroes.emplace_back(ui->heroes->item(i)->data(Qt::UserRole).toInt());
	}

	quest->mission.heroClasses.clear();
	for(int i = 0; i < ui->heroClasses->count(); ++i)
	{
		if(ui->heroClasses->item(i)->checkState() == Qt::Checked)
			quest->mission.heroClasses.emplace_back(ui->heroClasses->item(i)->data(Qt::UserRole).toInt());
	}

	quest->mission.players.clear();
	for(int i = 0; i < ui->players->count(); ++i)
	{
		if(ui->players->item(i)->checkState() == Qt::Checked)
			quest->mission.players.emplace_back(ui->players->item(i)->data(Qt::UserRole).toInt());
	}

	if(canChangeQuestRepeatability)
		quest->repeatedQuest = ui->repeatableCheckbox->isChecked();
	quest->lastDay = ui->deadlineCheckbox->isChecked() ? ui->deadlineSpinBox->value() : -1;

	//quest->mission.destroyedObjects is set directly in object picking

}

void QuestWidget::clearData()
{
	quest = nullptr;
	auto emptyQuest = Quest();
	showData(&emptyQuest);
}

bool QuestWidget::isQuestLoaded() const
{
	return quest;
}

void QuestWidget::showMapAction(std::optional<std::function<void()>>showMap)
{
	this->showMap = showMap;
	if (targetPickerIsAllowed())
		ui->targetContainer->show();
	else
		ui->targetContainer->hide();
}

void QuestWidget::returnToWidgetAction(std::optional<std::function<void()>> returnToWidget)
{
	this->returnToWidget = returnToWidget;
	if (targetPickerIsAllowed())
		ui->targetContainer->show();
	else
		ui->targetContainer->hide();
}

void QuestWidget::canMakeQuestRepeat(bool allow)
{
	canChangeQuestRepeatability = allow;
	ui->repeatableCheckbox->setDisabled(!allow);
}

void QuestWidget::on_killTargetSelect_clicked()
{
	auto pred = [](const CGObjectInstance * obj) -> bool
	{
		if(auto * o = dynamic_cast<const CGHeroInstance*>(obj))
			return o->ID != Obj::PRISON;
		if(dynamic_cast<const CGCreature*>(obj))
			return true;
		return false;
	};

	for(MapScene * level : controller.getScenes())
	{
		auto & l = level->objectPickerView;
		l.highlight(pred);
		l.update();
		QObject::connect(&l, &ObjectPickerLayer::selectionMade, this, &QuestWidget::onTargetPicked);
	}
	showMap.value()();
}

void QuestWidget::onTargetPicked(const CGObjectInstance * obj)
{
	returnToWidget.value()();
	for(MapScene * level : controller.getScenes())
	{
		auto & l = level->objectPickerView;
		l.clear();
		l.update();
		QObject::disconnect(&l, &ObjectPickerLayer::selectionMade, this, &QuestWidget::onTargetPicked);
	}

	if(!obj) //discarded
	{
		quest->mission.destroyedObjects.clear();
		ui->killTarget->setText("");
		return;
	}

	ui->killTarget->setText(QString::fromStdString(obj->instanceName));
	quest->mission.destroyedObjects = { obj->id };
}

void QuestWidget::on_creatureAdd_clicked()
{
	onCreatureAdd(ui->creatures, ui->creatureId, ui->creatureAmount);
}


void QuestWidget::on_creatureRemove_clicked()
{
	std::set<int, std::greater<int>> rowsToRemove;
	for(auto * i : ui->creatures->selectedItems())
		rowsToRemove.insert(i->row());

	for(auto i : rowsToRemove)
		ui->creatures->removeRow(i);
}

void QuestWidget::on_deadlineCheckbox_stateChanged(int state)
{
	ui->deadlineSpinBox->setDisabled(!state);
}

void QuestWidget::on_repeatableCheckbox_clicked()
{
	repeatableClicked();
}

void QuestWidget::onCreatureAdd(QTableWidget * listWidget, QComboBox * comboWidget, QSpinBox * spinWidget)
{
	QTableWidgetItem * item = nullptr;
	QSpinBox * widget = nullptr;
	for(int i = 0; i < listWidget->rowCount(); ++i)
	{
		if(auto * cname = listWidget->item(i, 0))
		{
			if(cname->data(Qt::UserRole).toInt() == comboWidget->currentData().toInt())
			{
				item = cname;
				widget = qobject_cast<QSpinBox*>(listWidget->cellWidget(i, 1));
				break;
			}
		}
	}

	if(!item)
	{
		listWidget->setRowCount(listWidget->rowCount() + 1);
		item = new QTableWidgetItem(comboWidget->currentText());
		listWidget->setItem(listWidget->rowCount() - 1, 0, item);
	}

	item->setData(Qt::UserRole, comboWidget->currentData());

	if(!widget)
	{
		widget = new QSpinBox;
		widget->setRange(spinWidget->minimum(), spinWidget->maximum());
		listWidget->setCellWidget(listWidget->rowCount() - 1, 1, widget);
	}

	widget->setValue(spinWidget->value());
}

void QuestWidget::printQuestInformation()
{
	QStringList textList;

	if (!quest)
	{
		ui->questInfo->setText(QString());
		return;
	}

	if (quest->mission.dayOfWeek)
		textList += QObject::tr("Day of Week: %1").arg(quest->mission.dayOfWeek);
	if (quest->mission.daysPassed)
		textList += QObject::tr("Days Passed: %1").arg(quest->mission.daysPassed);
	if (quest->mission.heroLevel > 0)
		textList += QObject::tr("Hero Level: %1").arg(quest->mission.heroLevel);
	if (quest->mission.heroExperience)
		textList += QObject::tr("Hero Experience: %1").arg(quest->mission.heroExperience);
	if (quest->mission.manaPoints)
		textList += QObject::tr("Mana Points: %1").arg(quest->mission.manaPoints);
	if (quest->mission.manaPercentage > 0)
		textList += QObject::tr("Mana Percentage: %1").arg(quest->mission.manaPercentage);
	if (quest->mission.primary[0] || quest->mission.primary[1] || quest->mission.primary[2] || quest->mission.primary[3])
		textList += QObject::tr("Primary Skills: %1/%2/%3/%4").arg(quest->mission.primary[0]).arg(quest->mission.primary[1]).arg(quest->mission.primary[2]).arg(quest->mission.primary[3]);
	QStringList resourcesList;
	for(GameResID resource = GameResID::WOOD; resource < GameResID::COUNT ; resource++)
	{
		if(quest->mission.resources[resource] == 0)
			continue;
		MetaString str;
		str.appendName(resource);
		resourcesList += QString("%1: %2").arg(QString::fromStdString(str.toString(&Translator::instance()))).arg(quest->mission.resources[resource]);
	}
	if (!resourcesList.empty())
		textList += QObject::tr("Resources: %1").arg(resourcesList.join(", "));

	if (!quest->mission.artifacts.empty())
	{
		QStringList artifactsList;
		for(const auto & artifact : quest->mission.artifacts)
		{
			artifactsList += QString::fromStdString(LIBRARY->artifacts()->getById(artifact)->getNameTranslated());
		}
		textList += QObject::tr("Artifacts: %1").arg(artifactsList.join(", "));
	}

	if (!quest->mission.spells.empty())
	{
		QStringList spellsList;
		for(const auto & spell : quest->mission.spells)
		{
			spellsList += QString::fromStdString(LIBRARY->spells()->getById(spell)->getNameTranslated());
		}
		textList += QObject::tr("Spells: %1").arg(spellsList.join(", "));
	}

	if (!quest->mission.secondary.empty())
	{
		QStringList secondarySkillsList;
		for(const auto & [skill, skillLevel] : quest->mission.secondary)
		{
			secondarySkillsList += QString("%1 (%2)").arg(QString::fromStdString(LIBRARY->skills()->getById(skill)->getNameTranslated())).arg(skillLevel);
		}
		textList += QObject::tr("Secondary Skills: %1").arg(secondarySkillsList.join(", "));
	}

	if (!quest->mission.creatures.empty())
	{
		QStringList creaturesList;
		for(const auto & creature : quest->mission.creatures)
		{
			creaturesList += QString("%1 %2").arg(creature.getCount()).arg(QString::fromStdString(creature.getType()->getNamePluralTranslated()));
		}
		textList += QObject::tr("Creatures: %1").arg(creaturesList.join(", "));
	}

	if (!quest->mission.heroes.empty())
	{
		QStringList heroesList;
		for(const auto & hero : quest->mission.heroes)
		{
			heroesList += QString::fromStdString(LIBRARY->heroTypes()->getById(hero)->getNameTranslated());
		}
		textList += QObject::tr("Heroes: %1").arg(heroesList.join(", "));
	}

	if (!quest->mission.heroClasses.empty())
	{
		QStringList heroClassesList;
		for(const auto & heroClass : quest->mission.heroClasses)
		{
			heroClassesList += QString::fromStdString(LIBRARY->heroClasses()->getById(heroClass)->getNameTranslated());
		}
		textList += QObject::tr("Hero Classes: %1").arg(heroClassesList.join(", "));
	}

	if (!quest->mission.players.empty())
	{
		QStringList playersList;
		for(const auto & player : quest->mission.players)
		{
			MetaString str;
			str.appendName(player);
			playersList += QString::fromStdString(str.toString(&Translator::instance()));
		}
		textList += QObject::tr("Players: %1").arg(playersList.join(", "));
	}

	ObjectInstanceID killTarget = quest->mission.destroyedObjects.empty() ? ObjectInstanceID::NONE : quest->mission.destroyedObjects.front();
	if(killTarget != ObjectInstanceID::NONE && killTarget < controller.map()->objects.size())
		textList += QObject::tr("Kill enemy: %1").arg(QString::fromStdString(controller.map()->objects[killTarget]->instanceName));;

	if (!textList.isEmpty())
		ui->questInfo->setText(textList.join("\n"));
	else
		ui->questInfo->setText(QString(QObject::tr("Empty Quest")));
}

// parent widget needs to take care of the hide/show functions being implemented for unit picker to work
bool QuestWidget::targetPickerIsAllowed()
{
	return showMap && returnToWidget;
}
