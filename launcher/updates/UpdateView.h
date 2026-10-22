/*
 * UpdateView.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QUrl>
#include <QWidget>

namespace Ui
{
class UpdateView;
}

class UpdateChecker;

/// Tab with the result of an update check: notes, changelog and the way to update
class UpdateView : public QWidget
{
	Q_OBJECT

	std::unique_ptr<Ui::UpdateView> ui;
	UpdateChecker * checker;

	QUrl actionUrl;
	QUrl actionFallbackUrl; ///< opened if no application can open actionUrl

	void changeEvent(QEvent * event) override;
	void refresh();
	void showUpdate();

public:
	explicit UpdateView(QWidget * parent = nullptr);
	~UpdateView() override;

private slots:
	void on_buttonAction_clicked();
	void on_buttonCheck_clicked();
	void on_buttonChangelog_clicked();
};
