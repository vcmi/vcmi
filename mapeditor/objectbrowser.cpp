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
#include "mapeditorroles.h"

#include "../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/TerrainHandler.h"
#include "../lib/GameLibrary.h"

ObjectBrowserProxyModel::ObjectBrowserProxyModel(QObject *parent)
	: QSortFilterProxyModel{parent}, terrain(ETerrainId::ANY_TERRAIN)
{
}

static bool fitsAnyLand(const AObjectTypeHandler & factory, const std::shared_ptr<const ObjectTemplate> & templ)
{
	//H3 templates of objects without terrain graphics have placeholder masks, JSON adds an any-land template with the same graphics
	for(const auto & other : factory.getTemplates())
		if(other->canBePlacedAtAnyTerrain() && other->animationFile == templ->animationFile)
			return true;

	//anyLandTerrain is false for templates that also allow water, such as Event; getTemplates(TerrainId) ignores the H3 masks of artifacts and monsters
	for(const auto & terrain : LIBRARY->terrainTypeHandler->objects)
		if(terrain->isLand() && terrain->isPassable() && !vstd::contains(factory.getTemplates(terrain->getId()), templ))
			return false;

	return true;
}

bool ObjectBrowserProxyModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
	auto * standardModel = qobject_cast<QStandardItemModel*>(sourceModel());
	assert(standardModel);

	QModelIndex currentIndex = sourceModel()->index(source_row, 0, source_parent);
	auto item = standardModel->itemFromIndex(currentIndex);
	if(!item)
		return false;

	if(!filterAcceptsRowText(source_row, source_parent))
		return false;

	if(!filter.isEmpty())
		return true;

	bool inCategory = item->data(ObjectGroupRole).toString() == category;
	if(terrain == ETerrainId::ANY_TERRAIN)
		return inCategory;

	auto data = item->data().toJsonObject();
	auto objId = data["id"].toInt();
	auto objSubId = data["subid"].toInt();
	auto templateId = data["template"].toInt();

	auto factory = LIBRARY->objtypeh->getHandlerFor(objId, objSubId);
	auto templ = factory->getTemplates().at(templateId);

	return templ->canBePlacedAt(terrain) && (inCategory || !fitsAnyLand(*factory, templ));
}

bool ObjectBrowserProxyModel::filterAcceptsRowText(int source_row, const QModelIndex &source_parent) const
{
	QModelIndex index = sourceModel()->index(source_row, 0 ,source_parent);
	if(!index.isValid())
		return false;

	auto * standardModel = qobject_cast<QStandardItemModel*>(sourceModel());
	assert(standardModel);

	auto item = standardModel->itemFromIndex(index);
	if(!item)
		return false;
	
	auto data = item->data().toJsonObject();

	return (filter.isEmpty()
			|| item->text().contains(filter, Qt::CaseInsensitive)
			|| data["typeName"].toString().contains(filter, Qt::CaseInsensitive));
}

//catalog order instead of names keeps the monsters of each faction together
bool ObjectBrowserProxyModel::lessThan(const QModelIndex & left, const QModelIndex & right) const
{
	bool leftInCategory = left.data(ObjectGroupRole).toString() == category;
	bool rightInCategory = right.data(ObjectGroupRole).toString() == category;
	if(leftInCategory != rightInCategory)
		return leftInCategory;

	return left.row() < right.row();
}

Qt::ItemFlags ObjectBrowserProxyModel::flags(const QModelIndex & index) const
{
	Qt::ItemFlags defaultFlags = QSortFilterProxyModel::flags(index) & ~Qt::ItemIsEditable;

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

class ObjectIconDelegate : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;

protected:
	void initStyleOption(QStyleOptionViewItem * option, const QModelIndex & index) const override
	{
		QStyledItemDelegate::initStyleOption(option, index);
		const auto * view = qobject_cast<const QListView *>(option->widget);
		if(view && view->viewMode() == QListView::IconMode)
		{
			option->text.clear();
			option->features.setFlag(QStyleOptionViewItem::HasDisplay, false);
		}
	}
};

ObjectBrowser::ObjectBrowser(QWidget * parent):
	QListView(parent)
{
	setItemDelegate(new ObjectIconDelegate(this));
	setDropIndicatorShown(false);
	setUniformItemSizes(true);
	setGridMode(true);
}

void ObjectBrowser::setGridMode(bool grid)
{
	setViewMode(grid ? IconMode : ListMode);
	setFlow(grid ? LeftToRight : TopToBottom);
	setWrapping(grid);
	setIconSize(grid ? QSize(64, 64) : QSize(32, 32));
	setGridSize(grid ? QSize(68, 68) : QSize());
	verticalScrollBar()->setSingleStep(grid ? gridSize().height() : 1); //in IconMode the automatic step from Qt scrolls only a few pixels per wheel notch
	setMovement(Static); //IconMode defaults to Free, which lets items be dragged around inside the view
	setResizeMode(Adjust);
	setDragDropMode(DragOnly); //setMovement(Static) disables dragging, objects are dragged out onto the map
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
		
	auto *drag = new QDrag(this);
	drag->setMimeData(mimeData);
	drag->exec(supportedActions);
}

ObjectCategoryList::ObjectCategoryList(QWidget * parent):
	QListWidget(parent)
{
	setViewMode(IconMode);
	setFlow(LeftToRight);
	setWrapping(true);
	setMovement(Static);
	setResizeMode(Adjust);
	setIconSize(QSize(32, 32));
	setGridSize(QSize(36, 36));
	setSelectionMode(SingleSelection);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void ObjectCategoryList::fitHeight()
{
	doItemsLayout();
	setFixedHeight(contentsSize().height() + 2 * frameWidth());
}

void ObjectCategoryList::resizeEvent(QResizeEvent * event)
{
	QListWidget::resizeEvent(event);
	fitHeight();
}
