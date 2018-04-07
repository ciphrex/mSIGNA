///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// keychainbackupwizard.h
//
// Copyright (c) 2015 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinQ/CoinQ_typedefs.h>

#include <QWizard>
#include <QList>

class QPlainTextEdit;
class QLineEdit;

class KeychainBackupWizard : public QWizard
{
    Q_OBJECT

public:
    KeychainBackupWizard(const QString& name, const secure_bytes_t& seed, QWidget* parent = NULL);
    KeychainBackupWizard(const QList<QString>& names, const QList<secure_bytes_t>& seeds, QWidget* parent = NULL);
};


class WordlistViewPage : public QWizardPage
{
    Q_OBJECT

public:
    WordlistViewPage(int i, const QString& name, const secure_bytes_t& seed, QWidget* parent = NULL);

protected:
    QPlainTextEdit* wordlistEdit;
};

class WordlistVerifyPage : public QWizardPage
{
    Q_OBJECT

public:
    WordlistVerifyPage(int i, const QString& name, const secure_bytes_t& seed, QWidget* parent = NULL);

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
    WordlistCompletePage(int i, QWidget* parent = NULL);
};
