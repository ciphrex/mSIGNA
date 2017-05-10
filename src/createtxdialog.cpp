///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// createtxdialogview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "createtxdialog.h"
#include "unspenttxoutmodel.h"
#include "unspenttxoutview.h"
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
#include <QCheckBox>
#include <QLabel>

#include <QMessageBox>

#include <stdexcept>

const int COIN_CONTROL_VIEW_MIN_WIDTH = 700;
const int COIN_CONTROL_VIEW_MIN_HEIGHT = 200;

TxOutLayout::TxOutLayout(uint64_t currencyDivisor, const QString& currencySymbol, uint64_t currencyMax, unsigned int currencyDecimals, QWidget* parent)
    : QHBoxLayout(parent)
{
    // Base58 version bytes
    base58_versions[0] = getCoinParams().pay_to_pubkey_hash_version();
#ifdef SUPPORT_OLD_ADDRESS_VERSIONS
    base58_versions[1] = getUseOldAddressVersions() ? getCoinParams().old_pay_to_script_hash_version() : getCoinParams().pay_to_script_hash_version();
#else
    base58_versions[1] = getCoinParams().pay_to_script_hash_version();
#endif

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
    amountEdit->setAlignment(Qt::AlignRight);
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

CoinControlWidget::CoinControlWidget(CoinDB::Vault* vault, const QString& accountName, QWidget* parent)
    : QWidget(parent)
{
    model = new UnspentTxOutModel(vault, accountName, this);
    view = new UnspentTxOutView(this);
    view->setModel(model);
    view->setMinimumWidth(COIN_CONTROL_VIEW_MIN_WIDTH);
    view->setMinimumHeight(COIN_CONTROL_VIEW_MIN_HEIGHT);

    QItemSelectionModel* selectionModel = view->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            this, &CoinControlWidget::updateTotal);

    QLabel* inputsLabel = new QLabel(tr("Select Inputs:"));

    QLabel* totalLabel = new QLabel(tr("Input total") + " (" + getCurrencySymbol() + "):");
    totalEdit = new QLineEdit();
    totalEdit->setAlignment(Qt::AlignRight);
    totalEdit->setReadOnly(true);
    totalEdit->setText("0");

    QHBoxLayout* totalLayout = new QHBoxLayout();
    totalLayout->addWidget(totalLabel);
    totalLayout->addWidget(totalEdit);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->addWidget(inputsLabel);
    layout->addWidget(view);
    layout->addLayout(totalLayout);
    setLayout(layout);
}

std::vector<unsigned long> CoinControlWidget::getInputTxOutIds() const
{
    std::vector<unsigned long> ids;

    QItemSelectionModel* selectionModel = view->selectionModel();
    QModelIndexList indexes = selectionModel->selectedRows(0);

    for (auto& index: indexes)
    {
        QStandardItem* item = model->item(index.row(), 1);
        ids.push_back((unsigned long)item->data(Qt::UserRole).toInt());
    }

    return ids; 
}

void CoinControlWidget::updateAll()
{
    if (model)  { model->update(); }
    if (view)   { view->update(); }
}

void CoinControlWidget::refreshView()
{
    if (!isHidden())
    {
        hide();
        show();
    }
}

void CoinControlWidget::updateTotal(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/)
{
    QItemSelectionModel* selectionModel = view->selectionModel();
    QModelIndexList indexes = selectionModel->selectedRows(0);

    uint64_t total = 0;
    for (auto& index: indexes)
    {
        QStandardItem* amountItem = model->item(index.row(), 0);
        QString strAmount = amountItem->data(Qt::UserRole).toString();
        total += strtoull(strAmount.toStdString().c_str(), NULL, 10);
    }

    //QString amount(QString::number(total/(1.0 * model->getCurrencyDivisor()), 'g', 8));
    QString amount(getFormattedCurrencyAmount(total));
    totalEdit->setText(amount);
}

CreateTxDialog::CreateTxDialog(CoinDB::Vault* vault, const QString& accountName, const PaymentRequest& paymentRequest, QWidget* parent)
    : QDialog(parent), status(SAVE)
{
    // Coin parameters
    currencyDivisor = getCurrencyDivisor(); //getCoinParams().currency_divisor();
    currencySymbol = getCurrencySymbol(); //getCoinParams().currency_symbol();
    currencyMax = getCurrencyMax(); //getCoinParams().currency_max();
    currencyDecimals = getCurrencyDecimals(); //getCoinParams().currency_decimals();
    defaultFee = getDefaultFee(); // getCoinParams().default_fee();

    // Buttons
    signButton = new QPushButton(tr("Sign"));
    saveButton = new QPushButton(tr("Save Unsigned"));
    cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setDefault(true);

    QDialogButtonBox* buttonBox = new QDialogButtonBox;
    buttonBox->addButton(signButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(saveButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);

    connect(signButton, &QPushButton::clicked, [this]() { status = SIGN; accept(); });
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

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
    feeEdit->setText(getFormattedCurrencyAmount(defaultFee, HIDE_TRAILING_DECIMALS)); // TODO: suggest more intelligently

    QHBoxLayout* feeLayout = new QHBoxLayout();
    feeLayout->addWidget(feeLabel);
    feeLayout->addWidget(feeEdit);

    // Coin control
    coinControlCheckBox = new QCheckBox(tr("Enable Coin Control"));
    coinControlCheckBox->setChecked(false);
    coinControlWidget = new CoinControlWidget(vault, accountName, this);
    coinControlWidget->hide();
    connect(coinControlCheckBox, SIGNAL(stateChanged(int)), this, SLOT(switchCoinControl(int)));

    // Prompt
    QLabel* addOutputsLabel = new QLabel(tr("Add Outputs:"));

    // TxOuts
    txOutVBoxLayout = new QVBoxLayout();
    txOutVBoxLayout->setSizeConstraint(QLayout::SetMinimumSize);
    addTxOut(paymentRequest);

    // Add Output Button
    QPushButton* addTxOutButton = new QPushButton(tr("Add Output"));
    connect(addTxOutButton, SIGNAL(clicked()), this, SLOT(addTxOut()));

    mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(accountLayout);
    mainLayout->addLayout(feeLayout);
    mainLayout->addWidget(coinControlCheckBox);
    mainLayout->addWidget(coinControlWidget);
    mainLayout->addWidget(addOutputsLabel);
    mainLayout->addLayout(txOutVBoxLayout);
    mainLayout->addWidget(addTxOutButton);
    mainLayout->addWidget(buttonBox);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
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

std::vector<unsigned long> CreateTxDialog::getInputTxOutIds() const
{
    if (!coinControlCheckBox->isChecked()) return std::vector<unsigned long>();
    return coinControlWidget->getInputTxOutIds();
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

void CreateTxDialog::switchCoinControl(int state)
{
    if (state == Qt::Checked)
    {
        coinControlWidget->updateAll();
        coinControlWidget->show();
    }
    else
    {
        coinControlWidget->hide();
    }
}

void CreateTxDialog::addTxOut(const PaymentRequest& paymentRequest)
{
    TxOutLayout* txOutLayout = new TxOutLayout(currencyDivisor, currencySymbol, currencyMax, currencyDecimals);
    txOutLayouts.insert(txOutLayout);
    setRemoveEnabled(txOutLayouts.size() > 1);
    QPushButton* removeButton = txOutLayout->getRemoveButton();
    connect(removeButton, &QPushButton::clicked, [=]() { this->removeTxOut(txOutLayout); });
    txOutVBoxLayout->addLayout(txOutLayout);

    // Windows repaint workaround
    coinControlWidget->refreshView();

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
    setRemoveEnabled(txOutLayouts.size() > 1);
    txOutLayout->removeWidgets();
    txOutVBoxLayout->removeItem(txOutLayout);    
    delete txOutLayout;

    // Windows repaint workaround
    coinControlWidget->refreshView();
}


void CreateTxDialog::setRemoveEnabled(bool enabled)
{
    for (auto& layout: txOutLayouts)
    {
        layout->getRemoveButton()->setEnabled(enabled);
    } 
}

