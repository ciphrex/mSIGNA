///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// scriptview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "scriptview.h"
#include "scriptmodel.h"

#include <QMenu>
#include <QContextMenuEvent>

ScriptView::ScriptView(QWidget* parent)
    : QTreeView(parent), scriptModel(NULL)
{
}

void ScriptView::setModel(ScriptModel* model)
{
    QTreeView::setModel(model);
    scriptModel = model;
}

void ScriptView::update()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    resizeColumnToContents(3);
}

void ScriptView::contextMenuEvent(QContextMenuEvent* /*event*/)
{
/*
    QMenu menu(this);
    if (exportPrivateAction)    menu.addAction(exportPrivateAction);
    if (exportPublicAction)     menu.addAction(exportPublicAction);
    if (!menu.isEmpty())        menu.exec(event->globalPos());
*/
}
