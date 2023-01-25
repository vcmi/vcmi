/*
 * cmodlistmodel_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "cmodlist.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

namespace ModFields
{
enum EModFields
{
	NAME,
	STATUS_ENABLED,
	STATUS_UPDATE,
	TYPE,
	VERSION,
	COUNT
};
}

namespace ModRoles
{
enum EModRoles
{
	ValueRole = Qt::UserRole,
	ModNameRole
};
}

class CModListModel : public QAbstractItemModel, public CModList
{
	Q_OBJECT

	QVector<QString> modNameToID;
	// contains mapping mod -> numbered list of submods
	// mods that have no parent located under "" key (empty string)
	QMap<QString, QVector<QString>> modIndex;

	void endResetModel();

	QString modIndexToName(const QModelIndex & index) const;

	QVariant getTextAlign(int field) const;
	QVariant getValue(const CModEntry & mod, int field) const;
	QVariant getText(const CModEntry & mod, int field) const;
	QVariant getIcon(const CModEntry & mod, int field) const;

public:
	explicit CModListModel(QObject * parent = 0);

	/// CModListContainer overrides
	void resetRepositories() override;
	void reloadRepositories() override;
	void addRepository(QVariantMap data) override;
	void modChanged(QString modID) override;

	QVariant data(const QModelIndex & index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	int rowCount(const QModelIndex & parent) const override;
	int columnCount(const QModelIndex & parent) const override;

	QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex & child) const override;

	Qt::ItemFlags flags(const QModelIndex & index) const override;

signals:

public slots:

};

class CModFilterModel : public QSortFilterProxyModel
{
	CModListModel * base;
	int filteredType;
	int filterMask;

	bool filterMatchesThis(const QModelIndex & source) const;

	bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

public:
	void setTypeFilter(int filteredType, int filterMask);

	CModFilterModel(CModListModel * model, QObject * parent = 0);
};
