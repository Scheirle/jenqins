/*
 * Copyright (C) 2008-2013 The Communi Project
 *
 * This example is free, and not covered by the LGPL license. There is no
 * restriction applied to their modification, redistribution, using and so on.
 * You can study them, modify them, use them in your own program - either
 * completely or partially.
 */

#include <QCoreApplication>
#include <QSettings>
#include <QTextStream>
#include "ircbot.h"
#include <iostream>

#include <csignal>

struct CleanExit{
	CleanExit() {
		signal(SIGINT, &CleanExit::exitQt);
		signal(SIGTERM, &CleanExit::exitQt);
	}

	static void exitQt(int sig) {
		std::cout << "exiting.." << std::endl;
		QCoreApplication::exit(0);
	}
};

int main(int argc, char* argv[])
{
	CleanExit cleanExit;
    QCoreApplication app(argc, argv);
    IrcBot bot;
    QTextStream qout(stdout);

    QSettings settings("settings.ini", QSettings::IniFormat);
    bot.setHost(settings.value("host", "irc.freenode.net").toString());
    bot.setPort(settings.value("port", 6667).toInt());
    bot.setNickName(settings.value("nickname", "jenqins").toString());
    bot.setUserName(settings.value("username", bot.nickName()).toString());
    bot.setRealName(settings.value("realname", bot.userName()).toString());
    bot.setNickservPassword(settings.value("nickservpassword", "").toString());
	QString concatenatedChannels = settings.value("channels", "#ceylon").toString();
	QStringList channels = concatenatedChannels.split(" ", QString::SkipEmptyParts);
    bot.setChannels(channels);

    bot.open();
    qout << "Verbinde als: " << bot.nickName() << "@" << bot.host() << ":" << bot.port() << "..." << endl;
    return app.exec();
}
