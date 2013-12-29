///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// splashscreen.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "versioninfo.h"
#include "copyrightinfo.h"
#include "splashscreen.h"

#include <QPainter>

const QString SPLASHSCREENFILENAME(":/images/splash.png");

SplashScreen::SplashScreen()
    : QSplashScreen()
{
    QPixmap pixmap(SPLASHSCREENFILENAME);

    QPainter painter(&pixmap);
    QFontMetrics metrics = painter.fontMetrics();

    painter.drawText(
        pixmap.width() - metrics.width(VERSIONTEXT) - VERSIONPADDINGRIGHT,
        pixmap.height() - metrics.height() - VERSIONPADDINGBOTTOM,
        VERSIONTEXT
    );

    painter.drawText(
        pixmap.width() - metrics.width(COPYRIGHTTEXT) - COPYRIGHTPADDINGRIGHT,
        pixmap.height() - metrics.height() - COPYRIGHTPADDINGBOTTOM,
        COPYRIGHTTEXT
    );

    painter.end();

    setPixmap(pixmap);
}
