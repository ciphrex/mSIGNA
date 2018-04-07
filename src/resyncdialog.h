///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// resyncdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class QLineEdit;

#include <QDialog>

class ResyncDialog : public QDialog
{
    Q_OBJECT

public:
    ResyncDialog(int resyncHeight, int maxHeight, QWidget* parent = NULL);

    int getResyncHeight() const;

private:
    QLineEdit* resyncHeightEdit;
};

