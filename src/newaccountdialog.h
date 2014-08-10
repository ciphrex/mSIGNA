///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// newaccountdialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

class QLineEdit;
class QComboBox;
class QDateTimeEdit;
class QCalendarWidget;

#include <QDialog>

class NewAccountDialog : public QDialog
{
    Q_OBJECT

public:
    NewAccountDialog(const QList<QString>& keychainNames, QWidget* parent = NULL);
    ~NewAccountDialog();

    QString getName() const;
    const QList<QString>& getKeychainNames() const { return keychainNames; }
    int getMinSigs() const;

    qint64 getCreationTime() const;

private:
    QLineEdit* nameEdit;
    QLineEdit* keychainEdit;
    QComboBox* minSigComboBox;
    QLineEdit* minSigLineEdit;
    QList<QString> keychainNames;

    QDateTimeEdit* creationTimeEdit;
    QCalendarWidget* calendarWidget;
};

