///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// scriptview.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_SCRIPTVIEW_H
#define COINVAULT_SCRIPTVIEW_H

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

#endif // COINVAULT_SCRIPTVIEW_H
