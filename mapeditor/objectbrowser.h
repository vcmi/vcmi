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

class ObjectBrowser : public QSortFilterProxyModel
{
public:
	explicit ObjectBrowser(QObject *parent = nullptr);

	TerrainId terrain;
	QString filter;

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
	bool filterAcceptsRowText(int source_row, const QModelIndex &source_parent) const;
};
