/*
 * herosspellwidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "herospellwidget.h"
#include "ui_herospellwidget.h"
#include "inspector.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/spells/CSpellHandler.h"

HeroSpellWidget::HeroSpellWidget(CGHeroInstance & h, QWidget * parent) :
	QDialog(parent),
	ui(new Ui::HeroSpellWidget),
	hero(h)
{
	ui->setupUi(this);
}

HeroSpellWidget::~HeroSpellWidget()
{
	delete ui;
}


void HeroSpellWidget::obtainData()
{
	initSpellLists();
	if (hero.spellbookContainsSpell(SpellID::PRESET)) {
		ui->customizeSpells->setChecked(true);
	}
	else
	{
		ui->customizeSpells->setChecked(false);
		ui->tabWidget->setEnabled(false);
	}
}

void HeroSpellWidget::initSpellLists()
{
	QListWidget * spellLists[] = { ui->spellList1, ui->spellList2, ui->spellList3, ui->spellList4, ui->spellList5 };

	for (int i = 0; i < GameConstants::SPELL_LEVELS; i++)
	{
		std::vector<const CSpell*> spellsByLevel;
		for (auto const & spellID : LIBRARY->spellh->getDefaultAllowed())
		{
			if (spellID.toSpell()->getLevel() == i + 1)
				spellsByLevel.push_back(spellID.toSpell());
		}

		spellLists[i]->clear();
		for (auto spell : spellsByLevel)
		{
			auto* item = new QListWidgetItem(QString::fromStdString(spell->getNameTranslated()));
			item->setData(Qt::UserRole, QVariant::fromValue(spell->getIndex()));
			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
			item->setCheckState(hero.spellbookContainsSpell(spell->getId()) ? Qt::Checked : Qt::Unchecked);
			spellLists[i]->addItem(item);
		}
	}
}

void HeroSpellWidget::commitChanges()
{
	QListWidget * spellLists[] = { ui->spellList1, ui->spellList2, ui->spellList3, ui->spellList4, ui->spellList5 };
	for (auto spellList : spellLists)
	{
		for (int i = 0; i < spellList->count(); i++)
		{
			auto * item = spellList->item(i);
			if (item->checkState() == Qt::Checked)
			{
				hero.addSpellToSpellbook(item->data(Qt::UserRole).toInt());
			}
			else
			{
				hero.removeSpellFromSpellbook(item->data(Qt::UserRole).toInt());
			}
		}
	}
}

void HeroSpellWidget::on_customizeSpells_toggled(bool checked)
{
	if (checked)
	{
		hero.addSpellToSpellbook(SpellID::PRESET);
	}
	else
	{
		hero.removeSpellFromSpellbook(SpellID::PRESET);
		hero.removeSpellbook();
	}
	ui->tabWidget->setEnabled(checked);
	initSpellLists();
}

HeroSpellDelegate::HeroSpellDelegate(CGHeroInstance & h)
	: BaseInspectorItemDelegate()
	, hero(h)
{
}

QWidget * HeroSpellDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new HeroSpellWidget(hero, parent);
}

void HeroSpellDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	if (auto * ed = qobject_cast<HeroSpellWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void HeroSpellDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	if (auto * ed = qobject_cast<HeroSpellWidget *>(editor))
	{
		ed->commitChanges();
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

void HeroSpellDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList textList;
	if(hero.spellbookContainsSpell(SpellID::PRESET))
	{
		textList += QObject::tr("Custom Spells:");
		for(auto const & spellID : hero.getSpellsInSpellbook())
		{
			if(spellID != SpellID::PRESET)
				textList += QString::fromStdString(spellID.toSpell()->getNameTranslated());
		}
	}
	else
	{
		textList += QObject::tr("Default Spells");
	}
	setModelTextData(model, index, textList);
}
