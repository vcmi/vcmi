#include "objectbrowser.h"
#include "../lib/mapObjects/CObjectClassesHandler.h"

ObjectBrowser::ObjectBrowser(QObject *parent)
	: QSortFilterProxyModel{parent}, terrain(Terrain::ANY_TERRAIN)
{
}

bool ObjectBrowser::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
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

	if(terrain == Terrain::ANY_TERRAIN)
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

	result = result & templ->canBePlacedAt(terrain);

	//text filter

	return result;
}

bool ObjectBrowser::filterAcceptsRowText(int source_row, const QModelIndex &source_parent) const
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

