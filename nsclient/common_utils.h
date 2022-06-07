#pragma once

#include "common_includes.h"

bool is_digit(const unsigned char c);
bool is_hyphen(const unsigned char c);
bool is_letter(const unsigned char c);
bool is_let_dig_hyp(const unsigned char c);
bool is_let_dig(const unsigned char c);
bool str_check_all(const unsigned char* s, bool (*is_char_condition)(const unsigned char c));
void printAsBytes(const unsigned char* s, size_t sizeof_s);