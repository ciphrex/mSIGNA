///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// quicknewaccountdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class QLineEdit;
class QComboBox;
class QDateTimeEdit;
class QCalendarWidget;
class QCheckBox;

#include <QDialog>

class QuickNewAccountDialog : public QDialog
{
    Q_OBJECT

public:
    QuickNewAccountDialog(QWidget* parent = NULL);
    ~QuickNewAccountDialog();

    QString getName() const;
    int getMinSigs() const;
    int getMaxSigs() const;
    qint64 getCreationTime() const;
    bool getUseSegwit() const;

private:
    QLineEdit* nameEdit;
    QComboBox* minSigComboBox;
    QComboBox* maxSigComboBox;
    QDateTimeEdit* creationTimeEdit;
    QCalendarWidget* calendarWidget;
    QCheckBox* segwitCheckBox;
};

