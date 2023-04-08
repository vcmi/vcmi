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
#include "../lib/VCMI_Lib.h"
#include "../lib/CSkillHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/StringConstants.h"

QuestWidget::QuestWidget(const CMap & _map, CGSeerHut & _sh, QWidget *parent) :
	QDialog(parent),
	map(_map),
	seerhut(_sh),
	ui(new Ui::QuestWidget)
{
	ui->setupUi(this);
}

QuestWidget::~QuestWidget()
{
	delete ui;
}

void QuestWidget::obtainData()
{
	assert(seerhut.quest);
	bool activeId = false;
	bool activeAmount = false;
	switch(seerhut.quest->missionType) {
		case CQuest::Emission::MISSION_LEVEL:
			activeAmount = true;
			ui->targetId->addItem("Reach level");
			ui->targetAmount->setText(QString::number(seerhut.quest->m13489val));
			break;
		case CQuest::Emission::MISSION_PRIMARY_STAT:
			activeId = true;
			activeAmount = true;
			for(auto s : PrimarySkill::names)
				ui->targetId->addItem(QString::fromStdString(s));
			for(int i = 0; i < seerhut.quest->m2stats.size(); ++i)
			{
				if(seerhut.quest->m2stats[i] > 0)
				{
					ui->targetId->setCurrentIndex(i);
					ui->targetAmount->setText(QString::number(seerhut.quest->m2stats[i]));
					break; //TODO: support multiple stats
				}
			}
			break;
		case CQuest::Emission::MISSION_KILL_HERO:
			activeId = true;
			//TODO: implement
			break;
		case CQuest::Emission::MISSION_KILL_CREATURE:
			activeId = true;
			//TODO: implement
			break;
		case CQuest::Emission::MISSION_ART:
			activeId = true;
			for(int i = 0; i < map.allowedArtifact.size(); ++i)
				ui->targetId->addItem(QString::fromStdString(VLC->arth->objects.at(i)->getNameTranslated()));
			if(!seerhut.quest->m5arts.empty())
				ui->targetId->setCurrentIndex(seerhut.quest->m5arts.front());
			//TODO: support multiple artifacts
			break;
		case CQuest::Emission::MISSION_ARMY:
			activeId = true;
			activeAmount = true;
			break;
		case CQuest::Emission::MISSION_RESOURCES:
			activeId = true;
			activeAmount = true;
			for(auto s : GameConstants::RESOURCE_NAMES)
				ui->targetId->addItem(QString::fromStdString(s));
			for(int i = 0; i < seerhut.quest->m7resources.size(); ++i)
			{
				if(seerhut.quest->m7resources[i] > 0)
				{
					ui->targetId->setCurrentIndex(i);
					ui->targetAmount->setText(QString::number(seerhut.quest->m7resources[i]));
					break; //TODO: support multiple resources
				}
			}
			break;
		case CQuest::Emission::MISSION_HERO:
			activeId = true;
			for(int i = 0; i < map.allowedHeroes.size(); ++i)
				ui->targetId->addItem(QString::fromStdString(VLC->heroh->objects.at(i)->getNameTranslated()));
			ui->targetId->setCurrentIndex(seerhut.quest->m13489val);
			break;
		case CQuest::Emission::MISSION_PLAYER:
			activeId = true;
			for(auto s : GameConstants::PLAYER_COLOR_NAMES)
				ui->targetId->addItem(QString::fromStdString(s));
			ui->targetId->setCurrentIndex(seerhut.quest->m13489val);
			break;
		case CQuest::Emission::MISSION_KEYMASTER:
			break;
		default:
			break;
	}
	
	ui->targetId->setEnabled(activeId);
	ui->targetAmount->setEnabled(activeAmount);
}

QString QuestWidget::commitChanges()
{
	assert(seerhut.quest);
	switch(seerhut.quest->missionType) {
		case CQuest::Emission::MISSION_LEVEL:
			seerhut.quest->m13489val = ui->targetAmount->text().toInt();
			return QString("Reach lvl ").append(ui->targetAmount->text());
		case CQuest::Emission::MISSION_PRIMARY_STAT:
			seerhut.quest->m2stats.resize(sizeof(PrimarySkill::names), 0);
			seerhut.quest->m2stats[ui->targetId->currentIndex()] = ui->targetAmount->text().toInt();
			//TODO: support multiple stats
			return ui->targetId->currentText().append(ui->targetAmount->text());
		case CQuest::Emission::MISSION_KILL_HERO:
			//TODO: implement
			return QString("N/A");
		case CQuest::Emission::MISSION_KILL_CREATURE:
			//TODO: implement
			return QString("N/A");
		case CQuest::Emission::MISSION_ART:
			seerhut.quest->m5arts.clear();
			seerhut.quest->m5arts.push_back(ArtifactID(ui->targetId->currentIndex()));
			//TODO: support multiple artifacts
			return ui->targetId->currentText();
		case CQuest::Emission::MISSION_ARMY:
			//TODO: implement
			return QString("N/A");
		case CQuest::Emission::MISSION_RESOURCES:
			seerhut.quest->m7resources[ui->targetId->currentIndex()] = ui->targetAmount->text().toInt();
			//TODO: support resources
			return ui->targetId->currentText().append(ui->targetAmount->text());
		case CQuest::Emission::MISSION_HERO:
			seerhut.quest->m13489val = ui->targetId->currentIndex();
			return ui->targetId->currentText();
		case CQuest::Emission::MISSION_PLAYER:
			seerhut.quest->m13489val = ui->targetId->currentIndex();
			return ui->targetId->currentText();
		case CQuest::Emission::MISSION_KEYMASTER:
			return QString("N/A");
		default:
			return QString("N/A");
	}
}

QuestDelegate::QuestDelegate(const CMap & m, CGSeerHut & t): map(m), seerhut(t), QStyledItemDelegate()
{
}

QWidget * QuestDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new QuestWidget(map, seerhut, parent);
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
		auto quest = ed->commitChanges();
		model->setData(index, quest);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}
