#include "StdInc.h"
#include "cmodlistmodel_moc.h"

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
		"Size",
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

QVariant CModListModel::getValue(const CModEntry &mod, int field) const
{
	switch(field)
	{
		case ModFields::STATUS_ENABLED:
			return mod.getModStatus() & (ModStatus::ENABLED | ModStatus::INSTALLED);

		case ModFields::STATUS_UPDATE:
			return mod.getModStatus() & (ModStatus::UPDATEABLE | ModStatus::INSTALLED);

		default:
			return mod.getValue(ModFields::names[field]);
	}
}

QVariant CModListModel::getText(const CModEntry & mod, int field) const
{
	switch(field)
	{
		case ModFields::STATUS_ENABLED:
		case ModFields::STATUS_UPDATE:
			return "";
		case ModFields::SIZE:
			return CModEntry::sizeToString(getValue(mod, field).toDouble());
		default:
			return getValue(mod, field);
	}
}

QVariant CModListModel::getIcon(const CModEntry & mod, int field) const
{
	if (field == ModFields::STATUS_ENABLED && mod.isEnabled())
		return QIcon(ModStatus::iconEnabled);
	if (field == ModFields::STATUS_ENABLED && mod.isDisabled())
		return QIcon(ModStatus::iconDisabled);

	if (field == ModFields::STATUS_UPDATE  && mod.isUpdateable())
		return QIcon(ModStatus::iconUpdate);
	if (field == ModFields::STATUS_UPDATE  && !mod.isInstalled())
		return QIcon(ModStatus::iconDownload);

	return QVariant();
}

QVariant CModListModel::getTextAlign(int field) const
{
	if (field == ModFields::SIZE)
		return QVariant(Qt::AlignRight | Qt::AlignVCenter);
	else
		return QVariant(Qt::AlignLeft  | Qt::AlignVCenter);
}

QVariant CModListModel::data(const QModelIndex &index, int role) const
{
	if (index.isValid())
	{
		auto mod = getMod(modIndexToName(index.row()));

		switch (role)
		{
			case Qt::DecorationRole:    return getIcon(mod, index.column());
			case Qt::DisplayRole:       return getText(mod, index.column());
			case Qt::UserRole:          return getValue(mod, index.column());
			case Qt::TextAlignmentRole: return getTextAlign(index.column());
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

void CModListModel::resetRepositories()
{
	beginResetModel();
	CModList::resetRepositories();
	endResetModel();
}

void CModListModel::addRepository(QVariantMap data)
{
	beginResetModel();
	CModList::addRepository(data);
	endResetModel();
}

void CModListModel::setLocalModList(QVariantMap data)
{
	beginResetModel();
	CModList::setLocalModList(data);
	endResetModel();
}

void CModListModel::setModSettings(QVariant data)
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

CModFilterModel::CModFilterModel(CModListModel * model, QObject * parent):
    QSortFilterProxyModel(parent),
    base(model),
    filteredType(ModStatus::MASK_NONE),
    filterMask(ModStatus::MASK_NONE)
{
	setSourceModel(model);
	setSortRole(Qt::UserRole);
}
