/*
 * rumorsettings.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "abstractsettings.h"

namespace Ui {
class RumorSettings;
}

class RumorSettings : public AbstractSettings
{
	Q_OBJECT

public:
	explicit RumorSettings(QWidget *parent = nullptr);
	~RumorSettings();

	void initialize(MapController & map) override;
	void update() override;

private slots:
	void on_message_textChanged();

	void on_add_clicked();

	void on_remove_clicked();

	void on_rumors_itemSelectionChanged();

private:
	Ui::RumorSettings *ui;
};
