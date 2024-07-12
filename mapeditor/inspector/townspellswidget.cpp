/*
 * townspellswidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "townspellswidget.h"
#include "ui_townspellswidget.h"
#include "inspector.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/spells/CSpellHandler.h"

TownSpellsWidget::TownSpellsWidget(CGTownInstance & town, QWidget * parent) :
	QDialog(parent),
	ui(new Ui::TownSpellsWidget),
	town(town)
{
	ui->setupUi(this);
	BuildingID mageGuilds[] = {BuildingID::MAGES_GUILD_1, BuildingID::MAGES_GUILD_2, BuildingID::MAGES_GUILD_3, BuildingID::MAGES_GUILD_4, BuildingID::MAGES_GUILD_5};
	for (int i = 0; i < GameConstants::SPELL_LEVELS; i++)
	{
		ui->tabWidget->setTabEnabled(i, vstd::contains(town.getTown()->buildings, mageGuilds[i]));
	}
}

TownSpellsWidget::~TownSpellsWidget()
{
	delete ui;
}


void TownSpellsWidget::obtainData()
{
	initSpellLists();
	if (vstd::contains(town.possibleSpells, SpellID::PRESET)) {
		ui->customizeSpells->setChecked(true);
	}
	else
	{
		ui->customizeSpells->setChecked(false);
		ui->tabWidget->setEnabled(false);
	}
}

void TownSpellsWidget::resetSpells()
{
	town.possibleSpells.clear();
	town.obligatorySpells.clear();
	for (auto spell : VLC->spellh->objects)
	{
		if (!spell->isSpecial() && !spell->isCreatureAbility())
			town.possibleSpells.push_back(spell->id);
	}
}

void TownSpellsWidget::initSpellLists()
{
	QListWidget * possibleSpellLists[] = { ui->possibleSpellList1, ui->possibleSpellList2, ui->possibleSpellList3, ui->possibleSpellList4, ui->possibleSpellList5 };
	QListWidget * requiredSpellLists[] = { ui->requiredSpellList1, ui->requiredSpellList2, ui->requiredSpellList3, ui->requiredSpellList4, ui->requiredSpellList5 };
	auto spells = VLC->spellh->objects;
	for (int i = 0; i < GameConstants::SPELL_LEVELS; i++)
	{
		std::vector<std::shared_ptr<CSpell>> spellsByLevel;
		auto getSpellsByLevel = [i](auto spell) {
			return spell->getLevel() == i + 1 && !spell->isSpecial() && !spell->isCreatureAbility();
		};
		vstd::copy_if(spells, std::back_inserter(spellsByLevel), getSpellsByLevel);
		possibleSpellLists[i]->clear();
		requiredSpellLists[i]->clear();
		for (auto spell : spellsByLevel)
		{
			auto * possibleItem = new QListWidgetItem(QString::fromStdString(spell->getNameTranslated()));
			possibleItem->setData(Qt::UserRole, QVariant::fromValue(spell->getIndex()));
			possibleItem->setFlags(possibleItem->flags() | Qt::ItemIsUserCheckable);
			possibleItem->setCheckState(vstd::contains(town.possibleSpells, spell->getId()) ? Qt::Checked : Qt::Unchecked);
			possibleSpellLists[i]->addItem(possibleItem);

			auto * requiredItem = new QListWidgetItem(QString::fromStdString(spell->getNameTranslated()));
			requiredItem->setData(Qt::UserRole, QVariant::fromValue(spell->getIndex()));
			requiredItem->setFlags(requiredItem->flags() | Qt::ItemIsUserCheckable);
			requiredItem->setCheckState(vstd::contains(town.obligatorySpells, spell->getId()) ? Qt::Checked : Qt::Unchecked);
			requiredSpellLists[i]->addItem(requiredItem);
		}
	}
}

void TownSpellsWidget::commitChanges()
{
	if (!ui->tabWidget->isEnabled())
	{
		resetSpells();
		return;
	}
	town.possibleSpells.clear();
	town.obligatorySpells.clear();
	town.possibleSpells.push_back(SpellID::PRESET);
	QListWidget * possibleSpellLists[] = { ui->possibleSpellList1, ui->possibleSpellList2, ui->possibleSpellList3, ui->possibleSpellList4, ui->possibleSpellList5 };
	QListWidget * requiredSpellLists[] = { ui->requiredSpellList1, ui->requiredSpellList2, ui->requiredSpellList3, ui->requiredSpellList4, ui->requiredSpellList5 };
	for (auto spellList : possibleSpellLists)
	{
		for (int i = 0; i < spellList->count(); i++)
		{
			auto * item = spellList->item(i);
			if (item->checkState() == Qt::Checked)
			{
				town.possibleSpells.push_back(item->data(Qt::UserRole).toInt());
			}
		}
	}
	for (auto spellList : requiredSpellLists)
	{
		for (int i = 0; i < spellList->count(); i++)
		{
			auto * item = spellList->item(i);
			if (item->checkState() == Qt::Checked)
			{
				town.obligatorySpells.push_back(item->data(Qt::UserRole).toInt());
			}
		}
	}
}

void TownSpellsWidget::on_customizeSpells_toggled(bool checked)
{
	if (checked)
	{
		town.possibleSpells.push_back(SpellID::PRESET);
	}
	else
	{
		resetSpells();
	}
	ui->tabWidget->setEnabled(checked);
	initSpellLists();
}

TownSpellsDelegate::TownSpellsDelegate(CGTownInstance & town) : town(town), QStyledItemDelegate()
{
}

QWidget * TownSpellsDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new TownSpellsWidget(town, parent);
}

void TownSpellsDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	if (auto * ed = qobject_cast<TownSpellsWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void TownSpellsDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	if (auto * ed = qobject_cast<TownSpellsWidget *>(editor))
	{
		ed->commitChanges();
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}