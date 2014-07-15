///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// createtxdialogview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "createtxdialog.h"
#include "numberformats.h"
#include "currencyvalidator.h"

#include "settings.h"
#include "coinparams.h"

#include <stdutils/uchar_vector.h>
#include <CoinQ/CoinQ_script.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

#include <QMessageBox>

#include <stdexcept>

TxOutLayout::TxOutLayout(uint64_t currencyDivisor, const QString& currencySymbol, uint64_t currencyMax, unsigned int currencyDecimals, QWidget* parent)
    : QHBoxLayout(parent)
{
    // Base58 version bytes
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();

    // Coin parameters
    this->currencyDivisor = currencyDivisor;
    this->currencySymbol = currencySymbol;
    this->currencyMax = currencyMax;
    this->currencyDecimals = currencyDecimals;

    QLabel* addressLabel = new QLabel(tr("Address:"));
    addressEdit = new QLineEdit();
    addressEdit->setFixedWidth(300);

    QLabel* amountLabel = new QLabel(tr("Amount") + " (" + currencySymbol + "):");
    amountEdit = new QLineEdit();
    amountEdit->setFixedWidth(100);
    amountEdit->setValidator(new CurrencyValidator(currencyMax, currencyDecimals, this));

    QLabel* recipientLabel = new QLabel(tr("For:"));
    recipientEdit = new QLineEdit();
    recipientEdit->setFixedWidth(100);

    removeButton = new QPushButton(tr("Remove"));

    setSizeConstraint(QLayout::SetFixedSize);
    addWidget(addressLabel);
    addWidget(addressEdit);
    addWidget(amountLabel);
    addWidget(amountEdit);
    addWidget(recipientLabel);
    addWidget(recipientEdit);
    addWidget(removeButton);
}

inline void TxOutLayout::setAddress(const QString& address)
{
    addressEdit->setText(address);
}

inline QString TxOutLayout::getAddress() const
{
    return addressEdit->text();
}

bytes_t TxOutLayout::getScript() const
{
    bytes_t script = CoinQ::Script::getTxOutScriptForAddress(getAddress().toStdString(), base58_versions);
    return script;
}

void TxOutLayout::setValue(uint64_t value)
{
    amountEdit->setText(QString::number(value/(1.0 * currencyDivisor), 'g', 8));
}

uint64_t TxOutLayout::getValue() const
{
    return decimalStringToInteger(amountEdit->text().toStdString(), currencyMax, currencyDivisor, currencyDecimals);
}

// TODO: call this field the label rather than the recipient
void TxOutLayout::setRecipient(const QString& recipient)
{
    recipientEdit->setText(recipient);
}

inline QString TxOutLayout::getRecipient() const
{
    return recipientEdit->text();
}

void TxOutLayout::removeWidgets()
{
    while(!isEmpty()) {
        delete itemAt(0)->widget();
    }
}

CreateTxDialog::CreateTxDialog(const QString& accountName, const PaymentRequest& paymentRequest, QWidget* parent)
    : QDialog(parent), status(SAVE_ONLY)
{
    // Coin parameters
    currencyDivisor = getCoinParams().currency_divisor();
    currencySymbol = getCoinParams().currency_symbol();
    currencyMax = getCoinParams().currency_max();
    currencyDecimals = getCoinParams().currency_decimals();

    // Buttons
    signAndSendButton = new QPushButton(tr("Sign and Send"));
    signAndSaveButton = new QPushButton(tr("Sign and Save"));
    saveButton = new QPushButton(tr("Save Unsigned"));
    cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setDefault(true);
/*
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
                                        QDialogButtonBox::Ok |
                                        QDialogButtonBox::Cancel);

*/
    QDialogButtonBox* buttonBox = new QDialogButtonBox;
    buttonBox->addButton(signAndSendButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(signAndSaveButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(saveButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);

    connect(signAndSendButton, &QPushButton::clicked, [this]() { status = SIGN_AND_SEND; accept(); });
    connect(signAndSaveButton, &QPushButton::clicked, [this]() { status = SIGN_AND_SAVE; accept(); });
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Prompt
    QLabel* promptLabel = new QLabel(tr("Add Outputs:"));

    // Account
    QLabel* accountLabel = new QLabel(tr("Account:"));
    accountComboBox = new QComboBox();
    accountComboBox->insertItem(0, accountName);

    QHBoxLayout* accountLayout = new QHBoxLayout();
    accountLayout->addWidget(accountLabel);
    accountLayout->addWidget(accountComboBox);

    // Fee
    QLabel* feeLabel = new QLabel(tr("Fee") + " (" + currencySymbol + "):");
    feeEdit = new QLineEdit();
    feeEdit->setValidator(new CurrencyValidator(currencyMax, currencyDecimals, this));
    feeEdit->setText("0.0005"); // TODO: suggest more intelligently

    QHBoxLayout* feeLayout = new QHBoxLayout();
    feeLayout->addWidget(feeLabel);
    feeLayout->addWidget(feeEdit);

    // TxOuts
    txOutVBoxLayout = new QVBoxLayout();
    txOutVBoxLayout->setSizeConstraint(QLayout::SetMinimumSize);
    addTxOut(paymentRequest);

    // Add Output Button
    QPushButton* addTxOutButton = new QPushButton(tr("Add Output"));
    connect(addTxOutButton, SIGNAL(clicked()), this, SLOT(addTxOut()));

    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(promptLabel);
    mainLayout->addLayout(accountLayout);
    mainLayout->addLayout(feeLayout);
    mainLayout->addLayout(txOutVBoxLayout);
    mainLayout->addWidget(addTxOutButton);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}


QString CreateTxDialog::getAccountName() const
{
    return accountComboBox->currentText();
}

uint64_t CreateTxDialog::getFeeValue() const
{
    return decimalStringToInteger(feeEdit->text().toStdString(), currencyMax, currencyDivisor, currencyDecimals);
}

std::vector<std::shared_ptr<CoinDB::TxOut>> CreateTxDialog::getTxOuts()
{
    std::vector<std::shared_ptr<CoinDB::TxOut>> txouts;
    std::set<TxOutLayout*> txOutLayoutsCopy = txOutLayouts;
    for (auto& txOutLayout: txOutLayoutsCopy) {
        if (txOutLayout->getAddress().isEmpty()) {
            removeTxOut(txOutLayout);
            continue;
        }
        std::shared_ptr<CoinDB::TxOut> txout(new CoinDB::TxOut(txOutLayout->getValue(), txOutLayout->getScript()));
        txout->sending_label(txOutLayout->getRecipient().toStdString());
        txouts.push_back(txout);
    }

    if (txouts.empty()) {
        addTxOut();
        throw std::runtime_error("No outputs entered.");
    }
    return txouts;
}

std::vector<TaggedOutput> CreateTxDialog::getOutputs()
{
    std::vector<TaggedOutput> outputs;
    std::set<TxOutLayout*> txOutLayoutsCopy = txOutLayouts;
    for (auto& txOutLayout: txOutLayoutsCopy) {
        if (txOutLayout->getAddress().isEmpty()) {
            removeTxOut(txOutLayout);
            continue;
        }
        TaggedOutput output(txOutLayout->getScript(), txOutLayout->getValue(), txOutLayout->getRecipient().toStdString());
        outputs.push_back(output);
    }
 
    if (outputs.empty()) {
        addTxOut();
        throw std::runtime_error("No outputs entered.");
    }
    return outputs;
}

void CreateTxDialog::addTxOut(const PaymentRequest& paymentRequest)
{
    TxOutLayout* txOutLayout = new TxOutLayout(currencyDivisor, currencySymbol, currencyMax, currencyDecimals);
    txOutLayouts.insert(txOutLayout);
    QPushButton* removeButton = txOutLayout->getRemoveButton();
    removeButton->setEnabled(txOutLayouts.size() > 1);
    connect(removeButton, &QPushButton::clicked, [=]() { this->removeTxOut(txOutLayout); });
    txOutVBoxLayout->addLayout(txOutLayout);
    nOutputs++;

    if (paymentRequest.hasAddress()) {
        txOutLayout->setAddress(paymentRequest.address());
    }

    if (paymentRequest.hasValue()) {
        txOutLayout->setValue(paymentRequest.value());
    }

    if (paymentRequest.hasLabel()) {
        txOutLayout->setRecipient(paymentRequest.label());
    }
}

void CreateTxDialog::removeTxOut(TxOutLayout* txOutLayout)
{
    txOutLayouts.erase(txOutLayout);
    if (txOutLayouts.size() == 1)
    {
        txOutLayouts.begin()->getRemoveButton()->setEnabled(false);
    }
    txOutLayout->removeWidgets();
    delete txOutLayout;
}

