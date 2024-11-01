/*
 * questwidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "questwidget.h"
#include "ui_questwidget.h"
#include "../mapcontroller.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/CSkillHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/constants/StringConstants.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CGCreature.h"

#include <vcmi/HeroTypeService.h>
#include <vcmi/HeroType.h>
#include <vcmi/HeroClassService.h>
#include <vcmi/HeroClass.h>

QuestWidget::QuestWidget(MapController & _controller, CQuest & _sh, QWidget *parent) :
	QDialog(parent),
	controller(_controller),
	quest(_sh),
	ui(new Ui::QuestWidget)
{
	setAttribute(Qt::WA_DeleteOnClose, true);
	ui->setupUi(this);

	ui->lDayOfWeek->addItem(tr("None"));
	for(int i = 1; i <= 7; ++i)
		ui->lDayOfWeek->addItem(tr("Day %1").arg(i));
	
	//fill resources
	ui->lResources->setRowCount(GameConstants::RESOURCE_QUANTITY - 1);
	for(int i = 0; i < GameConstants::RESOURCE_QUANTITY - 1; ++i)
	{
		auto * item = new QTableWidgetItem(QString::fromStdString(GameConstants::RESOURCE_NAMES[i]));
		item->setData(Qt::UserRole, QVariant::fromValue(i));
		ui->lResources->setItem(i, 0, item);
		auto * spinBox = new QSpinBox;
		spinBox->setMaximum(i == GameResID::GOLD ? 999999 : 999);
		ui->lResources->setCellWidget(i, 1, spinBox);
	}
	
	//fill artifacts
	for(const auto & artifactPtr : VLC->arth->objects)
	{
		auto artifactIndex = artifactPtr->getIndex();
		auto * item = new QListWidgetItem(QString::fromStdString(artifactPtr->getNameTranslated()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		if(controller.map()->allowedArtifact.count(artifactIndex) == 0)
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		ui->lArtifacts->addItem(item);
	}
	
	//fill spells
	for(const auto & spellPtr : VLC->spellh->objects)
	{
		auto spellIndex = spellPtr->getIndex();
		auto * item = new QListWidgetItem(QString::fromStdString(spellPtr->getNameTranslated()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		if(controller.map()->allowedSpells.count(spellIndex) == 0)
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		ui->lSpells->addItem(item);
	}
	
	//fill skills
	ui->lSkills->setRowCount(VLC->skillh->objects.size());
	for(const auto & skillPtr : VLC->skillh->objects)
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
			
		ui->lSkills->setItem(skillIndex, 0, item);
		ui->lSkills->setCellWidget(skillIndex, 1, widget);
	}
	
	//fill creatures
	for(auto & creature : VLC->creh->objects)
	{
		ui->lCreatureId->addItem(QString::fromStdString(creature->getNameSingularTranslated()));
		ui->lCreatureId->setItemData(ui->lCreatureId->count() - 1, creature->getIndex());
	}
	
	//fill heroes
	VLC->heroTypes()->forEach([this](const HeroType * hero, bool &)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(hero->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(hero->getId().getNum()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->lHeroes->addItem(item);
	});
	
	//fill hero classes
	VLC->heroClasses()->forEach([this](const HeroClass * heroClass, bool &)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(heroClass->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(heroClass->getId().getNum()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->lHeroClasses->addItem(item);
	});
	
	//fill players
	for(auto color = PlayerColor(0); color < PlayerColor::PLAYER_LIMIT; ++color)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(GameConstants::PLAYER_COLOR_NAMES[color.getNum()]));
		item->setData(Qt::UserRole, QVariant::fromValue(color.getNum()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->lPlayers->addItem(item);
	}
}

QuestWidget::~QuestWidget()
{
	delete ui;
}

void QuestWidget::obtainData()
{
	ui->lDayOfWeek->setCurrentIndex(quest.mission.dayOfWeek);
	ui->lDaysPassed->setValue(quest.mission.daysPassed);
	ui->lHeroLevel->setValue(quest.mission.heroLevel);
	ui->lHeroExperience->setValue(quest.mission.heroExperience);
	ui->lManaPoints->setValue(quest.mission.manaPoints);
	ui->lManaPercentage->setValue(quest.mission.manaPercentage);
	ui->lAttack->setValue(quest.mission.primary[0]);
	ui->lDefence->setValue(quest.mission.primary[1]);
	ui->lPower->setValue(quest.mission.primary[2]);
	ui->lKnowledge->setValue(quest.mission.primary[3]);
	for(int i = 0; i < ui->lResources->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QSpinBox*>(ui->lResources->cellWidget(i, 1)))
			widget->setValue(quest.mission.resources[i]);
	}
	
	for(auto i : quest.mission.artifacts)
		ui->lArtifacts->item(VLC->artifacts()->getById(i)->getIndex())->setCheckState(Qt::Checked);
	for(auto i : quest.mission.spells)
		ui->lSpells->item(VLC->spells()->getById(i)->getIndex())->setCheckState(Qt::Checked);
	for(auto & i : quest.mission.secondary)
	{
		int index = VLC->skills()->getById(i.first)->getIndex();
		if(auto * widget = qobject_cast<QComboBox*>(ui->lSkills->cellWidget(index, 1)))
			widget->setCurrentIndex(i.second);
	}
	for(auto & i : quest.mission.creatures)
	{
		int index = i.type->getIndex();
		ui->lCreatureId->setCurrentIndex(index);
		ui->lCreatureAmount->setValue(i.count);
		onCreatureAdd(ui->lCreatures, ui->lCreatureId, ui->lCreatureAmount);
	}
	for(auto & i : quest.mission.heroes)
	{
		for(int e = 0; e < ui->lHeroes->count(); ++e)
		{
			if(ui->lHeroes->item(e)->data(Qt::UserRole).toInt() == i.getNum())
			{
				ui->lHeroes->item(e)->setCheckState(Qt::Checked);
				break;
			}
		}
	}
	for(auto & i : quest.mission.heroClasses)
	{
		for(int e = 0; e < ui->lHeroClasses->count(); ++e)
		{
			if(ui->lHeroClasses->item(e)->data(Qt::UserRole).toInt() == i.getNum())
			{
				ui->lHeroClasses->item(e)->setCheckState(Qt::Checked);
				break;
			}
		}
	}
	for(auto & i : quest.mission.players)
	{
		for(int e = 0; e < ui->lPlayers->count(); ++e)
		{
			if(ui->lPlayers->item(e)->data(Qt::UserRole).toInt() == i.getNum())
			{
				ui->lPlayers->item(e)->setCheckState(Qt::Checked);
				break;
			}
		}
	}
	
	if(quest.killTarget != ObjectInstanceID::NONE && quest.killTarget < controller.map()->objects.size())
		ui->lKillTarget->setText(QString::fromStdString(controller.map()->objects[quest.killTarget]->instanceName));
	else
		quest.killTarget = ObjectInstanceID::NONE;
}

bool QuestWidget::commitChanges()
{
	quest.mission.dayOfWeek = ui->lDayOfWeek->currentIndex();
	quest.mission.daysPassed = ui->lDaysPassed->value();
	quest.mission.heroLevel = ui->lHeroLevel->value();
	quest.mission.heroExperience = ui->lHeroExperience->value();
	quest.mission.manaPoints = ui->lManaPoints->value();
	quest.mission.manaPercentage = ui->lManaPercentage->value();
	quest.mission.primary[0] = ui->lAttack->value();
	quest.mission.primary[1] = ui->lDefence->value();
	quest.mission.primary[2] = ui->lPower->value();
	quest.mission.primary[3] = ui->lKnowledge->value();
	for(int i = 0; i < ui->lResources->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QSpinBox*>(ui->lResources->cellWidget(i, 1)))
			quest.mission.resources[i] = widget->value();
	}
	
	quest.mission.artifacts.clear();
	for(int i = 0; i < ui->lArtifacts->count(); ++i)
	{
		if(ui->lArtifacts->item(i)->checkState() == Qt::Checked)
			quest.mission.artifacts.push_back(VLC->artifacts()->getByIndex(i)->getId());
	}
	quest.mission.spells.clear();
	for(int i = 0; i < ui->lSpells->count(); ++i)
	{
		if(ui->lSpells->item(i)->checkState() == Qt::Checked)
			quest.mission.spells.push_back(VLC->spells()->getByIndex(i)->getId());
	}
	
	quest.mission.secondary.clear();
	for(int i = 0; i < ui->lSkills->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QComboBox*>(ui->lSkills->cellWidget(i, 1)))
		{
			if(widget->currentIndex() > 0)
				quest.mission.secondary[VLC->skills()->getByIndex(i)->getId()] = widget->currentIndex();
		}
	}
	
	quest.mission.creatures.clear();
	for(int i = 0; i < ui->lCreatures->rowCount(); ++i)
	{
		int index = ui->lCreatures->item(i, 0)->data(Qt::UserRole).toInt();
		if(auto * widget = qobject_cast<QSpinBox*>(ui->lCreatures->cellWidget(i, 1)))
			if(widget->value())
				quest.mission.creatures.emplace_back(VLC->creatures()->getByIndex(index)->getId(), widget->value());
	}
	
	quest.mission.heroes.clear();
	for(int i = 0; i < ui->lHeroes->count(); ++i)
	{
		if(ui->lHeroes->item(i)->checkState() == Qt::Checked)
			quest.mission.heroes.emplace_back(ui->lHeroes->item(i)->data(Qt::UserRole).toInt());
	}
	
	quest.mission.heroClasses.clear();
	for(int i = 0; i < ui->lHeroClasses->count(); ++i)
	{
		if(ui->lHeroClasses->item(i)->checkState() == Qt::Checked)
			quest.mission.heroClasses.emplace_back(ui->lHeroClasses->item(i)->data(Qt::UserRole).toInt());
	}
	
	quest.mission.players.clear();
	for(int i = 0; i < ui->lPlayers->count(); ++i)
	{
		if(ui->lPlayers->item(i)->checkState() == Qt::Checked)
			quest.mission.players.emplace_back(ui->lPlayers->item(i)->data(Qt::UserRole).toInt());
	}
	
	//quest.killTarget is set directly in object picking
	
	return true;
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

void QuestWidget::on_lKillTargetSelect_clicked()
{
	auto pred = [](const CGObjectInstance * obj) -> bool
	{
		if(auto * o = dynamic_cast<const CGHeroInstance*>(obj))
			return o->ID != Obj::PRISON;
		if(dynamic_cast<const CGCreature*>(obj))
			return true;
		return false;
	};
	
	for(int lvl : {0, 1})
	{
		auto & l = controller.scene(lvl)->objectPickerView;
		l.highlight(pred);
		l.update();
		QObject::connect(&l, &ObjectPickerLayer::selectionMade, this, &QuestWidget::onTargetPicked);
	}
	
	hide();
}

void QuestWidget::onTargetPicked(const CGObjectInstance * obj)
{
	show();
	
	for(int lvl : {0, 1})
	{
		auto & l = controller.scene(lvl)->objectPickerView;
		l.clear();
		l.update();
		QObject::disconnect(&l, &ObjectPickerLayer::selectionMade, this, &QuestWidget::onTargetPicked);
	}
	
	if(!obj) //discarded
	{
		quest.killTarget = ObjectInstanceID::NONE;
		ui->lKillTarget->setText("");
		return;
	}
	
	ui->lKillTarget->setText(QString::fromStdString(obj->instanceName));
	quest.killTarget = obj->id;
}

void QuestWidget::on_lCreatureAdd_clicked()
{
	onCreatureAdd(ui->lCreatures, ui->lCreatureId, ui->lCreatureAmount);
}


void QuestWidget::on_lCreatureRemove_clicked()
{
	std::set<int, std::greater<int>> rowsToRemove;
	for(auto * i : ui->lCreatures->selectedItems())
		rowsToRemove.insert(i->row());
	
	for(auto i : rowsToRemove)
		ui->lCreatures->removeRow(i);
}

QuestDelegate::QuestDelegate(MapController & c, CQuest & t): controller(c), quest(t), QStyledItemDelegate()
{
}

QWidget * QuestDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new QuestWidget(controller, quest, parent);
}

void QuestDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	if(auto *ed = qobject_cast<QuestWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void QuestDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	if(auto *ed = qobject_cast<QuestWidget *>(editor))
	{
		ed->commitChanges();
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

bool QuestDelegate::eventFilter(QObject * object, QEvent * event)
{
	if(auto * ed = qobject_cast<QuestWidget *>(object))
	{
		if(event->type() == QEvent::Hide || event->type() == QEvent::FocusOut)
			return false;
		if(event->type() == QEvent::Close)
		{
			emit commitData(ed);
			emit closeEditor(ed);
			return true;
		}
	}
	return QStyledItemDelegate::eventFilter(object, event);
}
