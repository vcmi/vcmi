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
	static const QStringList modTypes = {
		QT_TR_NOOP("Translation"),
		QT_TR_NOOP("Town"),
		QT_TR_NOOP("Test"),
		QT_TR_NOOP("Templates"),
		QT_TR_NOOP("Spells"),
		QT_TR_NOOP("Music"),
		QT_TR_NOOP("Maps"),
		QT_TR_NOOP("Sounds"),
		QT_TR_NOOP("Skills"),
		QT_TR_NOOP("Other"),
		QT_TR_NOOP("Objects"),
		QT_TR_NOOP("Mechanics"),
		QT_TR_NOOP("Interface"),
		QT_TR_NOOP("Heroes"),
		QT_TR_NOOP("Graphical"),
		QT_TR_NOOP("Expansion"),
		QT_TR_NOOP("Creatures"),
		QT_TR_NOOP("Compatibility"),
		QT_TR_NOOP("Campaigns"),
		QT_TR_NOOP("Artifacts"),
		QT_TR_NOOP("AI"),
		QT_TR_NOOP("Resources"),
	};

	if (modTypes.contains(modTypeID))
		return tr(modTypeID.toStdString().c_str());
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

		case ModFields::STARS:
			return mod.getGithubStars() == -1 ? QVariant("") : mod.getGithubStars();

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
	static const QString iconDisabled = ":/icons/mod-disabled.png";
	static const QString iconDisabledSubmod = ":/icons/submod-disabled.png";
	static const QString iconDownload = ":/icons/mod-download.png";
	static const QString iconEnabled = ":/icons/mod-enabled.png";
	static const QString iconEnabledSubmod = ":/icons/submod-enabled.png";
	static const QString iconUpdate = ":/icons/mod-update.png";

	if (field == ModFields::STATUS_ENABLED)
	{
		if (!model->isModInstalled(mod.getID()))
			return QVariant();

		if(mod.isSubmod() && !model->isModEnabled(mod.getTopParentID()))
		{
			QString topParentID = mod.getTopParentID();
			QString settingID = mod.getID().section('.', 1);

			if (model->isModSettingEnabled(topParentID, settingID))
				return QIcon(iconEnabledSubmod);
			else
				return QIcon(iconDisabledSubmod);
		}

		if (model->isModEnabled(mod.getID()))
			return QIcon(iconEnabled);
		else
			return QIcon(iconDisabled);
	}

	if(field == ModFields::STATUS_UPDATE)
	{
		if (model->isModUpdateAvailable(mod.getID()))
			return QIcon(iconUpdate);
		if (!model->isModInstalled(mod.getID()))
			return QIcon(iconDownload);
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
	static const std::array header =
	{
		QT_TRANSLATE_NOOP("ModFields", "Name"),
		QT_TRANSLATE_NOOP("ModFields", ""), // status icon
		QT_TRANSLATE_NOOP("ModFields", ""), // status icon
		QT_TRANSLATE_NOOP("ModFields", "Type"),
		QT_TRANSLATE_NOOP("ModFields", ""), // star icon
	};

	if(orientation == Qt::Horizontal)
	{
		if(role == Qt::DecorationRole)
		{
			if(section == ModFields::STARS)
				return QIcon(":/icons/star.png");
		}
		else if(role == Qt::DisplayRole)
			return QCoreApplication::translate("ModFields", header[section]);
	}
	return QVariant();
}

void ModStateItemModel::reloadViewModel()
{
	beginResetModel();
	endResetModel();
}

void ModStateItemModel::modChanged(QString modID)
{
	int index = modNameToID.indexOf(modID);
	QModelIndex parent = this->parent(createIndex(0, 0, index));
	int row = modIndex[modIndexToName(parent)].indexOf(modID);
	dataChanged(createIndex(row, 0, index), createIndex(row, 4, index));
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

bool CModFilterModel::filterMatchesCategory(const QModelIndex & source) const
{
	QString modID =source.data(ModRoles::ModNameRole).toString();
	ModState mod = base->model->getMod(modID);

	switch (filterMask)
	{
		case ModFilterMask::ALL:
			return true;
		case ModFilterMask::AVAILABLE:
			return !mod.isInstalled();
		case ModFilterMask::INSTALLED:
			return mod.isInstalled();
		case ModFilterMask::UPDATABLE:
			return mod.isUpdateAvailable();
		case ModFilterMask::ENABLED:
			return mod.isInstalled() && base->model->isModEnabled(modID);
		case ModFilterMask::DISABLED:
			return mod.isInstalled() && !base->model->isModEnabled(modID);
	}
	assert(0);
	return false;
}

bool CModFilterModel::filterMatchesThis(const QModelIndex & source) const
{
	return filterMatchesCategory(source) && QSortFilterProxyModel::filterAcceptsRow(source.row(), source.parent());
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
		if(filterMatchesThis(base->index(i, 0, index)))
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

bool CModFilterModel::lessThan(const QModelIndex & source_left, const QModelIndex & source_right) const
{
	if(source_left.column() == ModFields::STARS)
	{
		// Compare STARS numerically (descending)
		QVariant lData = sourceModel()->data(source_left);
		QVariant rData = sourceModel()->data(source_right);

		bool lIsInt = false;
		bool rIsInt = false;

		int lValue = lData.toInt(&lIsInt);
		int rValue = rData.toInt(&rIsInt);

		if (!lIsInt)
			lValue = -1;
			
		if (!rIsInt)
			rValue = -1;
		
		return lValue > rValue;

		// Compare NAME (ascending)
		const QString leftName  = sourceModel()->data(source_left.siblingAtColumn(ModFields::NAME)).toString();
		const QString rightName = sourceModel()->data(source_right.siblingAtColumn(ModFields::NAME)).toString();
		if (leftName != rightName)
			return leftName < rightName;
	}
	if(source_left.column() != ModFields::STATUS_ENABLED)
		return QSortFilterProxyModel::lessThan(source_left, source_right);

	const auto leftMod = base->model->getMod(base->modIndexToName(source_left));
	const auto rightMod = base->model->getMod(base->modIndexToName(source_right));

	const auto isLeftEnabled = base->model->isModEnabled(leftMod.getID());
	const auto isRightEnabled = base->model->isModEnabled(rightMod.getID());
	if(!isLeftEnabled && isRightEnabled)
		return true;
	if(isLeftEnabled && !isRightEnabled)
		return false;

	const auto isLeftInstalled = leftMod.isInstalled();
	const auto isRightInstalled = rightMod.isInstalled();
	if(!isLeftInstalled && isRightInstalled)
		return true;
	if(isLeftInstalled && !isRightInstalled)
		return false;

	return QSortFilterProxyModel::lessThan(source_left.siblingAtColumn(ModFields::NAME), source_right.siblingAtColumn(ModFields::NAME));
}

CModFilterModel::CModFilterModel(ModStateItemModel * model, QObject * parent)
	: QSortFilterProxyModel(parent), base(model), filterMask(ModFilterMask::ALL)
{
	setSourceModel(model);
	setSortRole(ModRoles::ValueRole);
}
