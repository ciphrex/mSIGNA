///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// quicknewaccountdialog.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "quicknewaccountdialog.h"
#include "coinparams.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QDateTimeEdit>
#include <QCalendarWidget>

#include <stdexcept>

static const int MAX_SIG_COUNT = 8;

QuickNewAccountDialog::QuickNewAccountDialog(QWidget* parent)
    : QDialog(parent)
{
    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Account Name
    QLabel* nameLabel = new QLabel();
    nameLabel->setText(tr("Account Name:"));
    nameEdit = new QLineEdit();

    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->setSizeConstraint(QLayout::SetNoConstraint);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);

    // Policy
    QLabel *policyLabel = new QLabel();
    policyLabel->setText(tr("Policy:"));
    QLabel *ofLabel = new QLabel();
    ofLabel->setText(tr("of"));

    minSigComboBox = new QComboBox();
    maxSigComboBox = new QComboBox();
    for (int i = 1; i <= MAX_SIG_COUNT; i++) {
        minSigComboBox->addItem(QString::number(i));
        maxSigComboBox->addItem(QString::number(i));
    }

    QHBoxLayout* policyLayout = new QHBoxLayout();
    policyLayout->setSizeConstraint(QLayout::SetNoConstraint);
    policyLayout->addWidget(policyLabel);
    policyLayout->addWidget(minSigComboBox);
    policyLayout->addWidget(ofLabel);
    policyLayout->addWidget(maxSigComboBox);

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
    mainLayout->addLayout(policyLayout);
    mainLayout->addLayout(creationTimeLayout);
    mainLayout->addWidget(segwitCheckBox);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

QuickNewAccountDialog::~QuickNewAccountDialog()
{
    delete calendarWidget;
}

QString QuickNewAccountDialog::getName() const
{
    return nameEdit->text();
}

int QuickNewAccountDialog::getMinSigs() const
{
    return minSigComboBox->currentIndex() + 1;
}

int QuickNewAccountDialog::getMaxSigs() const
{
    return maxSigComboBox->currentIndex() + 1;
}

qint64 QuickNewAccountDialog::getCreationTime() const
{
    return creationTimeEdit->dateTime().toMSecsSinceEpoch();
}

bool QuickNewAccountDialog::getUseSegwit() const
{
    return segwitCheckBox->isChecked();
}
