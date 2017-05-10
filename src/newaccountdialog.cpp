///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// newaccountdialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "newaccountdialog.h"
#include "coinparams.h"

#include <QtAlgorithms>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QDateTimeEdit>
#include <QCalendarWidget>

#include <stdexcept>

NewAccountDialog::NewAccountDialog(const QList<QString>& allKeychains, const QList<QString>& selectedKeychains, QWidget* parent)
    : QDialog(parent)
{
    if (allKeychains.isEmpty()) {
        throw std::runtime_error(tr("You must first create at least one keychain.").toStdString());
    }

    keychainSet = selectedKeychains.toSet();

    QList<QString> keychains = allKeychains;
    qSort(keychains.begin(), keychains.end());

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);
    okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Account Name
    QLabel* nameLabel = new QLabel();
    nameLabel->setText(tr("Account Name:"));
    nameEdit = new QLineEdit();
    connect(nameEdit, &QLineEdit::textChanged, [this](const QString& /*text*/) { updateEnabled(); });

    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->setSizeConstraint(QLayout::SetNoConstraint);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);

    // Keychain List Widget
    QLabel* selectionLabel = new QLabel(tr("Select keychains:"));
    keychainListWidget = new QListWidget();
    for (auto& keychain: keychains)
    {
        QCheckBox* checkBox = new QCheckBox(keychain);
        checkBox->setCheckState(selectedKeychains.count(keychain) ? Qt::Checked : Qt::Unchecked);
        connect(checkBox, &QCheckBox::stateChanged, [=](int state) { updateSelection(keychain, state); });
        QListWidgetItem* item = new QListWidgetItem();
        keychainListWidget->addItem(item);
        keychainListWidget->setItemWidget(item, checkBox);
    }

    // Minimum Signatures
    QLabel *minSigLabel = new QLabel();
    minSigLabel->setText(tr("Minimum Signatures:"));

    minSigComboBox = new QComboBox();
    minSigLineEdit = new QLineEdit();
    minSigLineEdit->setAlignment(Qt::AlignRight);
    minSigComboBox->setLineEdit(minSigLineEdit);

    QHBoxLayout* minSigLayout = new QHBoxLayout();
    minSigLayout->setSizeConstraint(QLayout::SetNoConstraint);
    minSigLayout->addWidget(minSigLabel);
    minSigLayout->addWidget(minSigComboBox);
    updateMinSigs();

    // Creation Time
    QDateTime localDateTime = QDateTime::currentDateTime();
    QLabel* creationTimeLabel = new QLabel(tr("Creation Time ") + "(" + localDateTime.timeZoneAbbreviation() + "):");
    creationTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    creationTimeEdit->setDisplayFormat("yyyy.MM.dd hh:mm:ss");
    creationTimeEdit->setCalendarPopup(true);
    calendarWidget = new QCalendarWidget(this);
    creationTimeEdit->setCalendarWidget(calendarWidget);

    QHBoxLayout* creationTimeLayout = new QHBoxLayout();
    creationTimeLayout->setSizeConstraint(QLayout::SetNoConstraint);
    creationTimeLayout->addWidget(creationTimeLabel);
    creationTimeLayout->addWidget(creationTimeEdit);

    // Segwit Support
    if (getCoinParams().segwit_enabled())
    {
        segwitCheckBox = new QCheckBox(tr("Use Seg&Wit"), this);
        segwitCheckBox->setChecked(false);
    }
    else
    {
        segwitCheckBox = new QCheckBox(tr("Use Seg&Wit (not active on this blockchain)"), this);
        segwitCheckBox->setEnabled(false);
    }

    // Main Layout 
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addLayout(nameLayout);
    mainLayout->addWidget(selectionLabel);
    mainLayout->addWidget(keychainListWidget);
    mainLayout->addLayout(minSigLayout);
    mainLayout->addLayout(creationTimeLayout);
    mainLayout->addWidget(segwitCheckBox);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

NewAccountDialog::~NewAccountDialog()
{
    delete calendarWidget;
}

QString NewAccountDialog::getName() const
{
    return nameEdit->text();
}

int NewAccountDialog::getMinSigs() const
{
    return minSigComboBox->currentIndex() + 1;
}

qint64 NewAccountDialog::getCreationTime() const
{
    return creationTimeEdit->dateTime().toMSecsSinceEpoch();
}

bool NewAccountDialog::getUseSegwit() const
{
    return segwitCheckBox->isChecked();
}

void NewAccountDialog::updateSelection(const QString& keychain, int state)
{
    if (state == Qt::Checked)
    {
        keychainSet.insert(keychain);
    }
    else
    {
        keychainSet.remove(keychain);
    }

    updateMinSigs();
    updateEnabled();
}

void NewAccountDialog::updateMinSigs()
{
    int maxSigs = minSigComboBox->count();

    // Increase maximum
    for (int i = maxSigs + 1; i <= keychainSet.size(); i++)
    {
        minSigComboBox->addItem(QString::number(i));        
    }

    // Decrease maximum
    while (minSigComboBox->count() > keychainSet.size())
    {
        minSigComboBox->removeItem(minSigComboBox->count() - 1);
    }
}

void NewAccountDialog::updateEnabled()
{
    okButton->setEnabled(!nameEdit->text().isEmpty() && keychainSet.size() > 0);
}

