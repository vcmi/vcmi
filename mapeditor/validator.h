/*
 * validator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QDialog>
#include <set>

VCMI_LIB_NAMESPACE_BEGIN
class CMap;
VCMI_LIB_NAMESPACE_END

namespace Ui {
class Validator;
}

class Validator : public QDialog
{
	Q_OBJECT
public:
	struct Issue
	{
		QString message;
		bool critical;
		
		Issue(const QString & m, bool c): message(m), critical(c) {}

		bool operator <(const Issue & other) const
		{
			return message < other.message;
		}
	};
	
public:
	explicit Validator(const CMap * map, QWidget *parent = nullptr);
	~Validator();
	
	static std::set<Issue> validate(const CMap * map);

private:
	Ui::Validator *ui;

	QRect screenGeometry;

	void showValidationResults(const CMap * map);
	void adjustWindowSize();
};

class ValidatorItemDelegate : public QStyledItemDelegate
{
public:
	explicit ValidatorItemDelegate(QObject * parent = nullptr) : QStyledItemDelegate(parent) 
	{
		screenGeometry = QApplication::primaryScreen()->availableGeometry();
	}

	int itemPaddingBottom = 14;
	int iconPadding = 4;
	int textOffsetForIcon = 30;
	int textPaddingRight = 10;
	int minItemWidth = 350;
	int screenMargin = 350;     // some reserved space from screenWidth; used if text.width > screenWidth - screenMargin 
	
	const int offsetForIcon = iconPadding + textOffsetForIcon;

	void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override;

private:
	QRect screenGeometry;
};
