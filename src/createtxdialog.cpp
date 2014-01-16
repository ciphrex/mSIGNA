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

#include "settings.h"
#include "coinparams.h"

#include <uchar_vector.h>
#include <CoinQ_script.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QLabel>

#include <QMessageBox>

#include <stdexcept>

/*
// TODO: move all display/formatting stuff to its own module

static const QRegExp AMOUNT_REGEXP("(([1-9]\\d{0,6}|1\\d{7}|20\\d{6}|0|)(\\.\\d{0,8})?|21000000(\\.0{0,8})?)");

// disallow more than 8 decimals and amounts > 21 million
static uint64_t btcStringToSatoshis(const std::string& btcString)
{
    uint32_t whole = 0;
    uint32_t frac = 0;

    unsigned int i = 0;
    bool stateWhole = true;
    for (auto& c: btcString) {
        i++;
        if (stateWhole) {
            if (c == '.') {
                stateWhole = false;
                i = 0;
                continue;
            }
            else if (i > 8 || c < '0' || c > '9') {
                throw std::runtime_error("Invalid amount.");
            }
            whole *= 10;
            whole += (uint32_t)(c - '0');
        }
        else {
            if (i > 8 || c < '0' || c > '9') {
                throw std::runtime_error("Invalid amount.");
            }
            frac *= 10;
            frac += (uint32_t)(c - '0');
        }
    }
    if (frac > 0) {
        while (i < 8) {
            i++;
            frac *= 10;
        }
    }
    uint64_t value = (uint64_t)whole * 100000000ull + frac;
    if (value > 2100000000000000ull) {
        throw std::runtime_error("Invalid amount.");
    }

    return value;
}
*/
TxOutLayout::TxOutLayout(QWidget* parent)
    : QHBoxLayout(parent)
{
    // Base58 version bytes
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();

    QLabel* addressLabel = new QLabel(tr("Address:"));
    addressEdit = new QLineEdit();
    addressEdit->setFixedWidth(300);

    QLabel* amountLabel = new QLabel(tr("Amount:"));
    amountEdit = new QLineEdit();
    amountEdit->setValidator(new QRegExpValidator(AMOUNT_REGEXP, this));

    QLabel* recipientLabel = new QLabel(tr("For:"));
    recipientEdit = new QLineEdit();

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
    amountEdit->setText(QString::number(value/100000000.0, 'g', 8));
}

uint64_t TxOutLayout::getValue() const
{
    return btcStringToSatoshis(amountEdit->text().toStdString());
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
    QLabel* feeLabel = new QLabel(tr("Fee:"));
    feeEdit = new QLineEdit();
    feeEdit->setValidator(new QRegExpValidator(AMOUNT_REGEXP));
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
    return btcStringToSatoshis(feeEdit->text().toStdString());
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
    TxOutLayout* txOutLayout = new TxOutLayout();
    txOutLayouts.insert(txOutLayout);
    QPushButton* removeButton = txOutLayout->getRemoveButton();
    connect(removeButton, &QPushButton::clicked, [=]() { this->removeTxOut(txOutLayout); });
    txOutVBoxLayout->addLayout(txOutLayout);

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
    txOutLayout->removeWidgets();
    delete txOutLayout;
}

