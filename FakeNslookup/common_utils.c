#include "common_utils.h"

bool is_digit(const unsigned char c) {
	return ('0' <= c) && (c <= '9');
}

bool is_hyphen(const unsigned char c) {
	return c == '-';
}

bool is_letter(const unsigned char c) {
	return (('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z'));
}

bool is_let_dig_hyp(const unsigned char c) {
	return is_hyphen(c) || is_digit(c) || is_letter(c);
}

bool is_let_dig(const unsigned char c) {
	return is_letter(c) || is_digit(c);
}

bool str_check_all(const unsigned char* s, bool (*is_char_condition)(const unsigned char c)) {
	int i;
	size_t len;
	len = strlen(s);
	for (i = 0; i < len; i++) {
		if (!is_char_condition(s[i])) return false;
	}
	return true;
}