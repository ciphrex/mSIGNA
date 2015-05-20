///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// keychainbackupwizard.h
//
// Copyright (c) 2015 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <CoinQ/CoinQ_typedefs.h>

#include <QWizard>

class QPlainTextEdit;
class QLineEdit;

class KeychainBackupWizard : public QWizard
{
    Q_OBJECT

public:
    KeychainBackupWizard(const QString& name, const secure_bytes_t& seed, QWidget* parent = NULL);
};


class WordlistViewPage : public QWizardPage
{
    Q_OBJECT

public:
    WordlistViewPage(const QString& name, const secure_bytes_t& seed, QWidget* parent = NULL);

protected:
    QPlainTextEdit* wordlistEdit;
};

class WordlistVerifyPage : public QWizardPage
{
    Q_OBJECT

public:
    WordlistVerifyPage(const QString& name, const secure_bytes_t& seed, QWidget* parent = NULL);

    bool isComplete() const;

protected:
    //void keyPressEvent(QKeyEvent* event);

    QString wordlist;
    QLineEdit* wordlistEdit;
};

class WordlistCompletePage : public QWizardPage
{
    Q_OBJECT

public:
    WordlistCompletePage(const QString& name, QWidget* parent = NULL);
};
