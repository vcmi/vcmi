/*
 * modstateview_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "modstateitemmodel_moc.h"

#include "modstatemodel.h"

#include <QIcon>

namespace ModStatus
{
static const QString iconDelete = ":/icons/mod-delete.png";
static const QString iconDisabled = ":/icons/mod-disabled.png";
static const QString iconDisabledSubmod = ":/icons/submod-disabled.png";
static const QString iconDownload = ":/icons/mod-download.png";
static const QString iconEnabled = ":/icons/mod-enabled.png";
static const QString iconEnabledSubmod = ":/icons/submod-enabled.png";
static const QString iconUpdate = ":/icons/mod-update.png";
}

ModStateItemModel::ModStateItemModel(std::shared_ptr<ModStateModel> model, QObject * parent)
	: QAbstractItemModel(parent)
	, model(model)
{
}

QString ModStateItemModel::modIndexToName(const QModelIndex & index) const
{
	if(index.isValid())
	{
		return modNameToID.at(index.internalId());
	}
	return "";
}


QString ModStateItemModel::modTypeName(QString modTypeID) const
{
	static const QMap<QString, QString> modTypes = {
		{"Translation", tr("Translation")},
		{"Town",        tr("Town")       },
		{"Test",        tr("Test")       },
		{"Templates",   tr("Templates")  },
		{"Spells",      tr("Spells")     },
		{"Music",       tr("Music")      },
		{"Maps",        tr("Maps")       },
		{"Sounds",      tr("Sounds")     },
		{"Skills",      tr("Skills")     },
		{"Other",       tr("Other")      },
		{"Objects",     tr("Objects")    },
		{"Mechanical",  tr("Mechanics")  },
		{"Mechanics",   tr("Mechanics")  },
		{"Themes",      tr("Interface")  },
		{"Interface",   tr("Interface")  },
		{"Heroes",      tr("Heroes")     },
		{"Graphic",     tr("Graphical")  },
		{"Graphical",   tr("Graphical")  },
		{"Expansion",   tr("Expansion")  },
		{"Creatures",   tr("Creatures")  },
		{"Compatibility", tr("Compatibility") },
		{"Artifacts",   tr("Artifacts")  },
		{"AI",          tr("AI")         },
	};

	if (modTypes.contains(modTypeID))
		return modTypes[modTypeID];
	return tr("Other");
}

QVariant ModStateItemModel::getValue(const ModState & mod, int field) const
{
	switch(field)
	{
		case ModFields::STATUS_ENABLED:
			return model->isModEnabled(mod.getID());

		case ModFields::STATUS_UPDATE:
			return model->isModUpdateAvailable(mod.getID());

		case ModFields::NAME:
			return mod.getName();

		case ModFields::TYPE:
			return modTypeName(mod.getType());

		default:
			return QVariant();
	}
}

QVariant ModStateItemModel::getText(const ModState & mod, int field) const
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

QVariant ModStateItemModel::getIcon(const ModState & mod, int field) const
{
	if (field == ModFields::STATUS_ENABLED)
	{
		if (!model->isModInstalled(mod.getID()))
			return QVariant();

		if(mod.isSubmod())
		{
			if (!model->isModEnabled(mod.getTopParentID()))
			{
				if (model->isModEnabled(mod.getID()))
					return QIcon(ModStatus::iconEnabledSubmod);
				else
					return QIcon(ModStatus::iconDisabledSubmod);
			}
		}

		if (model->isModEnabled(mod.getID()))
			return QIcon(ModStatus::iconEnabled);
		else
			return QIcon(ModStatus::iconDisabled);
	}

	if(field == ModFields::STATUS_UPDATE)
	{
		if (model->isModUpdateAvailable(mod.getID()))
			return QIcon(ModStatus::iconUpdate);
		if (!model->isModInstalled(mod.getID()))
			return QIcon(ModStatus::iconDownload);
	}

	return QVariant();
}

QVariant ModStateItemModel::getTextAlign(int field) const
{
	return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
}

QVariant ModStateItemModel::data(const QModelIndex & index, int role) const
{
	if(index.isValid())
	{
		auto mod = model->getMod(modIndexToName(index));

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
			return mod.getID();
		}
	}
	return QVariant();
}

int ModStateItemModel::rowCount(const QModelIndex & index) const
{
	if(index.isValid())
		return modIndex[modIndexToName(index)].size();
	return modIndex[""].size();
}

int ModStateItemModel::columnCount(const QModelIndex &) const
{
	return ModFields::COUNT;
}

Qt::ItemFlags ModStateItemModel::flags(const QModelIndex &) const
{
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant ModStateItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	static const QString header[ModFields::COUNT] =
	{
		QT_TRANSLATE_NOOP("ModFields", "Name"),
		QT_TRANSLATE_NOOP("ModFields", ""), // status icon
		QT_TRANSLATE_NOOP("ModFields", ""), // status icon
		QT_TRANSLATE_NOOP("ModFields", "Type"),
	};

	if(role == Qt::DisplayRole && orientation == Qt::Horizontal)
		return QCoreApplication::translate("ModFields", header[section].toStdString().c_str());
	return QVariant();
}

void ModStateItemModel::reloadRepositories()
{
	beginResetModel();
	endResetModel();
}

void ModStateItemModel::modChanged(QString modID)
{
	int index = modNameToID.indexOf(modID);
	QModelIndex parent = this->parent(createIndex(0, 0, index));
	int row = modIndex[modIndexToName(parent)].indexOf(modID);
	emit dataChanged(createIndex(row, 0, index), createIndex(row, 4, index));
}

void ModStateItemModel::endResetModel()
{
	modNameToID = model->getAllMods();
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

QModelIndex ModStateItemModel::index(int row, int column, const QModelIndex & parent) const
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
	return QModelIndex();
}

QModelIndex ModStateItemModel::parent(const QModelIndex & child) const
{
	QString modID = modNameToID[child.internalId()];
	for(auto entry = modIndex.begin(); entry != modIndex.end(); entry++) // because using range-for entry type is QMap::value_type oO
	{
		if(entry.key() != "" && entry.value().indexOf(modID) != -1)
		{
			return createIndex(entry.value().indexOf(modID), child.column(), modNameToID.indexOf(entry.key()));
		}
	}
	return QModelIndex();
}

void CModFilterModel::setTypeFilter(ModFilterMask newFilterMask)
{
	filterMask = newFilterMask;
	invalidateFilter();
}

bool CModFilterModel::filterMatchesThis(const QModelIndex & source) const
{
	//QString modID =source.data(ModRoles::ModNameRole).toString();
	//ModState mod = base->model->getMod(modID);
	return /*(mod.getModStatus() & filterMask) == filteredType &&*/
	       QSortFilterProxyModel::filterAcceptsRow(source.row(), source.parent());
}

bool CModFilterModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
	QModelIndex index = base->index(source_row, 0, source_parent);
	QString modID = index.data(ModRoles::ModNameRole).toString();
	if (base->model->getMod(modID).isHidden())
		return false;

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

CModFilterModel::CModFilterModel(ModStateItemModel * model, QObject * parent)
	: QSortFilterProxyModel(parent), base(model), filterMask(ModFilterMask::ALL)
{
	setSourceModel(model);
	setSortRole(ModRoles::ValueRole);
}
