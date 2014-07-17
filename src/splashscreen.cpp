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

    QString fullVersionText = "(" + getShortCommitHash() + ") ";
    fullVersionText += getVersionText();

    painter.drawText(
        pixmap.width() - metrics.width(fullVersionText) - getVersionPaddingRight(),
        pixmap.height() - metrics.height() - getVersionPaddingBottom(),
        fullVersionText 
    );

    painter.drawText(
        pixmap.width() - metrics.width(getCopyrightText()) - getCopyrightPaddingRight(),
        pixmap.height() - metrics.height() - getCopyrightPaddingBottom(),
        getCopyrightText()
    );

    painter.end();

    setPixmap(pixmap);
}
