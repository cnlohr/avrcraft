#ifndef UTIL10_H
#define UTIL10_H

#include <stdint.h>

//Warning: this library's 'StrTack' function uses memcpy and strlen.  Use sparingly.

//Pass in a buffer of at least 10 bytes.  number -> string
//Warning: This currently uses a (uint32_t)/(uint8_t) operation!
void Uint32To10Str( char * sto, uint32_t indata );

//Pass in a buffer of at least 4 bytes. number->string.  The string will _always_ be three bytes long with a null.
void Uint8To10Str( char * str, uint8_t val );

//Convert an int to hex.  Pass in 3-byte buffer.  number->hexstring
void Uint8To16Str( char * str, uint8_t val );

/*
//XXX This function should really leave!!!
//Tack two strings together.
void StrTack( char * str, uint16_t * optr, const char * strin );

//Only strin is a pgmstr
void PgmStrTack( char * str, uint16_t * optr, const char * strin );

//Only strin is a pgmstr
void PgmStrTack( char * str, uint16_t * optr, const char * strin );
*/

#endif
