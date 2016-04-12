///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// docdir.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

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
