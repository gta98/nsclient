#pragma once

#include "common_includes.h"


bool is_digit(const unsigned char c);
// Checks whether or not, according to RFC1035, c is a digit

bool is_hyphen(const unsigned char c);
// Checks whether or not, according to RFC1035, c is a hyphen

bool is_letter(const unsigned char c);
// Checks whether or not, according to RFC1035, c is a letter

bool is_let_dig_hyp(const unsigned char c);
// Checks whether or not, according to RFC1035, c is a letter or a digit or a hyphen

bool is_let_dig(const unsigned char c);
// Checks whether or not, according to RFC1035, c is a letter or a digit

bool str_check_all(const unsigned char* s, bool (*is_char_condition)(const unsigned char c));
// Checks whether or not all chars in s return true for is_char_condition

int removeSignificantBit(int num);
// Removes significant bit - taken from https://kalkicode.com/remove-significant-set-bit-number

void printAsBytes(const unsigned char* s, size_t sizeof_s);
// (enabled iff FLAG_DEBUG == 1) Prints "bytes([...])" for debugging with Python