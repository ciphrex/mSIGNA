///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// accountview.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class QMenu;

#include <QTreeView>

class AccountView : public QTreeView
{
    Q_OBJECT

public:
    AccountView(QWidget* parent = NULL);

    void setMenu(QMenu* menu) { this->menu = menu; }

signals:
    void updateModel();

public slots:
    void updateAll();
    void updateColumns();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    QMenu* menu;
};

