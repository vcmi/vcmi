/*
 * eventsettings.h, part of VCMI engine
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
class EventSettings;
}

QVariant toVariant(const TResources & resources);
QVariant toVariant(const std::set<PlayerColor> & players);

TResources resourcesFromVariant(const QVariant & v);
std::set<PlayerColor> playersFromVariant(const QVariant & v);

class EventSettings : public AbstractSettings
{
	Q_OBJECT

public:
	explicit EventSettings(QWidget *parent = nullptr);
	~EventSettings();

	void initialize(MapController & map) override;
	void update() override;

private slots:
	void on_timedEventAdd_clicked();

	void on_timedEventRemove_clicked();

	void on_eventsList_itemActivated(QListWidgetItem *item);

private:
	Ui::EventSettings *ui;
};

