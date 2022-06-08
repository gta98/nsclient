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

void printAsBytes(const unsigned char* s, size_t sizeof_s) {
#if FLAG_DEBUG==1
	int i;
	printd("bytes([");
	for (i = 0; i < sizeof_s - 1; i++)
		printd("0x%x,", s[i]);
	printd("0x%x", s[sizeof_s - 1]);
	printd("])\n");
#else
	return;
#endif
}

//this function was copied from https://kalkicode.com/remove-significant-set-bit-number
int removeSignificantBit(int num) {
	if (num <= 0)
	{
		return 0;
	}
	int r = num >> 1;
	r = r | (r >> 1);
	r = r | (r >> 2);
	r = r | (r >> 4);
	r = r | (r >> 8);
	r = r | (r >> 16);
	// Remove most significant bit
	int value = r & num;
	// Display calculated result
	return value;
}