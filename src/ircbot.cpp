/*
 * Copyright (C) 2008-2013 The Communi Project
 *
 * This example is free, and not covered by the LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include "ircbot.h"
#include "modules/greetmodule.h"
#include "modules/echomodule.h"
#include "modules/messagemodule.h"
#include "modules/topicmodule.h"
#include <Communi/IrcCommand>
#include <Communi/IrcMessage>


IrcBot::IrcBot(QObject* parent) : IrcSession(parent), out(stdout)
{
	connect(this, SIGNAL(connected()), this, SLOT(onConnected()));

	connect(this, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));
	helpOutput.insert("help", "Shows this help.");

	/*
	 *	GreetModule* greeter = new GreetModule(this);
	 */
	MessageModule* messenger = new MessageModule(this);	
	TopicModule* topic = new TopicModule(this);
	/*
	 E c*hoModule* echoer = new EchoModule(this);
	 */
}

void IrcBot::registerModule(BotModule* module, QString command, QString description)
{
	connect(this, SIGNAL(messageReceived(IrcMessage*)), module, SLOT(onMessageReceived(IrcMessage*)));
	helpOutput.insert(command, description);
}

void IrcBot::registerModule(BotModule* module, QMap<QString, QString> moduleHelpOutput)
{
	connect(this, SIGNAL(messageReceived(IrcMessage*)), module, SLOT(onMessageReceived(IrcMessage*)));
	helpOutput.unite(moduleHelpOutput);
}

QStringList IrcBot::channels() const
{
	return m_channels;
}

void IrcBot::setChannels(const QStringList& channels)
{
	m_channels = channels;

}

QString IrcBot::nickservPassword() const
{
	return m_nickservPassword;
}

void IrcBot::setNickservPassword(const QString& password)
{
	m_nickservPassword = password;
}


void IrcBot::onConnected()
{
	out << "Verbunden." << endl;
	if (!m_nickservPassword.isEmpty()) {
		sendCommand(IrcCommand::createMessage(QString("NickServ"), QString("identify ") + m_nickservPassword));
		out << "tried to auth with nickserv." << endl;
	} else {
		foreach (QString channel, m_channels)
		{
			sendCommand(IrcCommand::createJoin(channel));
			out << "Betrete " << channel << "." << endl;
		}
	}

}

void IrcBot::onMessageReceived(IrcMessage* message)
{
	if (message->type() == IrcMessage::Notice) {
		IrcNoticeMessage* msg = static_cast<IrcNoticeMessage*>(message);
		out << "[Notice] " << msg->message() << endl;
		if (msg->message().startsWith("You are now identified for ")) {
			foreach (QString channel, m_channels)
			{
				sendCommand(IrcCommand::createJoin(channel));
				out << "Betrete " << channel << "." << endl;
			}
		}
	} else if (message->type() == IrcMessage::Private) {
		IrcPrivateMessage* msg = static_cast<IrcPrivateMessage*>(message);
		out << "[PM] <" << msg->sender().name() << " > " << msg->message() << endl;
		
		//help
		if (msg->target().compare(nickName(), Qt::CaseInsensitive) != 0) {
			// message is from channel
			if (msg->message().startsWith(nickName(), Qt::CaseInsensitive)) {
				// message is for bot
				QStringList parts = msg->message().split(" ", QString::SkipEmptyParts);
				if (parts.size() >= 2) {
					parts.removeFirst(); //bot name
					
					if (parts.first() == "help") {
						out << "Sending help to " << msg->target() << endl;

						sendCommand(IrcCommand::createMessage(msg->target(), QString("Commands\t - Description")));
						QMapIterator<QString, QString> i(helpOutput);
						while (i.hasNext()) {
							i.next();
							sendCommand(IrcCommand::createMessage(msg->target(), QString("%1\t - %2").arg(i.key()).arg(i.value())));
						}
					}
				}
			}
		}
		
	} else if (message->type() == IrcMessage::Error) {
		IrcErrorMessage* msg = static_cast<IrcErrorMessage*>(message);
		out << "[Error] " << msg->error() << endl;

	}
}


#include "ircbot.moc"