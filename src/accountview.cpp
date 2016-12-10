///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// accountview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "accountview.h"
#include "accountmodel.h"

#include "txdialog.h"

#include <logger/logger.h>

#include <QMenu>
#include <QContextMenuEvent>
#include <QAction>

AccountView::AccountView(QWidget* parent)
    : QTreeView(parent), menu(NULL)
{
}

void AccountView::updateAll()
{
    if (!model()) return;

    LOGGER(trace) << "AccountView::updateAll()" << std::endl;

    QModelIndexList indexes = selectionModel()->selectedRows(0);
    int selectedRow = indexes.isEmpty() ? -1 : indexes.at(0).row();

    emit updateModel();
    for (int i = 0; i < model()->columnCount(); i++) { resizeColumnToContents(i); }

    if (selectedRow >= model()->rowCount()) { selectedRow = 0; }
    if (selectedRow < model()->rowCount())
    {
        QItemSelection selection(model()->index(selectedRow, 0), model()->index(selectedRow, model()->columnCount() - 1));
        selectionModel()->select(selection, QItemSelectionModel::SelectCurrent);
    }
}

void AccountView::updateColumns()
{
    if (!model()) return;
    for (int i = 0; i < model()->columnCount(); i++) { resizeColumnToContents(i); }
}

void AccountView::contextMenuEvent(QContextMenuEvent* event)
{
    if (menu) menu->exec(event->globalPos());
}

