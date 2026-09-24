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
#include "../helper.h"
#include "questwidget.h"
#include "ui_questwidget.h"
#include "../mapcontroller.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/entities/ResourceTypeHandler.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/spells/CSpellHandler.h"

#include <vcmi/HeroTypeService.h>
#include <vcmi/HeroType.h>
#include <vcmi/HeroClassService.h>
#include <vcmi/HeroClass.h>
#include <vcmi/spells/Service.h>
#include <vcmi/spells/Spell.h>
#include "../translator.h"

QuestWidget::QuestWidget(MapController & _controller, QuestSource & questSource, QWidget *parent) :
	QDialog(parent),
	controller(_controller),
	questSource(questSource),
	ui(new Ui::QuestWidget)
{
	setAttribute(Qt::WA_DeleteOnClose, true);
	ui->setupUi(this);
	Helper::decorateDialog(this);
	ui->lDayOfWeek->addItem(tr("None"));
	for(int i = 1; i <= 7; ++i)
		ui->lDayOfWeek->addItem(tr("Day %1").arg(i));
	
	//fill resources
	ui->lResources->setRowCount(LIBRARY->resourceTypeHandler->getAllObjects().size());
	for(auto & i : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		MetaString str;
		str.appendName(GameResID(i));
		auto * item = new QTableWidgetItem(QString::fromStdString(str.toString(&Translator::instance())));
		item->setData(Qt::UserRole, QVariant::fromValue(i.getNum()));
		ui->lResources->setItem(i, 0, item);
		auto * spinBox = new QSpinBox;
		spinBox->setMaximum(i == GameResID::GOLD ? 999999 : 999);
		ui->lResources->setCellWidget(i, 1, spinBox);
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
		ui->lArtifacts->addItem(item);
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
		ui->lSpells->addItem(item);
	}
	
	//fill skills
	ui->lSkills->setRowCount(LIBRARY->skillh->objects.size());
	for(const auto & skillPtr : LIBRARY->skillh->objects)
	{
		auto skillIndex = skillPtr->getIndex();
		auto * item = new QTableWidgetItem(QString::fromStdString(skillPtr->getNameTranslated()));
		
		auto * widget = new QComboBox;
		for(const auto & s : NSecondarySkill::levels)
			widget->addItem(QString::fromUtf8(s));
		
		if(controller.map()->allowedAbilities.count(skillIndex) == 0)
		{
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
			widget->setEnabled(false);
		}
			
		ui->lSkills->setItem(skillIndex, 0, item);
		ui->lSkills->setCellWidget(skillIndex, 1, widget);
	}
	
	//fill creatures
	for(auto & creature : LIBRARY->creh->objects)
	{
		ui->lCreatureId->addItem(QString::fromStdString(creature->getNameSingularTranslated()));
		ui->lCreatureId->setItemData(ui->lCreatureId->count() - 1, creature->getIndex());
	}
	
	//fill heroes
	LIBRARY->heroTypes()->forEach([this](const HeroType * hero, bool &)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(hero->getNameTranslated()));
		item->setData(Qt::UserRole, QVariant::fromValue(hero->getId().getNum()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->lHeroes->addItem(item);
	});
	
	//fill hero classes
	LIBRARY->heroClasses()->forEach([this](const HeroClass * heroClass, bool &)
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
		MetaString str;
		str.appendName(color);
		auto * item = new QListWidgetItem(QString::fromStdString(str.toString(&Translator::instance())));
		item->setData(Qt::UserRole, QVariant::fromValue(color.getNum()));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		ui->lPlayers->addItem(item);
	}

	if (!questSource.allQuests().empty())
		selectedQuest = questSource.allQuestsEditor()[0];

	QObject::connect(ui->questRequirementsTab, &QTabWidget::currentChanged, this, &QuestWidget::highlightModifiedTabs);
}

QuestWidget::~QuestWidget()
{
	delete ui;
}

void QuestWidget::loadQuestData()
{
	if (!selectedQuest)
	{
		ui->QuestSettings->hide();
		ui->textsWidget->hide();
		return;
	}
	else
	{
		ui->QuestSettings->show();
		ui->textsWidget->show();
	}

	ui->lDayOfWeek->setCurrentIndex(selectedQuest->mission.dayOfWeek);
	ui->lDaysPassed->setValue(selectedQuest->mission.daysPassed);
	ui->lHeroLevel->setValue(selectedQuest->mission.heroLevel);
	ui->lHeroExperience->setValue(selectedQuest->mission.heroExperience);
	ui->lManaPoints->setValue(selectedQuest->mission.manaPoints);
	ui->lManaPercentage->setValue(selectedQuest->mission.manaPercentage);
	ui->lAttack->setValue(selectedQuest->mission.primary[0]);
	ui->lDefence->setValue(selectedQuest->mission.primary[1]);
	ui->lPower->setValue(selectedQuest->mission.primary[2]);
	ui->lKnowledge->setValue(selectedQuest->mission.primary[3]);

	//remove values of prior quest
	auto uncheckListWidget = [](QListWidget * listWidget) {
		for (int i = 0; i < listWidget->count(); ++i)
			listWidget->item(i)->setCheckState(Qt::Unchecked);
	};
	uncheckListWidget(ui->lArtifacts);
	uncheckListWidget(ui->lSpells);
	for (int i = 0; i < ui->lSkills->rowCount(); ++i)
		qobject_cast<QComboBox*>(ui->lSkills->cellWidget(i, 1))->setCurrentIndex(0);
	ui->lCreatures->setRowCount(0);
	uncheckListWidget(ui->lHeroes);
	uncheckListWidget(ui->lHeroClasses);
	uncheckListWidget(ui->lPlayers);


	//assign values of currently selected quest
	for(int i = 0; i < ui->lResources->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QSpinBox*>(ui->lResources->cellWidget(i, 1)))
			widget->setValue(selectedQuest->mission.resources[i]);
	}

	for(auto i : selectedQuest->mission.artifacts)
		ui->lArtifacts->item(LIBRARY->artifacts()->getById(i)->getIndex())->setCheckState(Qt::Checked);

	for(auto i : selectedQuest->mission.spells)
		ui->lSpells->item(LIBRARY->spells()->getById(i)->getIndex())->setCheckState(Qt::Checked);

	for(auto & i : selectedQuest->mission.secondary)
	{
		int index = LIBRARY->skills()->getById(i.first)->getIndex();
		if(auto * widget = qobject_cast<QComboBox*>(ui->lSkills->cellWidget(index, 1)))
			widget->setCurrentIndex(i.second);
	}
	for(auto & i : selectedQuest->mission.creatures)
	{
		int index = i.getType()->getIndex();
		ui->lCreatureId->setCurrentIndex(index);
		ui->lCreatureAmount->setValue(i.getCount());
		onCreatureAdd(ui->lCreatures, ui->lCreatureId, ui->lCreatureAmount);
	}
	for(auto & i : selectedQuest->mission.heroes)
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
	for(auto & i : selectedQuest->mission.heroClasses)
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
	for(auto & i : selectedQuest->mission.players)
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

	ObjectInstanceID killTarget = selectedQuest->mission.destroyedObjects.empty() ? ObjectInstanceID::NONE : selectedQuest->mission.destroyedObjects.front();
	if(killTarget != ObjectInstanceID::NONE && killTarget < controller.map()->objects.size())
		ui->lKillTarget->setText(QString::fromStdString(controller.map()->objects[killTarget]->instanceName));
	else
	{
		selectedQuest->mission.destroyedObjects.clear();
		ui->lKillTarget->clear();
	}
	questDataLoaded = true;

	ui->firstVisitText->clear();
	ui->firstVisitText->append(QString::fromStdString(selectedQuest->firstVisitText.toString(&Translator::instance())));

	ui->nextVisitText->clear();
	ui->nextVisitText->append(QString::fromStdString(selectedQuest->nextVisitText.toString(&Translator::instance())));

	ui->completedText->clear();
	ui->completedText->append(QString::fromStdString(selectedQuest->completedText.toString(&Translator::instance())));

	ui->repetableCheckbox->setChecked(selectedQuest->repeatedQuest);
	ui->deadlineCheckbox->setChecked(selectedQuest->lastDay >= 0);
	ui->deadlineSpinbox->setDisabled(selectedQuest->lastDay < 0);
	ui->deadlineSpinbox->setValue(selectedQuest->lastDay);

	highlightModifiedTabs();
}

void QuestWidget::obtainData()
{
	prepareQuestsList();
	selectQuest(0);
}

void QuestWidget::prepareQuestsList(const std::shared_ptr<Quest> & questToSelect)
{
	auto & allQuests = questSource.allQuestsEditor();
	ui->questsList->clear();

	std::ranges::stable_sort(allQuests, [](auto & a, auto & b)
					 {
						 return !a->repeatedQuest && b->repeatedQuest;
					 });

	setTranslationIdentifiers();	//translation identifiers depend on the order

	for(int i = 0; i< allQuests.size(); ++i)
	{
		ui->questsList->addItem(tr("%1 quest on position %2")
									.arg(tr(allQuests[i]->repeatedQuest ? "Repeated" : "One time"))
									.arg(i));
		if (questToSelect && questSource.allQuests()[i] == questToSelect)
		{
			ui->questsList->setCurrentRow(i);
			loadQuestData();
		}
	}
	if (ui->questsList->count() == 0)
		selectedQuest = nullptr;
}

void QuestWidget::setTranslationIdentifiers()
{
	auto & allQuests = questSource.allQuestsEditor();
	std::vector<std::string> fv;
	std::vector<std::string> nv;
	std::vector<std::string> com;
	for(int i = 0; i< allQuests.size(); ++i) {
		fv.push_back(allQuests[i]->firstVisitText.toString(&Translator::instance()));
		nv.push_back(allQuests[i]->nextVisitText.toString(&Translator::instance()));
		com.push_back(allQuests[i]->completedText.toString(&Translator::instance()));
	}

	for(int i = 0; i< allQuests.size(); ++i) {
		setTranslation(allQuests[i]->firstVisitText, TextIdentifier("quest", questSource.instanceName, i, "firstVisit"), fv[i]);
		setTranslation(allQuests[i]->nextVisitText, TextIdentifier("quest", questSource.instanceName, i, "nextVisit"), nv[i]);
		setTranslation(allQuests[i]->completedText, TextIdentifier("quest", questSource.instanceName, i, "completed"), com[i]);
	}
}

void QuestWidget::setTranslation(MetaString & metastring, const TextIdentifier & identifier, const std::string & translation)
{
	metastring = MetaString::createFromTextID(mapRegisterLocalizedString(
		"map", *controller.map(), identifier, translation));
	if(translation.empty())
		metastring.clear();
}

void QuestWidget::selectQuest(int index)
{
	if (questSource.allQuests().size() > index)
	{
		ui->questsList->setCurrentRow(index);
		selectedQuest = questSource.allQuestsEditor()[index];
	}
	loadQuestData();
}

bool QuestWidget::commitChanges()
{
	if (!selectedQuest)
		return false;

	selectedQuest->mission.dayOfWeek = ui->lDayOfWeek->currentIndex();
	selectedQuest->mission.daysPassed = ui->lDaysPassed->value();
	selectedQuest->mission.heroLevel = ui->lHeroLevel->value();
	selectedQuest->mission.heroExperience = ui->lHeroExperience->value();
	selectedQuest->mission.manaPoints = ui->lManaPoints->value();
	selectedQuest->mission.manaPercentage = ui->lManaPercentage->value();
	selectedQuest->mission.primary[0] = ui->lAttack->value();
	selectedQuest->mission.primary[1] = ui->lDefence->value();
	selectedQuest->mission.primary[2] = ui->lPower->value();
	selectedQuest->mission.primary[3] = ui->lKnowledge->value();
	for(int i = 0; i < ui->lResources->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QSpinBox*>(ui->lResources->cellWidget(i, 1)))
			selectedQuest->mission.resources[i] = widget->value();
	}
	
	selectedQuest->mission.artifacts.clear();
	for(int i = 0; i < ui->lArtifacts->count(); ++i)
	{
		if(ui->lArtifacts->item(i)->checkState() == Qt::Checked)
			selectedQuest->mission.artifacts.push_back(LIBRARY->artifacts()->getByIndex(i)->getId());
	}
	selectedQuest->mission.spells.clear();
	for(int i = 0; i < ui->lSpells->count(); ++i)
	{
		if(ui->lSpells->item(i)->checkState() == Qt::Checked)
			selectedQuest->mission.spells.push_back(LIBRARY->spells()->getByIndex(i)->getId());
	}
	
	selectedQuest->mission.secondary.clear();
	for(int i = 0; i < ui->lSkills->rowCount(); ++i)
	{
		if(auto * widget = qobject_cast<QComboBox*>(ui->lSkills->cellWidget(i, 1)))
		{
			if(widget->currentIndex() > 0)
				selectedQuest->mission.secondary[LIBRARY->skills()->getByIndex(i)->getId()] = widget->currentIndex();
		}
	}
	
	selectedQuest->mission.creatures.clear();
	for(int i = 0; i < ui->lCreatures->rowCount(); ++i)
	{
		int index = ui->lCreatures->item(i, 0)->data(Qt::UserRole).toInt();
		if(auto * widget = qobject_cast<QSpinBox*>(ui->lCreatures->cellWidget(i, 1)))
			if(widget->value())
				selectedQuest->mission.creatures.emplace_back(LIBRARY->creatures()->getByIndex(index)->getId(), widget->value());
	}
	
	selectedQuest->mission.heroes.clear();
	for(int i = 0; i < ui->lHeroes->count(); ++i)
	{
		if(ui->lHeroes->item(i)->checkState() == Qt::Checked)
			selectedQuest->mission.heroes.emplace_back(ui->lHeroes->item(i)->data(Qt::UserRole).toInt());
	}
	
	selectedQuest->mission.heroClasses.clear();
	for(int i = 0; i < ui->lHeroClasses->count(); ++i)
	{
		if(ui->lHeroClasses->item(i)->checkState() == Qt::Checked)
			selectedQuest->mission.heroClasses.emplace_back(ui->lHeroClasses->item(i)->data(Qt::UserRole).toInt());
	}
	
	selectedQuest->mission.players.clear();
	for(int i = 0; i < ui->lPlayers->count(); ++i)
	{
		if(ui->lPlayers->item(i)->checkState() == Qt::Checked)
			selectedQuest->mission.players.emplace_back(ui->lPlayers->item(i)->data(Qt::UserRole).toInt());
	}

	auto index = std::find(questSource.allQuests().begin(), questSource.allQuests().end(), selectedQuest) - questSource.allQuests().begin();
	setTranslation(selectedQuest->firstVisitText, TextIdentifier("quest", questSource.instanceName, index, "firstVisit"), ui->firstVisitText->toPlainText().toStdString());
	setTranslation(selectedQuest->nextVisitText, TextIdentifier("quest", questSource.instanceName, index, "nextVisit"), ui->nextVisitText->toPlainText().toStdString());
	setTranslation(selectedQuest->completedText, TextIdentifier("quest", questSource.instanceName, index, "completed"), ui->completedText->toPlainText().toStdString());

	selectedQuest->repeatedQuest = ui->repetableCheckbox->isChecked();
	selectedQuest->lastDay = ui->deadlineCheckbox->isChecked() ? ui->deadlineSpinbox->value() : -1;
	
	//selectedQuest->mission.destroyedObjects is set directly in object picking
	
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

void QuestWidget::highlightModifiedTabs()
{
	auto getColorOfListWidgetTab = [](QListWidget * list) -> QColor {
		auto tabColor = Qt::black;
		for (int i = 0; i < list->count(); ++i)
		{
			if (list->item(i)->checkState() == Qt::Checked)
			{
				tabColor = Qt::darkYellow;
				break;
			}
		}
		return tabColor;
	};

	auto resourceTabColor = Qt::black;
	for(int i = 0; i < ui->lResources->rowCount(); ++i)
	{
		if (qobject_cast<QSpinBox*>(ui->lResources->cellWidget(i, 1))->value() > 0)
		{
			resourceTabColor = Qt::darkYellow;
			break;
		}
	}

	auto skillsColor = Qt::black;
	for (int i = 0; i < ui->lSkills->rowCount(); ++i)
	{
		if (qobject_cast<QComboBox*>(ui->lSkills->cellWidget(i, 1))->currentIndex() > 0)
		{
			skillsColor = Qt::darkYellow;
			break;
		}
	}

	ui->questRequirementsTab->tabBar()->setTabTextColor(0, resourceTabColor);
	ui->questRequirementsTab->tabBar()->setTabTextColor(1, getColorOfListWidgetTab(ui->lArtifacts));
	ui->questRequirementsTab->tabBar()->setTabTextColor(2, getColorOfListWidgetTab(ui->lSpells));
	ui->questRequirementsTab->tabBar()->setTabTextColor(3, skillsColor);
	ui->questRequirementsTab->tabBar()->setTabTextColor(4, ui->lCreatures->rowCount() > 0 ? Qt::red : Qt::black);
	ui->questRequirementsTab->tabBar()->setTabTextColor(5, getColorOfListWidgetTab(ui->lHeroes));
	ui->questRequirementsTab->tabBar()->setTabTextColor(6, getColorOfListWidgetTab(ui->lHeroClasses));
	ui->questRequirementsTab->tabBar()->setTabTextColor(7, getColorOfListWidgetTab(ui->lPlayers));

	for (auto & child : ui->questRequirementsTab->tabBar()->findChildren<QWidget *>())
	{
		child->update();
	}
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
	
	for(MapScene * level : controller.getScenes())
	{
		auto & l = level->objectPickerView;
		l.highlight(pred);
		l.update();
		QObject::connect(&l, &ObjectPickerLayer::selectionMade, this, &QuestWidget::onTargetPicked);
	}
	
	hide();
}

void QuestWidget::onTargetPicked(const CGObjectInstance * obj)
{
	show();
	
	for(MapScene * level : controller.getScenes())
	{
		auto & l = level->objectPickerView;
		l.clear();
		l.update();
		QObject::disconnect(&l, &ObjectPickerLayer::selectionMade, this, &QuestWidget::onTargetPicked);
	}
	
	if(!obj) //discarded
	{
		selectedQuest->mission.destroyedObjects.clear();
		ui->lKillTarget->setText("");
		return;
	}

	ui->lKillTarget->setText(QString::fromStdString(obj->instanceName));
	selectedQuest->mission.destroyedObjects = { obj->id };
}

void QuestWidget::on_addQuestButton_clicked()
{
	questSource.addQuest();
	prepareQuestsList();
	selectQuest(ui->questsList->count() - 1);
}

void QuestWidget::on_deleteQuestButton_clicked()
{
	if (questSource.allQuests().empty())
		return;
	questSource.allQuestsEditor().erase(questSource.allQuests().begin() + ui->questsList->currentRow());
	prepareQuestsList();
	selectQuest(0);
}

void QuestWidget::on_deadlineCheckbox_stateChanged(int state)
{
	ui->deadlineSpinbox->setDisabled(!state);
}

void QuestWidget::on_repetableCheckbox_stateChanged(int state)
{
	commitChanges();
	prepareQuestsList(selectedQuest);
}

void QuestWidget::on_questsList_currentRowChanged(int row)
{
	if (!questDataLoaded || row == -1)
		return;

	commitChanges();
	questDataLoaded = false;
	selectQuest(row);
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

QuestDelegate::QuestDelegate(MapController & c, QuestSource & questSource): BaseInspectorItemDelegate(), controller(c), questSource(questSource)
{
}

QWidget * QuestDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new QuestWidget(controller, questSource, parent);
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
		updateModelData(model, index);
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
			commitData(ed);
			closeEditor(ed);
			return true;
		}
	}
	return QStyledItemDelegate::eventFilter(object, event);
}

void QuestDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList textList(QObject::tr("Quest:"));

	setModelTextData(model, index, textList);
}
