///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// splashscreen.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "versioninfo.h"
#include "copyrightinfo.h"
#include "splashscreen.h"

#include <QPainter>

#ifdef DEFAULT_NETWORK_LITECOIN
const QString SPLASHSCREENFILENAME(":/images/splash_ltc.png");
#else
const QString SPLASHSCREENFILENAME(":/images/splash_btc.png");
#endif

const QColor TEXTCOLOR(225, 225, 225);

const int VERSIONPADDINGRIGHT = 20;
const int VERSIONPADDINGBOTTOM = 50;
 
const int COPYRIGHTPADDINGRIGHT = 20;
const int COPYRIGHTPADDINGBOTTOM = 30;

SplashScreen::SplashScreen()
    : QSplashScreen()
{
    QPixmap pixmap(SPLASHSCREENFILENAME);

    QPainter painter(&pixmap);
    QFontMetrics metrics = painter.fontMetrics();

    painter.setPen(QPen(TEXTCOLOR));

    QString fullVersionText = "(" + getShortCommitHash() + ") ";
    fullVersionText += getVersionText();

    painter.drawText(
        pixmap.width() - metrics.width(fullVersionText) - VERSIONPADDINGRIGHT,
        pixmap.height() - metrics.height() - VERSIONPADDINGBOTTOM,
        fullVersionText 
    );

    painter.drawText(
        pixmap.width() - metrics.width(getCopyrightText()) - COPYRIGHTPADDINGRIGHT,
        pixmap.height() - metrics.height() - COPYRIGHTPADDINGBOTTOM,
        getCopyrightText()
    );

    painter.end();

    setPixmap(pixmap);
}

void SplashScreen::showProgressMessage(const QString& message)
{
    showMessage(QString("\n    ") + message, Qt::AlignLeft, TEXTCOLOR);
}
