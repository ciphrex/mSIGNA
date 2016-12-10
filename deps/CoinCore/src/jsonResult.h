////////////////////////////////////////////////////////////////////////////////
//
// jsonResult.h
//
// Helper functions for outputting json
//
// Copyright (c) 2011 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#ifndef _JSON_RESULT_H__
#define _JSON_RESULT_H__

#include <iostream>
#include <string>

void showJsonError(const std::string& error)
{
    std::cout << "{\"status\":\"error\",\"error\":\"" << std::error << "\"}";
}

void showJsonSuccess(const std::string& keysValues)
{
    std::cout << "{" << keysValues << ",\"status\":\"success\"}";
}

#endif // _JSON_RESULT_H__
