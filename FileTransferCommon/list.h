#pragma once

#include "common_includes.h"

typedef struct {
	uint8_t data[31];
	struct node_t* next;
} node_t;

typedef struct {
	int size;
	struct node_t* head;
	struct node_t* tail;
} linkedlist_t;