#pragma once

#include "common_includes.h"

// NOTE: parity32, hamming_encode, hamming_decode, print_bin taken from:
// https://gist.github.com/qsxcv/b2f9976763d52bf1e7fc255f52f05f5b

// https://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
static inline bool parity32(uint32_t v);

// Hamming(31, 26) plus total parity at bit 0 for double error detection
// https://en.wikipedia.org/wiki/Hamming_code#General_algorithm
uint32_t hamming_encode(uint32_t d);

uint32_t hamming_decode(uint32_t h);

void print_bin(uint32_t v);

//uint64_t encode_26_block_to_31(char* dst, char* src, uint64_t src_size);
//uint64_t decode_31_block_to_26(char* dst, char* src, uint64_t src_size);
//uint64_t encode_x_block_to_y(char** dst, char* src, uint64_t src_size, int x, int y, uint32_t(*f_encode)(uint32_t));
void encode_x_block_to_y(char dst[], char src[], int x, int y, uint32_t(*f_encode)(uint32_t));
void encode_x_block_to_y_offset(char dst[], char src[], int x, int y, uint32_t(*f_encode)(uint32_t), int src_offset);
void encode_26_block_to_31(uint8_t dst[31], uint8_t src[26]);
void decode_31_block_to_26(uint8_t dst[26], uint8_t src[31]);
void encode_26_block_to_31_offset(uint8_t dst[], uint8_t src[], int src_offset);
void decode_31_block_to_26_offset(uint8_t dst[], uint8_t src[], int src_offset);
//void decode_31_block_to_26(char* dst, uint64_t* dst_size, char* src, uint64_t src_size);
