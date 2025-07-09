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

std::string joinVector(const std::vector<int>& vec, char delimiter) {
    std::string result;
    for (std::size_t i = 0; i < vec.size(); ++i) {
        result += std::to_string(vec[i]);
        if (i != vec.size() - 1) {
            result += delimiter;
        }
    }
    return result;
}

std::vector<int> splitStringToVector(const std::string& input, char delimiter) {
    std::vector<int> result;
    std::string temp;

    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == delimiter) {
            if (!temp.empty()) {
                result.push_back(std::atoi(temp.c_str()));
                temp.clear();
            }
        } else {
            temp += input[i];
        }
    }

    // Don't forget the last value
    if (!temp.empty()) {
        result.push_back(std::atoi(temp.c_str()));
    }

    return result;
}

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
		{ NOT_LIKE_ZONE, tr("Not like zone (comma seperated)") },
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

		QLineEdit *lineEdit = new QLineEdit;
		QRegularExpression regex("[0-9,]*");
		QRegularExpressionValidator *validator = new QRegularExpressionValidator(regex, lineEdit);
		lineEdit->setValidator(validator);
		lineEdit->setText(QString::fromStdString(joinVector(zones, ',')));
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
	ui->tableWidgetTownHints->setColumnWidth(1, 100);

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
	for (int row = 0; row < ui->tableWidgetTownHints->rowCount() - 1; ++row)
	{
		auto mode = static_cast<modes>(static_cast<QComboBox *>(ui->tableWidgetTownHints->cellWidget(row, 0))->currentData().toInt());
		auto text = static_cast<QLineEdit *>(ui->tableWidgetTownHints->cellWidget(row, 1))->text().toStdString();
		auto values = splitStringToVector(text, ',');
		
		rmg::ZoneOptions::CTownHints hint;
		switch (mode)
		{
		case LIKE_ZONE:
			hint.likeZone = values.at(0);
			break;
		case NOT_LIKE_ZONE:
			hint.notLikeZone = values;
			break;
		case RELATED_TO_ZONE_TERRAIN:
			hint.relatedToZoneTerrain = values.at(0);
			break;
		}
	}

    close();
}

void TownHintSelector::on_buttonBoxResult_rejected()
{
    close();
}