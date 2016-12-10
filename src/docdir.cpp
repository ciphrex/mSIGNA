///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// docdir.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "docdir.h"

QString docDir;

const QString& getDocDir()
{
    return docDir;
}

void setDocDir(const QString& dir)
{
    docDir = dir;
}
