/*
 * objectbrowser.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QSortFilterProxyModel>
#include "../lib/Terrain.h"

class ObjectBrowserProxyModel : public QSortFilterProxyModel
{
public:
	explicit ObjectBrowserProxyModel(QObject *parent = nullptr);
	
	Qt::ItemFlags flags(const QModelIndex &index) const override;
		
	QMimeData * mimeData(const QModelIndexList & indexes) const override;

	TerrainId terrain;
	QString filter;

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
	bool filterAcceptsRowText(int source_row, const QModelIndex &source_parent) const;
};

class ObjectBrowser : public QTreeView
{
public:
	ObjectBrowser(QWidget * parent);
	
protected:
	void startDrag(Qt::DropActions supportedActions) override;
};
