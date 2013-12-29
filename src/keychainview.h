///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// keychainview.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_KEYCHAINVIEW_H
#define COINVAULT_KEYCHAINVIEW_H

class QMenu;

#include <QTreeView>

class KeychainView : public QTreeView
{
    Q_OBJECT

public:
    KeychainView(QWidget* parent = NULL);

    void setMenu(QMenu* menu) { this->menu = menu; }
    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    QMenu* menu;
};

#endif // COINVAULT_ACCOUNTVIEW_H
