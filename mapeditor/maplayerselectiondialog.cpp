/*
 * maplayerselectiondialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "maplayerselectiondialog.h"
#include "ui_maplayerselectiondialog.h"
#include "helper.h"
#include "../lib/GameLibrary.h"
#include "../lib/MapLayerHandler.h"

#include <QHeaderView>
#include <QComboBox>
#include <QTableWidget>
#include <QDialogButtonBox>

MapLayerSelectionDialog::MapLayerSelectionDialog(int levelCount, const std::vector<MapLayerId> & currentLayers, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::MapLayerSelectionDialog)
	, levelCount(levelCount)
{
	ui->setupUi(this);
	setWindowTitle(tr("Map Layer Configuration"));

	Helper::decorateDialog(this);

	ui->table->setColumnCount(2);
	ui->table->setRowCount(levelCount);
	ui->table->setHorizontalHeaderLabels({tr("Level"), tr("Map Layer")});
	ui->table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	// Build combo boxes for each level
	auto & layers = LIBRARY->mapLayerHandler->objects;

	for (int i = 0; i < levelCount; i++)
	{
		// Level label (read-only)
		auto *levelItem = new QTableWidgetItem(tr("Level %1").arg(i + 1));
		levelItem->setFlags(levelItem->flags() & ~Qt::ItemIsEditable);
		ui->table->setItem(i, 0, levelItem);

		// Combo for layer selection
		auto *combo = new QComboBox();
		layerCombos.push_back(combo);

		for (size_t j = 0; j < layers.size(); j++)
		{
			const auto *layer = layers[j].get();
			combo->addItem(QString::fromStdString(layer->getNameTranslated()), QVariant(static_cast<int>(j)));
		}

		// Set current value
		if (i < static_cast<int>(currentLayers.size()))
		{
			int idx = currentLayers[i].getNum();
			for (int j = 0; j < combo->count(); j++)
			{
				if (combo->itemData(j).toInt() == idx)
				{
					combo->setCurrentIndex(j);
					break;
				}
			}
		}

		ui->table->setCellWidget(i, 1, combo);
		ui->table->setRowHeight(i, 28);
	}
}

MapLayerSelectionDialog::~MapLayerSelectionDialog()
{
	delete ui;
}

std::vector<MapLayerId> MapLayerSelectionDialog::getSelectedLayers() const
{
	std::vector<MapLayerId> result;
	result.reserve(layerCombos.size());
	for (auto *combo : layerCombos)
	{
		int idx = combo->currentData().toInt();
		result.emplace_back(MapLayerId(idx));
	}
	return result;
}
