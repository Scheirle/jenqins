/*
 *    <one line to give the program's name and a brief idea of what it does.>
 *    Copyright (C) 2013  Florian Jacob <fjacob@lavabit.com>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "messagemodule.h"
#include <QTime>
#include <QFile>
#include <QDataStream>
#include <Irc>

MessageModule::MessageModule(IrcBot* bot): BotModule(bot)
{
	QFile file("messages.dat");
	if (file.exists()) {
		file.open(QIODevice::ReadOnly);
		QDataStream dataIn(&file);    // read the data serialized from the file
		dataIn >> messages;
		out << "messages loaded." << endl;
	}
	bot->registerModule(this, "memo", "Creates a memo for a given user. Usage: memo USER CUSTOM MEMO");
}

MessageModule::~MessageModule()
{
	QFile file("messages.dat");
	file.open(QIODevice::WriteOnly);
	QDataStream dataOut(&file);   // we will serialize the data into the file
	dataOut << messages;
	out << "messages saved." << endl;
}


void MessageModule::onMessageReceived(IrcMessage* message)
{
	if (message->type() == IrcMessage::Private) {
		IrcPrivateMessage* msg = static_cast<IrcPrivateMessage*>(message);

		// is the message not a direct message => a message from a channel?
		if (msg->target().compare(bot->nickName(), Qt::CaseInsensitive) != 0) {
			if (messages.contains(msg->sender().name())) {
				out << "replaying messages for " << msg->sender().name() << ".." << endl;
				foreach (QString message, messages.values(msg->sender().name()))
				{
					bot->sendCommand(IrcCommand::createMessage(msg->target(), message));
				}
				messages.remove(msg->sender().name());
			}

			if (msg->message().startsWith(bot->nickName(), Qt::CaseInsensitive)) {
				QStringList parts = msg->message().split(" ", QString::SkipEmptyParts);
				if (parts.size() >= 4) {
					parts.removeFirst();

					if (parts.first() == "memo") {
						parts.removeFirst();
						QString receiver = parts.first();
						parts.removeFirst();
						if (receiver == bot->nickName()) {
							bot->sendCommand(IrcCommand::createMessage(msg->target(),
																	   msg->sender().name() + QString(": You can't leave memos for me..")));
						} else if (receiver == msg->sender().name()) {
							bot->sendCommand(IrcCommand::createMessage(msg->target(),
																	   msg->sender().name() + QString(": You can't leave memos for yourself.")));
						} else if (receiver.contains(":")) {
							bot->sendCommand(IrcCommand::createMessage(msg->target(),
																	   msg->sender().name() + QString(": I'm afraid that colons aren't allowed in names. Do you mean somebody else?")));
						} else {
							messages.insertMulti(receiver,
												 QString("%1: [%2] <%3/%4> %5")
												 .arg(receiver).arg(QTime::currentTime().toString("HH:mm"))
							.arg(msg->target()).arg(msg->sender().name()).arg(parts.join(" ")));
							bot->sendCommand(IrcCommand::createMessage(msg->target(),
																	   QString("%1: Memo for %2 recorded.").arg(msg->sender().name()).arg(receiver)));
							bot->sendCommand(IrcCommand::createNames(bot->channels()));
						}
					}
				}
			}
		}

	} else if (message->type() == IrcMessage::Join) {
		// we get join messages from all users in the channel at startup, so this gets also called
		// for every user in the channel we joined
		notifyAboutMemos(message->sender().name());
	} else if (message->type() == IrcMessage::Nick) {
		IrcNickMessage* msg = static_cast<IrcNickMessage*>(message);
		notifyAboutMemos(msg->nick());
	} else if (message->type() == IrcMessage::Numeric) {
		IrcNumericMessage* msg = static_cast<IrcNumericMessage*>(message);
		if (msg->code() == Irc::RPL_NAMREPLY) {
			QString channel = msg->parameters().value(2);
			QStringList nicks = msg->parameters().last().split(" ", QString::SkipEmptyParts);
			foreach (QString nick, nicks)
			{
				notifyAboutMemos(nick, channel);
			}
		}
	}
}

void MessageModule::notifyAboutMemos(const QString& nick, const QString& channel)
{
	if (messages.contains(nick)) {
		out << "Notifying " << nick << " about his messages." << endl;
		bot->sendCommand(IrcCommand::createMessage(nick, QString("You have memos. Speak publicly in %1 to retreive them.").arg(channel)));
	} else {
		QString shortenedNick(nick.left(nick.size() - 1));
		if (messages.contains(shortenedNick)) {
			out << "Notifying " << nick << " about messages for " << shortenedNick << "." << endl;
			bot->sendCommand(IrcCommand::createMessage(nick, QString("%1 has memos. Is that you? Change your nick back and speak publicly in %2 to retreive them.").arg(shortenedNick).arg(channel)));
		}
	}
}


#include "messagemodule.moc"