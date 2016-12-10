///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// unspenttxoutview.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class UnspentTxOutModel;

#include <QTreeView>

class UnspentTxOutView : public QTreeView
{
    Q_OBJECT

public:
    UnspentTxOutView(QWidget* parent = nullptr);

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"
    void setModel(UnspentTxOutModel* model);
    #pragma clang diagnostic pop

    void setMenu(QMenu* menu) { this->menu = menu; }

public slots:
    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    UnspentTxOutModel* model;
    QMenu* menu;
};

