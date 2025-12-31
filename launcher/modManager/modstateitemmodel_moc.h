/*
 * modstateview_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

class ModStateModel;
class ModState;

namespace ModFields
{
enum EModFields
{
	NAME,
	STATUS_ENABLED,
	STATUS_UPDATE,
	TYPE,
	STARS,
	COUNT
};
}

enum class ModFilterMask : uint8_t
{
	ALL,
	AVAILABLE,
	INSTALLED,
	UPDATABLE,
	ENABLED,
	DISABLED
};

namespace ModRoles
{
enum EModRoles
{
	ValueRole = Qt::UserRole,
	ModNameRole
};
}

class ModStateItemModel final : public QAbstractItemModel
{
	friend class CModFilterModel;
	Q_OBJECT

	std::shared_ptr<ModStateModel> model;

	QStringList modNameToID;
	// contains mapping mod -> numbered list of submods
	// mods that have no parent located under "" key (empty string)
	QMap<QString, QStringList> modIndex;

	void endResetModel();

	QString modIndexToName(const QModelIndex & index) const;
	QString modTypeName(QString modTypeID) const;

	QVariant getTextAlign(int field) const;
	QVariant getValue(const ModState & mod, int field) const;
	QVariant getText(const ModState & mod, int field) const;
	QVariant getIcon(const ModState & mod, int field) const;

public:
	explicit ModStateItemModel(std::shared_ptr<ModStateModel> model, QObject * parent);

	/// CModListContainer overrides
	void reloadViewModel();
	void modChanged(QString modID);

	QVariant data(const QModelIndex & index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	int rowCount(const QModelIndex & parent) const override;
	int columnCount(const QModelIndex & parent) const override;

	QModelIndex index(int row, int column, const QModelIndex & parent) const override;
	QModelIndex parent(const QModelIndex & child) const override;

	Qt::ItemFlags flags(const QModelIndex & index) const override;
};

class CModFilterModel final : public QSortFilterProxyModel
{
	ModStateItemModel * base;
	ModFilterMask filterMask;

	bool filterMatchesThis(const QModelIndex & source) const;
	bool filterMatchesCategory(const QModelIndex & source) const;

	bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

	bool lessThan(const QModelIndex & source_left, const QModelIndex & source_right) const override;

public:
	void setTypeFilter(ModFilterMask filterMask);

	CModFilterModel(ModStateItemModel * model, QObject * parent = nullptr);
};
