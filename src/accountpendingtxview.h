///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// accountpendingtxview.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class TxModel;

#include <QTreeView>

class TxView : public QTreeView
{
    Q_OBJECT

public:
    TxView(QWidget* parent = NULL);

    void setModel(TxModel* model);
    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    TxModel* accountHistoryModel;
};

