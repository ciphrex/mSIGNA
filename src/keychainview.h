///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// keychainview.h
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

