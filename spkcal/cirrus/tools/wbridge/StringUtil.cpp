////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2015 Cirrus Logic International (UK) Ltd.  All rights reserved.
//
// This software as well as any related documentation is furnished under 
// license and may only be used or copied in accordance with the terms of the 
// license. The information in this file is furnished for informational use 
// only, is subject to change without notice, and should not be construed as 
// a commitment by Cirrus Logic International (UK) Ltd.  Cirrus Logic International
// (UK) Ltd assumes no responsibility or liability for any errors or inaccuracies
// that may appear in this document or any software that may be provided in
// association with this document. 
//
// Except as permitted by such license, no part of this document may be 
// reproduced, stored in a retrieval system, or transmitted in any form or by 
// any means without the express written consent of Cirrus Logic International
// (UK) Ltd or affiliated companies.
//
/// @file   StringUtil.cpp
/// @brief  Utility functions for std::string.
///
/// @version \$Id: StringUtil.cpp 16820 2015-01-09 13:47:56Z shackman $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////

//
// Include files
//
#include "StringUtil.h"
#include <string.h>
#include <stdlib.h>

//
// Global definitions
//
#ifdef _WIN32
//
// We have to build on Linux as well, so we can't use the secure versions
// of the CRTL functions.
//
#pragma warning( disable: 4996 )
#endif

////////////////////////////////////////////////////////////////////////////////
///
//  Function: SplitString
///
/// @brief  Splits a string on the specified delimiter.
///
/// Note: removes empty entries.
///
/// @param  str             String to split.
/// @param  delimiterStr    String containing the delimiter(s).
/// @param  pTokens         Receives the split tokens.
/// @param  matchAny        If false (default), matches delimiterStr exactly.
///                         If true, matches any character in delimiterStr.
///
/// @return void.
///
////////////////////////////////////////////////////////////////////////////////
void SplitString( std::string               str,
                  const char                *delimiterStr,
                  std::vector<std::string>  *pTokens,
                  bool                      matchAny /* = false */
                )
{
    // If we don't have a delimiter string, return the full string.
    if ( !delimiterStr || 0 == strlen( delimiterStr ) )
    {
        pTokens->push_back( str );
        goto done;
    }

    //
    // Otherwise, tokenize the string.
    //
    if ( matchAny )
    {
        std::string token;
        char        *strToTokenise = new char[str.length() + 1];
        char        *tokenStr;

        // We need a non-const version of str to tokenise.
        strcpy( strToTokenise, str.c_str() );

        tokenStr = strtok( strToTokenise, delimiterStr );
        while ( tokenStr != NULL )
        {
            token = tokenStr;
            if ( !token.empty() )
                pTokens->push_back( token );

            tokenStr = strtok( NULL, delimiterStr );
        }

        // Clean-up.
        if ( strToTokenise )
            delete [] strToTokenise;
    }
    else
    {
        size_t  startPos = 0;
        size_t  endPos = str.find( delimiterStr );

        while ( startPos < str.length() )
        {        
            if ( NPOS == endPos )
            {
                // This is the last string - we're done.
                pTokens->push_back( str.substr( startPos ) );
                goto done;
            }
            else
            {
                std::string substring = str.substr( startPos, endPos - startPos );
                if ( !substring.empty() )
                    pTokens->push_back( substring );
            }

            startPos = endPos + strlen( delimiterStr );
            endPos = str.find( delimiterStr, startPos );
        }
    }

done:
    return;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: StripString
///
/// @brief  Strips the delimiter characters from a string.
///
/// @param  str             String to split.
/// @param  delimiterStr    String containing the delimiter(s).
///
/// @return void
///
////////////////////////////////////////////////////////////////////////////////
void StripString( std::string &str, const char *delimiterStr )
{
    std::string strippedString = "";
    char        *strToTokenise = new char[str.length() + 1];
    char        *tokenStr;

    // We need a non-const version of str to tokenise.
    strcpy( strToTokenise, str.c_str() );

    //
    // Tokenise the string and append the tokens together to get the new string
    // with the delimiter char(s) removed.
    //
    tokenStr = strtok( strToTokenise, delimiterStr );
    while ( tokenStr != NULL )
    {
        strippedString += tokenStr;

        tokenStr = strtok( NULL, delimiterStr );
    }

    // Clean-up.
    if ( strToTokenise )
        delete [] strToTokenise;

    // Set str to our stripped string.
    str = strippedString;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: FindFirstNotOf
///
/// @brief  Finds the first character that does not match any of the characters specified.
///
/// Note: Android only supports find_first_not_of( char c, size_t pos = 0 ),
/// rather than the multiple-search-character version
/// find_first_not_of( const char *s, size_t pos = 0 ) which we want. Thus, the
/// implementation here.
///
/// @param  str         String to search.
/// @param  charStr     String containing the characters to use in the search.
/// @param  startPos    Position of the first character in the string to be
///                     considered in the search. If this is greater than the
///                     the string length, the function never finds matches.
///
/// @return The position of the first character that does not match. If no such
/// characters are found, the function returns std::string::npos.
///
////////////////////////////////////////////////////////////////////////////////
size_t FindFirstNotOf( std::string str, const char *charStr, size_t startPos /*= 0 */ )
{
    size_t returnPos = NPOS;

    // If the start position is greater than the string's length, we're done.
    if ( startPos >= str.length() )
        goto done;

    // If we don't have a character string, just return the startPos.
    if ( !charStr || 0 == strlen( charStr ) )
    {
        returnPos = startPos;
        goto done;
    }

    for ( size_t pos = startPos; pos < str.length(); pos++ )
    {
        //
        // If we don't find the current position's character in the character
        // string, we've found the first character that doesn't match.
        //
        if ( NULL == strchr( charStr, str[pos] ) )
        {
            returnPos = pos;
            goto done;
        }
    }

done:
    return returnPos;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: IsDigit
///
/// @brief  Checks whether the given character is a decimal digit.
///
/// @param  character   Character to be checked, casted to an int.
///
/// @return True iff the character is a decimal digit [0-9].
///
////////////////////////////////////////////////////////////////////////////////
bool IsDigit( int character )
{
    bool isDigit = false;

    if ( character >= '0' && character <= '9' )
        isDigit = true;

    return isDigit;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: IsHexDigit
///
/// @brief  Checks whether the given character is a hexadecimal digit.
///
/// Note: Android doesn't support isxdigit. Thus, the implementation here.
///
/// @param  character   Character to be checked, casted to an int.
///
/// @return True iff the character is a hexadecimal digit [0-9,a-f,A-F].
///
////////////////////////////////////////////////////////////////////////////////
bool IsHexDigit( int character )
{
    bool isHex = false;

    if ( ( character >= '0' && character <= '9' ) ||
         ( character >= 'a' && character <= 'f' ) || 
         ( character >= 'A' && character <= 'F' )
       )
    {
        isHex = true;
    }

    return isHex;
}

////////////////////////////////////////////////////////////////////////////////
///
//  Function: IsAlphanumeric
///
/// @brief  Checks whether the given character is alphanumeric.
///
/// Note: Android doesn't support isalnum. Thus, the implementation here.
///
/// @param  character   Character to be checked, casted to an int.
///
/// @return True iff the character is alphanumeric [0-9,a-z,A-Z].
///
////////////////////////////////////////////////////////////////////////////////
bool IsAlphanumeric( int character )
{
    bool isAlphanumeric = false;

    if ( ( character >= '0' && character <= '9' ) ||
         ( character >= 'a' && character <= 'z' ) || 
         ( character >= 'A' && character <= 'Z' )
       )
    {
        isAlphanumeric = true;
    }

    return isAlphanumeric;
}

///////////////////////////////// END OF FILE //////////////////////////////////