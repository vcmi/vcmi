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
#include "../mapcontroller.h"

#include "../../lib/constants/StringConstants.h"
#include "../../lib/entities/artifact/ArtifactUtils.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/mapping/CMap.h"

HeroArtifactsWidget::HeroArtifactsWidget(MapController & controller, CGHeroInstance & h, QWidget * parent) :
	QDialog(parent),
	ui(new Ui::HeroArtifactsWidget),
	controller(controller),
	hero(h),
	fittingSet(CArtifactFittingSet(h))
{
	ui->setupUi(this);

	connect(ui->saveButton, &QPushButton::clicked, this, &HeroArtifactsWidget::onSaveButtonClicked);
	connect(ui->cancelButton, &QPushButton::clicked, this, &HeroArtifactsWidget::onCancelButtonClicked);
}

HeroArtifactsWidget::~HeroArtifactsWidget()
{
	delete ui;
}

void HeroArtifactsWidget::on_addButton_clicked()
{
	auto * artifactWidget = new ArtifactWidget(fittingSet, this);
	connect(artifactWidget, &ArtifactWidget::saveArtifact, this, &HeroArtifactsWidget::onSaveArtifact);
	artifactWidget->open();
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

void HeroArtifactsWidget::onSaveButtonClicked()
{
	accept();
}

void HeroArtifactsWidget::onCancelButtonClicked()
{
	reject();
}

void HeroArtifactsWidget::onSaveArtifact(int32_t artifactIndex, ArtifactPosition slot) 
{
	auto artifact = controller.map()->createArtifact(LIBRARY->arth->getByIndex(artifactIndex)->getId());
	fittingSet.putArtifact(slot, artifact);
	addArtifactToTable(artifactIndex, slot);
}

void HeroArtifactsWidget::addArtifactToTable(int32_t artifactIndex, ArtifactPosition slot)
{
	auto artifact = LIBRARY->arth->getByIndex(artifactIndex);
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
		addArtifactToTable(LIBRARY->arth->getById(artSlotInfo.getArt()->getTypeId())->getIndex(), artPosition);
	}
	for (const auto & art : hero.artifactsInBackpack)
	{
		addArtifactToTable(LIBRARY->arth->getById(art.getArt()->getTypeId())->getIndex(), ArtifactPosition::BACKPACK_START);
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
		hero.putArtifact(artPosition, artSlotInfo.getArt());
	}

	for(const auto & art : fittingSet.artifactsInBackpack)
	{
		hero.putArtifact(ArtifactPosition::BACKPACK_START + static_cast<int>(hero.artifactsInBackpack.size()), art.getArt());
	}
}

HeroArtifactsDelegate::HeroArtifactsDelegate(MapController & controller, CGHeroInstance & h)
	: BaseInspectorItemDelegate()
	, controller(controller)
	, hero(h)
{
}

QWidget * HeroArtifactsDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new HeroArtifactsWidget(controller, hero, parent);
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
		if(ed->result() == QDialog::Accepted)
		{
			ed->commitChanges();
			updateModelData(model, index);
		}
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

void HeroArtifactsDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList textList;
	for(const auto & [artPosition, artSlotInfo] : hero.artifactsWorn)
	{
		auto slotText = NArtifactPosition::namesHero[artPosition.num];

		textList += QString("%1: %2").arg(QString::fromStdString(slotText)).arg(QString::fromStdString(artSlotInfo.getArt()->getType()->getNameTranslated()));
	}
	textList += QString("%1:").arg(QString::fromStdString(NArtifactPosition::backpack));
	for(const auto & art : hero.artifactsInBackpack)
	{
		textList += QString::fromStdString(art.getArt()->getType()->getNameTranslated());
	}
	setModelTextData(model, index, textList);
}
