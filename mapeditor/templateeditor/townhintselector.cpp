/*
 * townhintselector.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "townhintselector.h"
#include "ui_townhintselector.h"

enum modes { UNKNOWN, LIKE_ZONE, NOT_LIKE_ZONE, RELATED_TO_ZONE_TERRAIN };

TownHintSelector::TownHintSelector(std::vector<rmg::ZoneOptions::CTownHints> & townHints) :
	ui(new Ui::TownHintSelector),
	townHints(townHints)
{
	ui->setupUi(this);

	setWindowTitle(tr("Town hint Selector"));
	
	setWindowModality(Qt::ApplicationModal);

	ui->tableWidgetTownHints->setColumnCount(3);
	ui->tableWidgetTownHints->setRowCount(townHints.size() + 1);
	ui->tableWidgetTownHints->setHorizontalHeaderLabels({tr("Type"), tr("Value"), tr("Action")});
	
	std::map<modes, QString> values = {
		{ LIKE_ZONE, tr("Like Zone") },
		{ NOT_LIKE_ZONE, tr("Not like zone (comma separated)") },
		{ RELATED_TO_ZONE_TERRAIN, tr("Related to zone terrain") }
	};

	auto addRow = [this, values](int mode, std::vector<TRmgTemplateZoneId> zones, int row){
		QComboBox *combo = new QComboBox();
		for(auto & item : values)
    		combo->addItem(item.second, QVariant(static_cast<int>(item.first)));

		int index = combo->findData(static_cast<int>(mode));
		if (index != -1)
			combo->setCurrentIndex(index);

		ui->tableWidgetTownHints->setCellWidget(row, 0, combo);

		std::vector<std::string> values(zones.size());
		std::transform(zones.begin(), zones.end(), values.begin(), [](const int val) { return std::to_string(val); });
		std::string valuesText = boost::algorithm::join(values, ",");

		QLineEdit *lineEdit = new QLineEdit;
		QRegularExpression regex("^\\d+(,\\d+)*$");
		QRegularExpressionValidator *validator = new QRegularExpressionValidator(regex, lineEdit);
		lineEdit->setValidator(validator);
		lineEdit->setText(QString::fromStdString(valuesText));
		ui->tableWidgetTownHints->setCellWidget(row, 1, lineEdit);

		auto deleteButton = new QPushButton(tr("Delete"));
		ui->tableWidgetTownHints->setCellWidget(row, 2, deleteButton);
		connect(deleteButton, &QPushButton::clicked, this, [this, deleteButton]() {
			for (int r = 0; r < ui->tableWidgetTownHints->rowCount(); ++r) {
				if (ui->tableWidgetTownHints->cellWidget(r, 2) == deleteButton) {
					ui->tableWidgetTownHints->removeRow(r);
					break;
				}
			}
		});
	};

	for (int row = 0; row < townHints.size(); ++row)
	{
		int mode = UNKNOWN;
		std::vector<TRmgTemplateZoneId> zones;
		if(townHints[row].likeZone != rmg::ZoneOptions::NO_ZONE)
		{
			mode = LIKE_ZONE;
			zones = { townHints[row].likeZone };
		}
		else if(!townHints[row].notLikeZone.empty())
		{
			mode = NOT_LIKE_ZONE;
			zones = townHints[row].notLikeZone;
			
		}
		else if(townHints[row].relatedToZoneTerrain != rmg::ZoneOptions::NO_ZONE)
		{
			mode = RELATED_TO_ZONE_TERRAIN;
			zones = { townHints[row].relatedToZoneTerrain };
			
		}

		assert(mode != UNKNOWN);

		addRow(mode, zones, row);
	}

	auto addButton = new QPushButton("Add");
	ui->tableWidgetTownHints->setCellWidget(ui->tableWidgetTownHints->rowCount() - 1, 2, addButton);
	connect(addButton, &QPushButton::clicked, this, [this, addRow]() {
		ui->tableWidgetTownHints->insertRow(ui->tableWidgetTownHints->rowCount() - 1);
		addRow(LIKE_ZONE, std::vector<TRmgTemplateZoneId>({ 0 }), ui->tableWidgetTownHints->rowCount() - 2);
	});

	ui->tableWidgetTownHints->resizeColumnsToContents();
	ui->tableWidgetTownHints->setColumnWidth(0, 300);
	ui->tableWidgetTownHints->setColumnWidth(1, 150);

	show();
}

void TownHintSelector::showTownHintSelector(std::vector<rmg::ZoneOptions::CTownHints> & townHints)
{
	auto * dialog = new TownHintSelector(townHints);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->exec();
}

void TownHintSelector::on_buttonBoxResult_accepted()
{
	townHints.clear();
	for (int row = 0; row < ui->tableWidgetTownHints->rowCount() - 1; ++row) // iterate over all rows except the add button row
	{
		auto mode = static_cast<modes>(static_cast<QComboBox *>(ui->tableWidgetTownHints->cellWidget(row, 0))->currentData().toInt());
		auto text = static_cast<QLineEdit *>(ui->tableWidgetTownHints->cellWidget(row, 1))->text().toStdString();
		std::vector<std::string> values;
		boost::split(values, text, boost::is_any_of(","));
		if (!values.empty() && values.back().empty()) // remove "no number" after last comma; other cases are covered by regex
    		values.pop_back();
		std::vector<int> numValues(values.size());
		std::transform(values.begin(), values.end(), numValues.begin(), [](const std::string& str) { return std::stoi(str); });
		
		rmg::ZoneOptions::CTownHints hint;
		switch (mode)
		{
		case LIKE_ZONE:
			hint.likeZone = numValues.at(0);
			break;
		case NOT_LIKE_ZONE:
			hint.notLikeZone = numValues;
			break;
		case RELATED_TO_ZONE_TERRAIN:
			hint.relatedToZoneTerrain = numValues.at(0);
			break;
		}
		townHints.push_back(hint);
	}

    close();
}

void TownHintSelector::on_buttonBoxResult_rejected()
{
    close();
}