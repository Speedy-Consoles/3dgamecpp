/*
    AngelCode Tool Box Library
    Copyright (c) 2008 Andreas Jönsson

    This software is provided 'as-is', without any express or implied
    warranty. In no event will the authors be held liable for any
    damages arising from the use of this software.

    Permission is granted to anyone to use this software for any
    purpose, including commercial applications, and to alter it and
    redistribute it freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you
       must not claim that you wrote the original software. If you use
       this software in a product, an acknowledgment in the product
       documentation would be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and
       must not be misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
       distribution.

    Andreas Jönsson
    andreas@angelcode.com
*/

/*
    This file was modified from its original version!
    The original version could be obtained from
        http://www.angelcode.com/dev/bmfonts/
    as of 2015-03-22

    2015-03-22 - Lars Thorben Drögemüller
        Removed enclosing namespace and enforced coding conventions
*/

#ifndef UNICODE_INT_HPP
#define UNICODE_INT_HPP

enum UnicodeByteOrder {
    LITTLE_ENDIAN,
    BIG_ENDIAN,
};

// This function will attempt to decode a UTF-8 encoded character in the buffer.
// If the encoding is invalid, the function returns -1.
int decodeUTF8(const char *encodedBuffer, unsigned int *outCharLength);

// This function will encode the value into the buffer.
// If the value is invalid, the function returns -1, else the encoded length.
int encodeUTF8(unsigned int value, char *outEncodedBuffer, unsigned int *outCharLength);

// This function will attempt to decode a UTF-16 encoded character in the buffer.
// If the encoding is invalid, the function returns -1.
int decodeUTF16(const char *encodedBuffer, unsigned int *outCharLength, UnicodeByteOrder byteOrder = LITTLE_ENDIAN);

// This function will encode the value into the buffer.
// If the value is invalid, the function returns -1, else the encoded length.
int encodeUTF16(unsigned int value, char *outEncodedBuffer, unsigned int *outCharLength, UnicodeByteOrder byteOrder = LITTLE_ENDIAN);

#endif // UNICODE_INT_HPP