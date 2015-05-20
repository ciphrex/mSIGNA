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

private:
    QPlainTextEdit* wordlistEdit;
};

class WordlistVerifyPage : public QWizardPage
{
    Q_OBJECT

public:
    WordlistVerifyPage(const QString& name, const secure_bytes_t& seed, QWidget* parent = NULL);

    bool isComplete() const;

private:
    QString wordlist;
    QPlainTextEdit* wordlistEdit;
};

class WordlistCompletePage : public QWizardPage
{
    Q_OBJECT

public:
    WordlistCompletePage(QWidget* parent = NULL);
};
