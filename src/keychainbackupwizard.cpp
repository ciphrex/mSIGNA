///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// keychainbackupwizard.cpp
//
// Copyright (c) 2015 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "keychainbackupwizard.h"
#include "wordlistvalidator.h"

#include <CoinCore/bip39.h>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>

#include <stdexcept>

KeychainBackupWizard::KeychainBackupWizard(const QString& name, const secure_bytes_t& seed, QWidget* parent)
    : QWizard(parent)
{
    addPage(new WordlistViewPage(1, name, seed));
    addPage(new WordlistVerifyPage(2, name, seed));
    addPage(new WordlistCompletePage(3));
    setWindowTitle(tr("Keychain Backup"));
}

KeychainBackupWizard::KeychainBackupWizard(const QList<QString>& names, const QList<secure_bytes_t>& seeds, QWidget* parent)
    : QWizard(parent)
{
    if (names.size() != seeds.size()) throw std::runtime_error("Number of names does not match number of seeds.");
    if (names.isEmpty()) throw std::runtime_error("At least one keychain is required.");

    int i = 0;
    for (auto& name: names)
    {
        const secure_bytes_t& seed = seeds.at(i);
        addPage(new WordlistViewPage(2*i + 1, name, seed));
        addPage(new WordlistVerifyPage(2*i + 2, name, seed));
        i++;
    }
    addPage(new WordlistCompletePage(2*i + 1));
    setWindowTitle(tr("Keychain Backup"));
}


WordlistViewPage::WordlistViewPage(int i, const QString& name, const secure_bytes_t& seed, QWidget* parent)
    : QWizardPage(parent)
{
    setTitle(tr("Step ") + QString::number(i) + tr(": Copy"));
    setSubTitle(tr("Keychain: ") + name);

    QLabel* promptLabel = new QLabel(tr("IMPORTANT: Please write down the following list of words. In the event you lose your data you can recover your keychain by entering this list."));
    promptLabel->setWordWrap(true);

    wordlistEdit = new QPlainTextEdit();
    wordlistEdit->setReadOnly(true);
    wordlistEdit->setPlainText(Coin::BIP39::toWordlist(seed).c_str());

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(promptLabel);
    layout->addWidget(wordlistEdit);
    setLayout(layout);
}

WordlistVerifyPage::WordlistVerifyPage(int i, const QString& name, const secure_bytes_t& seed, QWidget* parent)
    : QWizardPage(parent)
{
    using namespace Coin;

    setTitle(tr("Step ") + QString::number(i) + tr(": Verify"));
    setSubTitle(tr("Keychain: ") + name);

    QLabel* promptLabel = new QLabel(tr("Please enter the words in the correct order below:"));
    promptLabel->setWordWrap(true);

    wordlist = BIP39::toWordlist(seed).c_str();

    wordlistEdit = new QLineEdit();
    wordlistEdit->setValidator(new WordlistValidator(BIP39::minWordLen(), BIP39::maxWordLen(), this));
    QObject::connect(wordlistEdit, &QLineEdit::textChanged, [=]() { emit completeChanged(); });

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(promptLabel);
    layout->addWidget(wordlistEdit);
    setLayout(layout);
}

/*
void WordlistVerifyPage::keyPressEvent(QKeyEvent* event)
{
    QWizardPage::keyPressEvent(event);
}
*/

bool WordlistVerifyPage::isComplete() const
{
    // TODO: convert to lowercase and disallow invalid characters and additional whitespace
    return (wordlist == wordlistEdit->text());
}

WordlistCompletePage::WordlistCompletePage(int i, QWidget* parent)
    : QWizardPage(parent)
{
    setTitle(tr("Step ") + QString::number(i) + tr(": Put in safe, secure place"));

    QLabel* promptLabel = new QLabel(tr("IMPORTANT: Keep all wordlists in a safe place."));
    promptLabel->setWordWrap(true);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(promptLabel);
    setLayout(layout);
}
