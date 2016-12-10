///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// entropysource.cpp
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "entropysource.h"
#include "entropydialog.h"

#include <logger/logger.h>

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

    bool isSeeded() const { return m_bSeeded; }
    void seed(bool reseed = false);
    void join();

private:
    bool m_bSeeded;
    thread m_thread;
};

void EntropySource::seed(bool reseed)
{
    if (m_bSeeded && !reseed) return;

    if (!m_thread.joinable())
    {
        LOGGER(trace) << "Starting entropy thread." << std::endl;
        m_thread = thread([&]() {
            secure_random_bytes(32);
            m_bSeeded = true;
        });
    }
}

void EntropySource::join()
{
    if (m_thread.joinable())
    {
        LOGGER(trace) << "Joining entropy thread (2)..." << std::endl;
        m_thread.join();
        LOGGER(trace) << "Entropy thread has exited (2)." << std::endl;
    }
    else
    {
        LOGGER(trace) << "Entropy thread has already exited." << std::endl;
    }
}

static EntropySource entropySource;

void seedEntropySource(bool reseed, bool showDialog, QWidget* parent)
{
    if (entropySource.isSeeded() && !reseed) return;

    EntropyDialog dlg(parent);

    if (showDialog)
    {
        dlg.setModal(true);
        dlg.setResult(QDialog::Accepted);
        dlg.show();
    }

    entropySource.seed(reseed);

    while (!entropySource.isSeeded())
    {
        qApp->processEvents();
        if (showDialog && dlg.result() == QDialog::Rejected) throw runtime_error("Entropy seeding operation canceled.");
        this_thread::sleep_for(std::chrono::microseconds(200)); 
    }

    LOGGER(trace) << "Joining entropy thread (1)..." << std::endl;
    entropySource.join();
    LOGGER(trace) << "Entropy thread has exited (1)." << std::endl;

    if (showDialog)
    {
        dlg.hide();
    }
}

void joinEntropyThread()
{
    entropySource.join();
}

secure_bytes_t getRandomBytes(int n, QWidget* parent)
{
    //seedEntropySource(false, parent);
    return secure_random_bytes(n);
}

