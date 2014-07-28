///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// scriptview.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

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

