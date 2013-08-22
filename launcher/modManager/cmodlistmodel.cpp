#include "StdInc.h"
#include "cmodlistmodel.h"

#include <QIcon>

namespace ModFields
{
	static const QString names [ModFields::COUNT] =
	{
		"",
		"",
		"modType",
		"name",
		"version",
		"size",
		"author"
	};

	static const QString header [ModFields::COUNT] =
	{
		"", // status icon
		"", // status icon
		"Type",
		"Name",
		"Version",
		"Size (KB)",
		"Author"
	};
}

namespace ModStatus
{
	static const QString iconDelete   = "icons:mod-delete.png";
	static const QString iconDisabled = "icons:mod-disabled.png";
	static const QString iconDownload = "icons:mod-download.png";
	static const QString iconEnabled  = "icons:mod-enabled.png";
	static const QString iconUpdate   = "icons:mod-update.png";
}

CModListModel::CModListModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

QString CModListModel::modIndexToName(int index) const
{
	return indexToName[index];
}

QVariant CModListModel::data(const QModelIndex &index, int role) const
{
	if (index.isValid())
	{
		auto mod = getMod(modIndexToName(index.row()));

		if (index.column() == ModFields::STATUS_ENABLED)
		{
			if (role == Qt::DecorationRole)
			{
				if (mod.isEnabled())
					return QIcon(ModStatus::iconEnabled);

				if (mod.isDisabled())
					return QIcon(ModStatus::iconDisabled);

				return QVariant();
			}
		}
		if (index.column() == ModFields::STATUS_UPDATE)
		{
			if (role == Qt::DecorationRole)
			{
				if (mod.isUpdateable())
					return QIcon(ModStatus::iconUpdate);

				if (!mod.isInstalled())
					return QIcon(ModStatus::iconDownload);

				return QVariant();
			}
		}

		if (role == Qt::DisplayRole)
		{
			return mod.getValue(ModFields::names[index.column()]);
		}
	}
	return QVariant();
}

int CModListModel::rowCount(const QModelIndex &) const
{
	return indexToName.size();
}

int CModListModel::columnCount(const QModelIndex &) const
{
	return ModFields::COUNT;
}

Qt::ItemFlags CModListModel::flags(const QModelIndex &) const
{
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant CModListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
		return ModFields::header[section];
	return QVariant();
}

void CModListModel::addRepository(QJsonObject data)
{
	beginResetModel();
	CModList::addRepository(data);
	endResetModel();
}

void CModListModel::setLocalModList(QJsonObject data)
{
	beginResetModel();
	CModList::setLocalModList(data);
	endResetModel();
}

void CModListModel::setModSettings(QJsonObject data)
{
	beginResetModel();
	CModList::setModSettings(data);
	endResetModel();
}

void CModListModel::endResetModel()
{
	indexToName = getModList();
	QAbstractItemModel::endResetModel();
}

void CModFilterModel::setTypeFilter(int filteredType, int filterMask)
{
	this->filterMask = filterMask;
	this->filteredType = filteredType;
	invalidateFilter();
}

bool CModFilterModel::filterMatches(int modIndex) const
{
	CModEntry mod = base->getMod(base->modIndexToName(modIndex));

	return (mod.getModStatus() & filterMask) == filteredType;
}

bool CModFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	if (filterMatches(source_row))
		return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
	return false;
}

bool CModFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	assert(left.column() == right.column());

	CModEntry mod = base->getMod(base->modIndexToName(left.row()));

	switch (left.column())
	{
		case ModFields::STATUS_ENABLED:
		{
			return (mod.getModStatus() & (ModStatus::ENABLED | ModStatus::INSTALLED))
			     < (mod.getModStatus() & (ModStatus::ENABLED | ModStatus::INSTALLED));
		}
		case ModFields::STATUS_UPDATE:
		{
			return (mod.getModStatus() & (ModStatus::UPDATEABLE | ModStatus::INSTALLED))
			     < (mod.getModStatus() & (ModStatus::UPDATEABLE | ModStatus::INSTALLED));
		}
		default:
		{
			return QSortFilterProxyModel::lessThan(left, right);
		}
	}
}

CModFilterModel::CModFilterModel(CModListModel * model, QObject * parent):
    QSortFilterProxyModel(parent),
    base(model),
    filteredType(ModStatus::MASK_NONE),
    filterMask(ModStatus::MASK_NONE)
{
	setSourceModel(model);
}
