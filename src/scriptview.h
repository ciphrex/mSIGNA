///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// scriptview.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class ScriptModel;

#include <QTreeView>

class ScriptView : public QTreeView
{
    Q_OBJECT

public:
    ScriptView(QWidget* parent = NULL);

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"
    void setModel(ScriptModel* model);
    #pragma clang diagnostic pop

    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    ScriptModel* scriptModel;
};

