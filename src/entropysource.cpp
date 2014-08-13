///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// entropydialog.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#include "entropysource.h"
#include "entropydialog.h"

#include <CoinCore/random.h>

#include <QApplication>

#include <thread>
#include <chrono>
#include <stdexcept>

using namespace std;

class EntropySource
{
public:
    EntropySource() : m_bSeeded(false) { }
    ~EntropySource() { if (m_thread.joinable()) m_thread.detach(); }

    bool isSeeded() const { return m_bSeeded; }
    void seed(bool reseed = false);
    void join() { if (m_thread.joinable()) m_thread.join(); }

private:
    bool m_bSeeded;
    thread m_thread;
};

void EntropySource::seed(bool reseed)
{
    if (m_bSeeded && !reseed) return;

    if (!m_thread.joinable())
    {
        m_thread = thread([&]() {
            secure_random_bytes(32);
            m_bSeeded = true;
        });
    }
}

static EntropySource entropySource;

void seedEntropySource(bool reseed, QWidget* parent)
{
    if (entropySource.isSeeded() && !reseed) return;

    EntropyDialog dlg(parent);
    dlg.setModal(true);
    dlg.setResult(QDialog::Accepted);
    dlg.show();

    entropySource.seed(reseed);

    while (!entropySource.isSeeded())
    {
        qApp->processEvents();
        if (dlg.result() == QDialog::Rejected) throw runtime_error("Entropy seeding operation canceled.");
        this_thread::sleep_for(std::chrono::microseconds(200)); 
    }

    entropySource.join();
    dlg.hide();
}

secure_bytes_t getRandomBytes(int n, QWidget* parent)
{
    seedEntropySource(false, parent);
    return secure_random_bytes(n);
}

