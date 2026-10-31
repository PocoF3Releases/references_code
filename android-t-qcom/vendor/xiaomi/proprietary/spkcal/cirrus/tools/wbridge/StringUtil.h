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
/// @file   StringUtil.h
/// @brief  Utility functions for std::string.
///
/// @version \$Id: StringUtil.h 16820 2015-01-09 13:47:56Z shackman $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////
#ifndef __STRINGUTIL_H__
#define __STRINGUTIL_H__

//
// Include files
//
#include <string>
#include <vector>

//
// Definitions
//
#define NPOS                            std::string::npos

#define LINE_CONTAINS( _line, _string ) ( NPOS != _line.find( _string ) )

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
///
//  Function: SplitString
///
/// @brief  Splits a string on the specified delimiter.
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
                  bool                      matchAny = false
                );

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
void StripString( std::string &str, const char *delimiterStr );

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
/// characters are found, the function returns string::npos.
///
////////////////////////////////////////////////////////////////////////////////
size_t FindFirstNotOf( std::string str, const char *charStr, size_t startPos = 0 );

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
bool IsDigit( int character );

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
bool IsHexDigit( int character );

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
bool IsAlphanumeric( int character );

////////////////////////////////////////////////////////////////////////////////
// Inline functions
////////////////////////////////////////////////////////////////////////////////


#endif  // __STRINGUTIL_H__
///////////////////////////////// END OF FILE //////////////////////////////////
