/*
 * cmodlistmodel_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "cmodlistmodel_moc.h"

#include <QIcon>

namespace ModFields
{
static const QString names[ModFields::COUNT] =
{
	"name",
	"",
	"",
	"modType",
	"version",
};

}

namespace ModStatus
{
static const QString iconDelete = "icons:mod-delete.png";
static const QString iconDisabled = "icons:mod-disabled.png";
static const QString iconDownload = "icons:mod-download.png";
static const QString iconEnabled = "icons:mod-enabled.png";
static const QString iconUpdate = "icons:mod-update.png";
}

CModListModel::CModListModel(QObject * parent)
	: QAbstractItemModel(parent)
{
}

QString CModListModel::modIndexToName(const QModelIndex & index) const
{
	if(index.isValid())
	{
		return modNameToID.at(index.internalId());
	}
	return "";
}

QVariant CModListModel::getValue(const CModEntry & mod, int field) const
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
	default:
		return getValue(mod, field);
	}
}

QVariant CModListModel::getIcon(const CModEntry & mod, int field) const
{
	if(field == ModFields::STATUS_ENABLED && mod.isEnabled())
		return QIcon(ModStatus::iconEnabled);
	if(field == ModFields::STATUS_ENABLED && mod.isDisabled())
		return QIcon(ModStatus::iconDisabled);

	if(field == ModFields::STATUS_UPDATE && mod.isUpdateable())
		return QIcon(ModStatus::iconUpdate);
	if(field == ModFields::STATUS_UPDATE && !mod.isInstalled())
		return QIcon(ModStatus::iconDownload);

	return {};
}

QVariant CModListModel::getTextAlign(int field) const
{
	return {Qt::AlignLeft | Qt::AlignVCenter};
}

QVariant CModListModel::data(const QModelIndex & index, int role) const
{
	if(index.isValid())
	{
		auto mod = getMod(modIndexToName(index));

		switch(role)
		{
		case Qt::DecorationRole:
			return getIcon(mod, index.column());
		case Qt::DisplayRole:
			return getText(mod, index.column());
		case Qt::TextAlignmentRole:
			return getTextAlign(index.column());
		case ModRoles::ValueRole:
			return getValue(mod, index.column());
		case ModRoles::ModNameRole:
			return mod.getName();
		}
	}
	return {};
}

int CModListModel::rowCount(const QModelIndex & index) const
{
	if(index.isValid())
		return modIndex[modIndexToName(index)].size();
	return modIndex[""].size();
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
	static const QString header[ModFields::COUNT] =
	{
		QT_TR_NOOP("Name"),
		QT_TR_NOOP(""), // status icon
		QT_TR_NOOP(""), // status icon
		QT_TR_NOOP("Type"),
		QT_TR_NOOP("Version"),
	};

	if(role == Qt::DisplayRole && orientation == Qt::Horizontal)
		return QCoreApplication::translate("ModFields", header[section].toStdString().c_str());
	return {};
}

void CModListModel::reloadRepositories()
{
	beginResetModel();
	endResetModel();
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

void CModListModel::modChanged(QString modID)
{
	int index = modNameToID.indexOf(modID);
	QModelIndex parent = this->parent(createIndex(0, 0, index));
	int row = modIndex[modIndexToName(parent)].indexOf(modID);
	emit dataChanged(createIndex(row, 0, index), createIndex(row, 4, index));
}

void CModListModel::endResetModel()
{
	modNameToID = getModList();
	modIndex.clear();
	for(const QString & str : modNameToID)
	{
		if(str.contains('.'))
		{
			modIndex[str.section('.', 0, -2)].append(str);
		}
		else
		{
			modIndex[""].append(str);
		}
	}
	QAbstractItemModel::endResetModel();
}

QModelIndex CModListModel::index(int row, int column, const QModelIndex & parent) const
{
	if(parent.isValid())
	{
		if(modIndex[modIndexToName(parent)].size() > row)
			return createIndex(row, column, modNameToID.indexOf(modIndex[modIndexToName(parent)][row]));
	}
	else
	{
		if(modIndex[""].size() > row)
			return createIndex(row, column, modNameToID.indexOf(modIndex[""][row]));
	}
	return {};
}

QModelIndex CModListModel::parent(const QModelIndex & child) const
{
	QString modID = modNameToID[child.internalId()];
	for(auto entry = modIndex.begin(); entry != modIndex.end(); entry++) // because using range-for entry type is QMap::value_type oO
	{
		if(entry.key() != "" && entry.value().indexOf(modID) != -1)
		{
			return createIndex(entry.value().indexOf(modID), child.column(), modNameToID.indexOf(entry.key()));
		}
	}
	return {};
}

void CModFilterModel::setTypeFilter(int filteredType, int filterMask)
{
	this->filterMask = filterMask;
	this->filteredType = filteredType;
	invalidateFilter();
}

bool CModFilterModel::filterMatchesThis(const QModelIndex & source) const
{
	CModEntry mod = base->getMod(source.data(ModRoles::ModNameRole).toString());
	return (mod.getModStatus() & filterMask) == filteredType &&
			mod.isValid() &&
	       QSortFilterProxyModel::filterAcceptsRow(source.row(), source.parent());
}

bool CModFilterModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
	QModelIndex index = base->index(source_row, 0, source_parent);

	if(filterMatchesThis(index))
	{
		return true;
	}

	for(size_t i = 0; i < base->rowCount(index); i++)
	{
		if(filterMatchesThis(base->index((int)i, 0, index)))
			return true;
	}

	QModelIndex parent = source_parent;
	while(parent.isValid())
	{
		if(filterMatchesThis(parent))
			return true;
		parent = parent.parent();
	}
	return false;
}

CModFilterModel::CModFilterModel(CModListModel * model, QObject * parent)
	: QSortFilterProxyModel(parent), base(model), filteredType(ModStatus::MASK_NONE), filterMask(ModStatus::MASK_NONE)
{
	setSourceModel(model);
	setSortRole(ModRoles::ValueRole);
}
