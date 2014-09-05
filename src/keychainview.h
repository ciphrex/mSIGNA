///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// keychainview.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

class QMenu;

#include <QTreeView>
#include <QList>
#include <QString>

class KeychainView : public QTreeView
{
    Q_OBJECT

public:
    KeychainView(QWidget* parent = NULL);

    void setMenu(QMenu* menu) { this->menu = menu; }

    QList<QString> getAllKeychains() const;
    QList<QString> getSelectedKeychains() const;

public slots:
    void updateColumns();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    QMenu* menu;
};

