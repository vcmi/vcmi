#include "chat_moc.h"
#include "ui_chat_moc.h"

Chat::Chat(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Chat)
{
	ui->setupUi(this);
	
	namesCompleter.setModel(ui->listUsers->model());
	namesCompleter.setCompletionMode(QCompleter::InlineCompletion);
	
	ui->messageEdit->setCompleter(&namesCompleter);
	
	for(auto i : {GLOBAL, ROOM})
		chatDocuments.push_back(new QTextDocument(this));
	
	ui->chat->setDocument(chatDocuments[GLOBAL]);
}

Chat::~Chat()
{
	delete ui;
}

void Chat::setUsername(const QString & user)
{
	username = user;
}

void Chat::setSession(const QString & s)
{
	session = s;
	if(session.isEmpty())
		setChatId(GLOBAL);
	else
		setChatId(ROOM);
}

void Chat::addUser(const QString & user)
{
	ui->listUsers->addItem(new QListWidgetItem("@" + user));
}

void Chat::clearUsers()
{
	ui->listUsers->clear();
}

void Chat::chatMessage(const QString & title, QString body, bool isSystem)
{
	const QTextCharFormat regularFormat;
	const QString boldHtml = "<b>%1</b>";
	const QString colorHtml = "<font color=\"%1\">%2</font>";
	bool meMentioned = false;
	bool isScrollBarBottom = (ui->chat->verticalScrollBar()->maximum() - ui->chat->verticalScrollBar()->value() < 24);
	
	QTextCursor curs(ui->chat->document());
	curs.movePosition(QTextCursor::End);
	
	QString titleColor = "Olive";
	if(isSystem || title == "System")
		titleColor = "ForestGreen";
	if(title == username)
		titleColor = "Gold";
	
	curs.insertHtml(boldHtml.arg(colorHtml.arg(titleColor, title + ": ")));
	
	QRegularExpression mentionRe("@[\\w\\d]+");
	auto subBody = body;
	int mem = 0;
	for(auto match = mentionRe.match(subBody); match.hasMatch(); match = mentionRe.match(subBody))
	{
		body.insert(mem + match.capturedEnd(), QChar(-1));
		body.insert(mem + match.capturedStart(), QChar(-1));
		mem += match.capturedEnd() + 2;
		subBody = body.right(body.size() - mem);
	}
	auto pieces = body.split(QChar(-1));
	for(auto & block : pieces)
	{
		if(block.startsWith("@"))
		{
			if(block == "@" + username)
			{
				meMentioned = true;
				curs.insertHtml(boldHtml.arg(colorHtml.arg("IndianRed", block)));
			}
			else
				curs.insertHtml(colorHtml.arg("DeepSkyBlue", block));
		}
		else
		{
			if(isSystem)
				curs.insertHtml(colorHtml.arg("ForestGreen", block));
			else
				curs.insertText(block, regularFormat);
		}
	}
	curs.insertText("\n", regularFormat);
	
	if(meMentioned || isScrollBarBottom)
	{
		ui->chat->ensureCursorVisible();
		ui->chat->verticalScrollBar()->setValue(ui->chat->verticalScrollBar()->maximum());
	}
}

void Chat::sysMessage(QString body)
{
	chatMessage("System", body, true);
}

void Chat::on_messageEdit_returnPressed()
{
	emit messageSent(ui->messageEdit->text());
	ui->messageEdit->clear();
}

void Chat::on_sendButton_clicked()
{
	emit messageSent(ui->messageEdit->text());
	ui->messageEdit->clear();
}

void Chat::on_chatSwitch_clicked()
{
	if(chatId == GLOBAL && !session.isEmpty())
		setChatId(ROOM);
	else
		setChatId(GLOBAL);
}

void Chat::setChatId(ChatId _chatId)
{
	static const QMap<ChatId, QString> chatNames{{GLOBAL, "Global"}, {ROOM, "Room"}};
	
	chatId = _chatId;
	ui->chatSwitch->setText(chatNames[chatId] + " chat");
	ui->chat->setDocument(chatDocuments[chatId]);
}
