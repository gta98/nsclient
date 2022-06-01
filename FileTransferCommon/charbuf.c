#include "charbuf.h"

uint64_t charbuf_select(charbuf_t* buf, uint64_t idx) {
	uint64_t bit_idx, bit_idx_end, cell_idx, cell_idx_end;
	uint8_t cell_shift, cell_shift_end, mask_shift, mask_shift_end;
	bit_idx     = buf->cell_size_bits * idx;
	bit_idx_end = bit_idx + buf->cell_size_bits;
	cell_idx = floor(bit_idx / sizeof(char));
	cell_shift = (bit_idx - (cell_idx * sizeof(char)));
	cell_idx_end = floor(bit_idx_end / sizeof(char));
	cell_shift_end = (bit_idx_end - (cell_idx_end * sizeof(char)));

	//mask_shift = (1 << (8))
	return 0;
}
void charbuf_assign(charbuf_t* buf, uint64_t idx, uint64_t value) {

}

void buf_shift_left(char buf[], uint64_t buf_size, uint64_t shift_count_bits) {
	uint64_t i, number_of_cells_dropped, buf_size_new;
	uint8_t shift_first, shift_second;
	number_of_cells_dropped = floor(shift_count_bits / sizeof(char));
	shift_first             = fmod (shift_count_bits , sizeof(char));
	shift_second            = sizeof(char) - shift_first;

	for (i=0; i<(buf_size-1-number_of_cells_dropped); i++) {
		buf[i] = (buf[i+number_of_cells_dropped] << shift_first) | (buf[i+number_of_cells_dropped+1] >> shift_second);
	}
	buf[buf_size-1-number_of_cells_dropped] = buf[buf_size - 1] << shift_first;
	for (i=(buf_size-1-number_of_cells_dropped+1); i<buf_size; i++) {
		buf[i] = 0;
	}
}

uint32_t pop_left_26(char buf[], uint64_t buf_size) {
	uint32_t ret = (((uint32_t)buf[0]) << 18) | (((uint32_t)buf[1]) << 10) | (((uint32_t)buf[2]) << 2) | ((((uint32_t)buf[3]) & 0b11000000)>>6);
	buf_shift_left(buf, buf_size, 26);
}

uint32_t pop_left_31(char buf[], uint64_t buf_size) {
	uint32_t ret = (((uint32_t)buf[0]) << 23) | (((uint32_t)buf[1]) << 15) | (((uint32_t)buf[2]) << 7) | ((((uint32_t)buf[3]) & 0b11111110)>>1);
	buf_shift_left(buf, buf_size, 31);
}