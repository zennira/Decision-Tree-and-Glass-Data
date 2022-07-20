//          Copyright John W. Wilkinson 2007 - 2009.
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.03

#include "json_spirit_reader.h"
#include "json_spirit_reader_template.h"

using namespace json_spirit;

bool json_spirit::read( const std::string& s, Value& value )
{
    return read_string( s, value );
}

void json_spirit::read_or_throw( const std::string& s, Value& value )
{
    read_string_or_throw( s, value );
}

bool json_spirit: