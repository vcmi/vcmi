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
#include "helper.h"
#include "../lib/GameLibrary.h"
#include "../lib/MapLayerHandler.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>
#include <QTableWidget>
#include <QDialogButtonBox>

MapLayerSelectionDialog::MapLayerSelectionDialog(int levelCount, const std::vector<MapLayerId> & currentLayers, QWidget *parent)
	: QDialog(parent)
	, levelCount(levelCount)
{
	setWindowTitle(tr("Map Layer Configuration"));
	setMinimumWidth(420);
	resize(420, 400);

	Helper::decorateDialog(this);

	auto *mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(6);

	// Table with level rows
	table = new QTableWidget(levelCount, 2, this);
	table->setHorizontalHeaderLabels({tr("Level"), tr("Map Layer")});
	table->horizontalHeader()->setStretchLastSection(true);
	table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	table->verticalHeader()->hide();
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setSelectionMode(QAbstractItemView::NoSelection);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);

	// Build combo boxes for each level
	auto & layers = LIBRARY->mapLayerHandler->objects;

	for (int i = 0; i < levelCount; i++)
	{
		// Level label (read-only)
		auto *levelItem = new QTableWidgetItem(tr("Level %1").arg(i + 1));
		levelItem->setFlags(levelItem->flags() & ~Qt::ItemIsEditable);
		table->setItem(i, 0, levelItem);

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

		table->setCellWidget(i, 1, combo);
		table->setRowHeight(i, 28);
	}

	mainLayout->addWidget(table, 1);

	buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
	mainLayout->addWidget(buttonBox);
}

MapLayerSelectionDialog::~MapLayerSelectionDialog() = default;

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
