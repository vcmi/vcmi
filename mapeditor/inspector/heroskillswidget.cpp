/*
 * heroskillswidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "heroskillswidget.h"
#include "ui_heroskillswidget.h"
#include "inspector.h"

#include "../../lib/CSkillHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/entities/hero/CHeroHandler.h"

static const QList<std::pair<QString, QVariant>> LevelIdentifiers
{
	{QObject::tr("Beginner"), QVariant::fromValue(1)},
	{QObject::tr("Advanced"), QVariant::fromValue(2)},
	{QObject::tr("Expert"), QVariant::fromValue(3)},
};

HeroSkillsWidget::HeroSkillsWidget(CGHeroInstance & h, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::HeroSkillsWidget),
	hero(h)
{
	ui->setupUi(this);
	
	ui->labelAttack->setText(QString::fromStdString(NPrimarySkill::names[0]));
	ui->labelDefence->setText(QString::fromStdString(NPrimarySkill::names[1]));
	ui->labelPower->setText(QString::fromStdString(NPrimarySkill::names[2]));
	ui->labelKnowledge->setText(QString::fromStdString(NPrimarySkill::names[3]));
	
	auto * delegate = new InspectorDelegate;
	for(auto const & s : LIBRARY->skillh->objects)
		delegate->options.push_back({QString::fromStdString(s->getNameTranslated()), QVariant::fromValue(s->getId().getNum())});
	ui->skills->setItemDelegateForColumn(0, delegate);
	
	delegate = new InspectorDelegate;
	delegate->options = LevelIdentifiers;
	ui->skills->setItemDelegateForColumn(1, delegate);
}

HeroSkillsWidget::~HeroSkillsWidget()
{
	delete ui;
}

void HeroSkillsWidget::on_addButton_clicked()
{
	ui->skills->setRowCount(ui->skills->rowCount() + 1);
}

void HeroSkillsWidget::on_removeButton_clicked()
{
	ui->skills->removeRow(ui->skills->currentRow());
}

void HeroSkillsWidget::on_checkBox_toggled(bool checked)
{
	ui->skills->setEnabled(checked);
	ui->addButton->setEnabled(checked);
	ui->removeButton->setEnabled(checked);
}

void HeroSkillsWidget::obtainData()
{
	ui->attack->setValue(hero.getBasePrimarySkillValue(PrimarySkill::ATTACK));
	ui->defence->setValue(hero.getBasePrimarySkillValue(PrimarySkill::DEFENSE));
	ui->power->setValue(hero.getBasePrimarySkillValue(PrimarySkill::SPELL_POWER));
	ui->knowledge->setValue(hero.getBasePrimarySkillValue(PrimarySkill::KNOWLEDGE));
	
	if(!hero.secSkills.empty() && hero.secSkills.front().first.getNum() == -1)
		return;
	
	ui->checkBox->setChecked(true);
	ui->skills->setRowCount(hero.secSkills.size());
	
	int i = 0;
	for(auto & s : hero.secSkills)
	{
		auto * itemSkill = new QTableWidgetItem;
		itemSkill->setText(QString::fromStdString(LIBRARY->skillh->getById(s.first)->getNameTranslated()));
		itemSkill->setData(Qt::UserRole, QVariant::fromValue(s.first.getNum()));
		ui->skills->setItem(i, 0, itemSkill);
		
		auto * itemLevel = new QTableWidgetItem;
		itemLevel->setText(LevelIdentifiers[s.second - 1].first);
		itemLevel->setData(Qt::UserRole, LevelIdentifiers[s.second - 1].second);
		ui->skills->setItem(i++, 1, itemLevel);
	}
}

void HeroSkillsWidget::commitChanges()
{
	hero.pushPrimSkill(PrimarySkill::ATTACK, ui->attack->value());
	hero.pushPrimSkill(PrimarySkill::DEFENSE, ui->defence->value());
	hero.pushPrimSkill(PrimarySkill::SPELL_POWER, ui->power->value());
	hero.pushPrimSkill(PrimarySkill::KNOWLEDGE, ui->knowledge->value());
	
	hero.secSkills.clear();
	
	if(!ui->checkBox->isChecked())
	{
		hero.secSkills.push_back(std::make_pair(SecondarySkill(-1), -1));
		return;
	}
	
	for(int i = 0; i < ui->skills->rowCount(); ++i)
	{
		if(ui->skills->item(i, 0) && ui->skills->item(i, 1))
			hero.secSkills.push_back(std::make_pair(SecondarySkill(ui->skills->item(i, 0)->data(Qt::UserRole).toInt()), ui->skills->item(i, 1)->data(Qt::UserRole).toInt()));
	}
}

HeroSkillsDelegate::HeroSkillsDelegate(CGHeroInstance & h)
	: BaseInspectorItemDelegate()
	, hero(h)
{
}

QWidget * HeroSkillsDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return new HeroSkillsWidget(hero, parent);
}

void HeroSkillsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if(auto * ed = qobject_cast<HeroSkillsWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void HeroSkillsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if(auto * ed = qobject_cast<HeroSkillsWidget *>(editor))
	{
		ed->commitChanges();
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

void HeroSkillsDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList textList;
	textList += QString("%1: %2").arg(QString::fromStdString(LIBRARY->generaltexth->primarySkillNames[PrimarySkill::ATTACK])).arg(hero.getBasePrimarySkillValue(PrimarySkill::ATTACK));
	textList += QString("%1: %2").arg(QString::fromStdString(LIBRARY->generaltexth->primarySkillNames[PrimarySkill::DEFENSE])).arg(hero.getBasePrimarySkillValue(PrimarySkill::DEFENSE));
	textList += QString("%1: %2").arg(QString::fromStdString(LIBRARY->generaltexth->primarySkillNames[PrimarySkill::SPELL_POWER])).arg(hero.getBasePrimarySkillValue(PrimarySkill::SPELL_POWER));
	textList += QString("%1: %2").arg(QString::fromStdString(LIBRARY->generaltexth->primarySkillNames[PrimarySkill::KNOWLEDGE])).arg(hero.getBasePrimarySkillValue(PrimarySkill::KNOWLEDGE));

	auto heroSecondarySkills = hero.secSkills;
	if(heroSecondarySkills.size() == 1 && heroSecondarySkills[0].first == SecondarySkill::NONE) 
	{
		if(hero.getHeroTypeID().isValid())
		{
			textList += QObject::tr("Default secondary skills:");
			heroSecondarySkills = hero.getHeroType()->secSkillsInit;
		}
		else
		{
			textList += QObject::tr("Random hero secondary skills");
			heroSecondarySkills.clear();
		}
	}
	else
	{
		textList += QObject::tr("Secondary skills:");
	}
	for(const auto & [skill, skillLevel] : heroSecondarySkills)
		textList += QString("%1 %2").arg(QString::fromStdString(LIBRARY->skillh->getById(skill)->getNameTranslated())).arg(LevelIdentifiers[skillLevel - 1].first);

	setModelTextData(model, index, textList);
}
