/*
 * objectbrowser.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "objectbrowser.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"

ObjectBrowserProxyModel::ObjectBrowserProxyModel(QObject *parent)
	: QSortFilterProxyModel{parent}, terrain(TerrainId::ANY_TERRAIN)
{
}

bool ObjectBrowserProxyModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
	bool result = QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);

	QModelIndex currentIndex = sourceModel()->index(source_row, 0, source_parent);
	int childCount = currentIndex.model()->rowCount(currentIndex);
	if(childCount)
		return false;

	auto item = dynamic_cast<QStandardItemModel*>(sourceModel())->itemFromIndex(currentIndex);
	if(!item)
		return result;

	if(!filterAcceptsRowText(source_row, source_parent))
		return false;

	if(terrain == TerrainId::ANY_TERRAIN)
		return result;

	auto data = item->data().toJsonObject();
	if(data.empty())
		return result;

	auto objIdJson = data["id"];
	if(objIdJson == QJsonValue::Undefined)
		return result;

	auto objId = data["id"].toInt();
	auto objSubId = data["subid"].toInt();
	auto templateId = data["template"].toInt();

	auto factory = VLC->objtypeh->getHandlerFor(objId, objSubId);
	auto templ = factory->getTemplates()[templateId];

	result = result && templ->canBePlacedAt(terrain);

	//if we are here, just text filter will be applied
	return result;
}

bool ObjectBrowserProxyModel::filterAcceptsRowText(int source_row, const QModelIndex &source_parent) const
{
	if(source_parent.isValid())
	{
		if(filterAcceptsRowText(source_parent.row(), source_parent.parent()))
			return true;
	}

	QModelIndex index = sourceModel()->index(source_row, 0 ,source_parent);
	if(!index.isValid())
		return false;

	auto item = dynamic_cast<QStandardItemModel*>(sourceModel())->itemFromIndex(index);
	if(!item)
		return false;

	return (filter.isNull() || filter.isEmpty() || item->text().contains(filter, Qt::CaseInsensitive));
}

Qt::ItemFlags ObjectBrowserProxyModel::flags(const QModelIndex & index) const
{
	Qt::ItemFlags defaultFlags = QSortFilterProxyModel::flags(index);

	if (index.isValid())
		return Qt::ItemIsDragEnabled | defaultFlags;
	
	return defaultFlags;
}

QMimeData * ObjectBrowserProxyModel::mimeData(const QModelIndexList & indexes) const
{
	assert(indexes.size() == 1);
	
	auto * standardModel = qobject_cast<QStandardItemModel*>(sourceModel());
	assert(standardModel);
	
	QModelIndex index = indexes.front();
	
	if(!index.isValid())
		return nullptr;

	QMimeData * mimeData = new QMimeData;
	mimeData->setImageData(standardModel->itemFromIndex(mapToSource(index))->data());
	return mimeData;
}

ObjectBrowser::ObjectBrowser(QWidget * parent):
	QTreeView(parent)
{
	setDropIndicatorShown(false);
}

void ObjectBrowser::startDrag(Qt::DropActions supportedActions)
{
	logGlobal->info("Drag'n'drop: Start dragging object from ObjectBrowser");
	auto indexes = selectedIndexes();
	if(indexes.isEmpty())
		return;
	
	QMimeData * mimeData = model()->mimeData(indexes);
	if(!mimeData)
		return;
		
	QDrag *drag = new QDrag(this);
	drag->setMimeData(mimeData);
	drag->exec(supportedActions);
}
