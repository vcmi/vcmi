#ifndef CHAT_MOC_H
#define CHAT_MOC_H

#include <QWidget>

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

#endif // CHAT_MOC_H
