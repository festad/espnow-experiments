#pragma once
#include <stdint.h>
#include <string.h>

typedef struct TreeNode
{
	void *data;
    uint8_t data_size;
	struct LinkedList *associated;
	struct TreeNode *left_child;
	struct TreeNode *right_child;
} TreeNode;

typedef struct LinkedList
{
	void *data;
	uint8_t data_size;
	struct LinkedList *next;
} LinkedList;

typedef struct MyQueue
{
	void *data;
	struct MyQueue *next;
} MyQueue;

typedef void *(*copy_func_t)(const void *src);
typedef void (*destroy_func_t)(void *data);
typedef int (*compare_func_t)(const void *a, const void *b);
typedef void (*process_couple_func_t)(const void *data);

TreeNode *create_node(const void *data, uint8_t data_size, copy_func_t copy_func);
void destroy_node(TreeNode *node, destroy_func_t destroy_func);
void insert_node(TreeNode **root, const void *data, uint8_t data_size, copy_func_t copy_func, compare_func_t compare_func);
TreeNode *find_node(TreeNode *root, const void *data, compare_func_t compare_func);

LinkedList *create_list(const void *data, uint8_t data_size, copy_func_t copy_func);
void destroy_list(LinkedList *list, destroy_func_t destroy_func);
void insert_list(LinkedList **head, const void *data, uint8_t data_size, copy_func_t copy_func);

MyQueue *create_queue(const void *data, uint8_t data_size, copy_func_t copy_func);
void destroy_queue(MyQueue *queue, destroy_func_t destroy_func);
void enqueue(MyQueue **head, const void *data, uint8_t data_size, copy_func_t copy_func);
void *dequeue(MyQueue **head);

typedef struct MACAddress
{
	unsigned char mac[6];
	uint8_t channel;
} MACAddress;

typedef struct CoupleAP_DEV
{
    MACAddress ap;
    MACAddress dev;
} CoupleAP_DEV;

int compare_mac_addresses(const void *a, const void *b);

void print_couple_mac_addresses(const void *data);

void process_tree(TreeNode *root, process_couple_func_t process_couple_func);

void simulation(process_couple_func_t cb);