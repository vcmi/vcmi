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
#include "artifactwidget.h"
#include "heroartifactswidget.h"
#include "ui_heroartifactswidget.h"
#include "inspector.h"
#include "mapeditorroles.h"
#include "../../lib/ArtifactUtils.h"
#include "../../lib/constants/StringConstants.h"

HeroArtifactsWidget::HeroArtifactsWidget(CGHeroInstance & h, QWidget * parent) :
	QDialog(parent),
	ui(new Ui::HeroArtifactsWidget),
	hero(h),
	fittingSet(CArtifactFittingSet(h))
{
	ui->setupUi(this);
}

HeroArtifactsWidget::~HeroArtifactsWidget()
{
	delete ui;
}

void HeroArtifactsWidget::on_addButton_clicked()
{
	ArtifactWidget artifactWidget{ fittingSet, this };
	connect(&artifactWidget, &ArtifactWidget::saveArtifact, this, &HeroArtifactsWidget::onSaveArtifact);
	artifactWidget.exec();
}

void HeroArtifactsWidget::on_removeButton_clicked()
{
	auto row = ui->artifacts->currentRow();
	if (row == -1)
	{
		return;
	}

	auto slot = ui->artifacts->item(row, Column::SLOT)->data(MapEditorRoles::ArtifactSlotRole).toInt();
	fittingSet.removeArtifact(ArtifactPosition(slot));
	ui->artifacts->removeRow(row);
}

void HeroArtifactsWidget::onSaveArtifact(int32_t artifactIndex, ArtifactPosition slot) 
{
	auto artifact = ArtifactUtils::createArtifact(VLC->arth->getByIndex(artifactIndex)->getId());
	fittingSet.putArtifact(slot, artifact);
	addArtifactToTable(artifactIndex, slot);
}

void HeroArtifactsWidget::addArtifactToTable(int32_t artifactIndex, ArtifactPosition slot)
{
	auto artifact = VLC->arth->getByIndex(artifactIndex);
	auto * itemArtifact = new QTableWidgetItem;
	itemArtifact->setText(QString::fromStdString(artifact->getNameTranslated()));
	itemArtifact->setData(MapEditorRoles::ArtifactIDRole, QVariant::fromValue(artifact->getIndex()));

	auto * itemSlot = new QTableWidgetItem;
	auto slotText = ArtifactUtils::isSlotBackpack(slot) ? NArtifactPosition::backpack : NArtifactPosition::namesHero[slot.num];
	itemSlot->setData(MapEditorRoles::ArtifactSlotRole, QVariant::fromValue(slot.num));
	itemSlot->setText(QString::fromStdString(slotText));

	ui->artifacts->insertRow(ui->artifacts->rowCount());
	ui->artifacts->setItem(ui->artifacts->rowCount() - 1, Column::ARTIFACT, itemArtifact);
	ui->artifacts->setItem(ui->artifacts->rowCount() - 1, Column::SLOT, itemSlot);
}

void HeroArtifactsWidget::obtainData()
{
	std::vector<const CArtifact *> combinedArtifactsParts;
	for (const auto & [artPosition, artSlotInfo] : fittingSet.artifactsWorn)
	{
		addArtifactToTable(VLC->arth->getById(artSlotInfo.artifact->getTypeId())->getIndex(), artPosition);
	}
	for (const auto & art : hero.artifactsInBackpack)
	{
		addArtifactToTable(VLC->arth->getById(art.artifact->getTypeId())->getIndex(), ArtifactPosition::BACKPACK_START);
	}
}

void HeroArtifactsWidget::commitChanges()
{
	while(!hero.artifactsWorn.empty())
	{
		hero.removeArtifact(hero.artifactsWorn.begin()->first);
	}

	while(!hero.artifactsInBackpack.empty())
	{
		hero.removeArtifact(ArtifactPosition::BACKPACK_START + static_cast<int>(hero.artifactsInBackpack.size()) - 1);
	}

	for(const auto & [artPosition, artSlotInfo] : fittingSet.artifactsWorn)
	{
		hero.putArtifact(artPosition, artSlotInfo.artifact);
	}

	for(const auto & art : fittingSet.artifactsInBackpack)
	{
		hero.putArtifact(ArtifactPosition::BACKPACK_START + static_cast<int>(hero.artifactsInBackpack.size()), art.artifact);
	}
}

HeroArtifactsDelegate::HeroArtifactsDelegate(CGHeroInstance & h) : QStyledItemDelegate(), hero(h)
{
}

QWidget * HeroArtifactsDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new HeroArtifactsWidget(hero, parent);
}

void HeroArtifactsDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	if (auto * ed = qobject_cast<HeroArtifactsWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void HeroArtifactsDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	if (auto * ed = qobject_cast<HeroArtifactsWidget *>(editor))
	{
		ed->commitChanges();
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}
