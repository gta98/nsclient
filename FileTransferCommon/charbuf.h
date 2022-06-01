#pragma once

#include "common_includes.h"

typedef struct {
	char* buf;
	size_t  buf_size_bytes;
	uint8_t cell_size_bits;
} charbuf_t;

uint64_t  charbuf_select(charbuf_t* buf, uint64_t idx);
void charbuf_assign(charbuf_t* buf, uint64_t idx, uint64_t value);
void charbuf_from_sparse_arr(charbuf_t* dst, uint8_t cell_size_bits, char src[], uint64_t src_size);

void buf_shift_left(char buf[], uint64_t buf_size, uint64_t shift_count_bits);
uint32_t pop_left_26(char buf[], uint64_t buf_size);
uint32_t pop_left_31(char buf[], uint64_t buf_size);