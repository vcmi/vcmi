/*
 * chat_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QWidget>
#include <QCompleter>

namespace Ui {
class Chat;
}

class Chat : public QWidget
{
	Q_OBJECT
	
	enum ChatId
	{
		GLOBAL = 0,
		ROOM
	};
	
	QCompleter namesCompleter;
	QString username, session;
	ChatId chatId = GLOBAL;
	
	QVector<QTextDocument*> chatDocuments;
	
private:
	void setChatId(ChatId);
	void sendMessage();

public:
	explicit Chat(QWidget *parent = nullptr);
	~Chat();
	
	void setUsername(const QString &);
	void setSession(const QString &);
	void setChannel(const QString &);
	
	void clearUsers();
	void addUser(const QString & user);
	
	void chatMessage(const QString & title, const QString & channel, QString body, bool isSystem = false);
	void chatMessage(const QString & title, QString body, bool isSystem = false);
	
signals:
	void messageSent(QString);
	void channelSwitch(QString);
	
public slots:
	void sysMessage(QString body);

private slots:
	void on_messageEdit_returnPressed();

	void on_sendButton_clicked();

	void on_chatSwitch_clicked();

private:
	Ui::Chat *ui;
};
