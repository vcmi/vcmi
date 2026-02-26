/*
 * towneventswidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "../StdInc.h"
#include "towneventswidget.h"
#include "ui_towneventswidget.h"
#include "towneventdialog.h"
#include "mapeditorroles.h"
#include "mapsettings/eventsettings.h"
#include "../../lib/constants/NumericConstants.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/mapping/CCastleEvent.h"

TownEventsWidget::TownEventsWidget(CGTownInstance & town, QWidget * parent) :
	QDialog(parent),
	ui(new Ui::TownEventsWidget),
	town(town)
{
	ui->setupUi(this);
}

TownEventsWidget::~TownEventsWidget()
{
	delete ui;
}

QVariant toVariant(const std::set<BuildingID> & buildings)
{
	QVariantList result;
	for (auto b : buildings)
		result.push_back(QVariant::fromValue(b.num));
	return result;
}

QVariant toVariant(const std::vector<si32> & creatures)
{
	QVariantList result;
	for (auto c : creatures)
		result.push_back(QVariant::fromValue(c));
	return result;
}

std::set<BuildingID> buildingsFromVariant(const QVariant& v)
{
	std::set<BuildingID> result;
	for (const auto & r : v.toList()) {
		result.insert(BuildingID(r.toInt()));
	}
	return result;
}

std::vector<si32> creaturesFromVariant(const QVariant& v)
{
	std::vector<si32> result;
	for (const auto & r : v.toList()) {
		result.push_back(r.toInt());
	}
	return result;
}

QVariant toVariant(const CCastleEvent& event)
{
	QVariantMap result;
	result["name"] = QString::fromStdString(event.name);
	result["message"] = QString::fromStdString(event.message.toString());
	result["players"] = toVariant(event.players);
	result["humanAffected"] = QVariant::fromValue(event.humanAffected);
	result["computerAffected"] = QVariant::fromValue(event.computerAffected);
	result["firstOccurrence"] = QVariant::fromValue(event.firstOccurrence);
	result["nextOccurrence"] = QVariant::fromValue(event.nextOccurrence);
	result["resources"] = toVariant(event.resources);
	result["buildings"] = toVariant(event.buildings);
	result["creatures"] = toVariant(event.creatures);

	return QVariant(result);
}

CCastleEvent eventFromVariant(CMapHeader& map, const CGTownInstance& town, const QVariant& variant)
{
	CCastleEvent result;
	auto v = variant.toMap();
	result.name = v.value("name").toString().toStdString();
	result.message.appendTextID(mapRegisterLocalizedString("map", map, TextIdentifier("town", town.instanceName, "event", result.name, "message"), v.value("message").toString().toStdString()));
	result.players = playersFromVariant(v.value("players"));
	result.humanAffected = v.value("humanAffected").toInt();
	result.computerAffected = v.value("computerAffected").toInt();
	result.firstOccurrence = v.value("firstOccurrence").toInt();
	result.nextOccurrence = v.value("nextOccurrence").toInt();
	result.resources = resourcesFromVariant(v.value("resources"));
	result.buildings = buildingsFromVariant(v.value("buildings"));
	result.creatures = creaturesFromVariant(v.value("creatures"));
	return result;
}

void TownEventsWidget::obtainData()
{
	for (const auto & event : town.events)
	{
		auto eventName = QString::fromStdString(event.name);
		auto itemText = tr("Day %1 - %2").arg(event.firstOccurrence+1, 3).arg(eventName);

		auto * item = new QListWidgetItem(itemText);
		item->setData(MapEditorRoles::TownEventRole, toVariant(event));
		ui->eventsList->addItem(item);
	}
}

void TownEventsWidget::commitChanges(MapController& controller)
{
	town.events.clear();
	for (int i = 0; i < ui->eventsList->count(); ++i)
	{
		const auto * item = ui->eventsList->item(i);
		town.events.push_back(eventFromVariant(*controller.map(), town, item->data(MapEditorRoles::TownEventRole)));
	}
}

void TownEventsWidget::on_timedEventAdd_clicked()
{
	CCastleEvent event;
	event.name = tr("New event").toStdString();
	auto* item = new QListWidgetItem(QString::fromStdString(event.name));
	item->setData(MapEditorRoles::TownEventRole, toVariant(event));
	ui->eventsList->addItem(item);
	on_eventsList_itemActivated(item);
}

void TownEventsWidget::on_timedEventRemove_clicked()
{
	delete ui->eventsList->takeItem(ui->eventsList->currentRow());
}

void TownEventsWidget::on_eventsList_itemActivated(QListWidgetItem* item)
{
	TownEventDialog dlg{ town, item, parentWidget() };
	dlg.exec();
}


TownEventsDelegate::TownEventsDelegate(CGTownInstance & town, MapController & c) : BaseInspectorItemDelegate(), town(town), controller(c)
{
}

QWidget* TownEventsDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new TownEventsWidget(town, parent);
}

void TownEventsDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	if (auto * ed = qobject_cast<TownEventsWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void TownEventsDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	if (auto * ed = qobject_cast<TownEventsWidget *>(editor))
	{
		ed->commitChanges(controller);
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

void TownEventsDelegate::updateModelData(QAbstractItemModel * model, const QModelIndex & index) const
{
	QStringList eventList(QObject::tr("Town Events:"));
	for (const auto & event : town.events)
	{
		auto eventName = QString::fromStdString(event.name);
		eventList += tr("Day %1 - %2").arg(event.firstOccurrence + 1).arg(eventName);
	}
	setModelTextData(model, index, eventList);
}
