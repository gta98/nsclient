#include "list.h"

node_t* node_init() {
	node_t* node = malloc(sizeof(node_t));
	for (int i = 0; i < 31; i++) node->data[i] = 0;
	node->next = NULL;
	return node;
}

node_t* node_init_data(uint8_t data[31]) {
	node_t* node = node_init();
	for (int i = 0; i < 31; i++) node->data[i] = data[i];
	return node;
}

void node_free_rec(node_t* node) {
	if (node == NULL) return;
	node_free_rec(node->next);
	free(node);
}

linkedlist_t* list_init() {
	linkedlist_t* list = malloc(sizeof(linkedlist_t));
	list->size = 0;
	list->head = NULL;
	list->tail = NULL;
}

void list_add(linkedlist_t* list, node_t* node) {
	if (list->size == 0) {
		list->head = node;
		list->tail = node;
	}
	else {
		((node_t*)list->tail)->next = node;
	}
	(list->size)++;
}

void list_add_data(linkedlist_t* list, uint8_t data[31]) {
	node_t* node = node_init_data(data);
	list_add(list, node);
}

void list_free(linkedlist_t* list) {
	node_free_rec(list->head);
	free(list);
}