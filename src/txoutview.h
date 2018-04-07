///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txoutview.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <QTreeView>

class TxOutView : public QTreeView
{
    Q_OBJECT

public:
    TxOutView(QWidget* parent = NULL);

    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
};

