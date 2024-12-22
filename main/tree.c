#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "tree.h"

TreeNode *create_node(const void *data, uint8_t data_size, copy_func_t copy_func)
{
	TreeNode *node = malloc(sizeof(TreeNode));
	if (!node) return NULL;

	node->data_size = data_size;
	if (copy_func)
	{
		node->data = copy_func(data);
	}
	else
	{
		node->data = malloc(data_size);
		if (!node->data)
		{
			free(node);
			return NULL;
		}
		memcpy(node->data, data, data_size);
	}

	node->associated = NULL;
	node->left_child = NULL;
	node->right_child = NULL;

	return node;
}

void destroy_node(TreeNode *node, destroy_func_t destroy_func)
{
	if (!node) return;

	if (node->associated) 
	{
		destroy_list(node->associated, destroy_func);
	}
	if (destroy_func)
	{
		destroy_func(node->data);
	}
	else
	{
		free(node->data);
	}

	free(node);    
}

void insert_node(TreeNode **root, const void *data, uint8_t data_size, copy_func_t copy_func, compare_func_t compare_func)
{
	if (!*root)
	{
		*root = create_node(data, data_size, copy_func);
		return;
	}

	int cmp = compare_func(data, (*root)->data);
	if (cmp < 0)
	{
		insert_node(&(*root)->left_child, data, data_size, copy_func, compare_func);
	}
	else if (cmp > 0)
	{
		insert_node(&(*root)->right_child, data, data_size, copy_func, compare_func);
	}
}

TreeNode *find_node(TreeNode *root, const void *data, compare_func_t compare_func)
{
	if (!root) return NULL;

	int cmp = compare_func(data, root->data);
	if (cmp < 0)
	{
		return find_node(root->left_child, data, compare_func);
	}
	else if (cmp > 0)
	{
		return find_node(root->right_child, data, compare_func);
	}

	return root;
}

LinkedList *create_list(const void *data, uint8_t data_size, copy_func_t copy_func)
{
	LinkedList *list = malloc(sizeof(LinkedList));
	if (!list) return NULL;

	list->data_size = data_size;
	if (copy_func)
	{
		list->data = copy_func(data);
	}
	else
	{
		list->data = malloc(data_size);
		if (!list->data)
		{
			free(list);
			return NULL;
		}
		memcpy(list->data, data, data_size);
	}

	list->next = NULL;

	return list;
}

void destroy_list(LinkedList *list, destroy_func_t destroy_func)
{
	if (!list) return;

	if (list->next) 
	{
		destroy_list(list->next, destroy_func);
	}
	if (destroy_func)
	{
		destroy_func(list->data);
	}
	else
	{
		free(list->data);
	}

	free(list);
}

void insert_list(LinkedList **head, const void *data, uint8_t data_size, copy_func_t copy_func)
{
	if (!*head)
	{
		*head = create_list(data, data_size, copy_func);
		return;
	}

	insert_list(&(*head)->next, data, data_size, copy_func);
}


int compare_mac_addresses(const void *a, const void *b)
{
	const MACAddress *mac_a = a;
	const MACAddress *mac_b = b;

	for (int i = 0; i < 6; i++)
	{
		if (mac_a->mac[i] < mac_b->mac[i]) return -1;
		if (mac_a->mac[i] > mac_b->mac[i]) return 1;
	}

	return 0;
}

void print_network(TreeNode *root)
{
	if (!root) return;

	print_network(root->left_child);
	printf("Access Point: ");
	for (int i = 0; i < 6; i++)
	{
		printf("%02X", ((MACAddress *)root->data)->mac[i]);
		if (i < 5) printf(":");
	}
	printf("\n");

	LinkedList *current = root->associated;
	while (current)
	{
		printf("  Device: ");
		for (int i = 0; i < 6; i++)
		{
			printf("%02X", ((MACAddress *)current->data)->mac[i]);
			if (i < 5) printf(":");
		}
		printf("\n");
		current = current->next;
	}

	print_network(root->right_child);
}

void process_tree(TreeNode *root, process_couple_func_t process_couple_func)
{
	if (!root) return;

	process_tree(root->left_child, process_couple_func);

	LinkedList *current = root->associated;
    if(!current)
    {
        MACAddress broadcast = {
            .mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
        };
        CoupleAP_DEV couple = {*(MACAddress *)root->data, broadcast};
        process_couple_func(&couple);
    }
	while (current)
	{
		CoupleAP_DEV couple = {*(MACAddress *)root->data, *(MACAddress *)current->data};
		process_couple_func(&couple);
		current = current->next;
	}

	process_tree(root->right_child, process_couple_func);
}

void simulation(process_couple_func_t cb)
{
	// We create a tree of mac addresses:
	// in the tree each node is the mac address
	// of an access point and each node has a list
	// of associated devices (still mac addresses),
	// we use a tree to store mac addresses alphabetically,

	MACAddress access_point1 = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x55}};
	MACAddress access_point2 = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x56}};
	MACAddress access_point3 = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x57}};
	MACAddress access_point4 = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x54}};
    MACAddress access_point5 = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x53}};
    MACAddress access_point6 = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x52}};

	MACAddress device1 = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x58}};
	MACAddress device2 = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x59}};
	MACAddress device3 = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x5A}};
	MACAddress device4 = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x5B}};
	MACAddress device5 = {{0x00, 0x11, 0x22, 0x33, 0x44, 0x5C}};

	// device 1 and 2 are associated with access point 1,
	// device 3 is associated with access point 2,
	// device 4 is associated with access point 3,
	// device 5 is associated with access point 4
	
	LinkedList *associated_to_access_point1 = NULL;
	insert_list(&associated_to_access_point1, &device1, sizeof(MACAddress), NULL);
	insert_list(&associated_to_access_point1, &device2, sizeof(MACAddress), NULL);
	
	LinkedList *associated_to_access_point2 = NULL;
	insert_list(&associated_to_access_point2, &device3, sizeof(MACAddress), NULL);

	LinkedList *associated_to_access_point3 = NULL;
	insert_list(&associated_to_access_point3, &device4, sizeof(MACAddress), NULL);

	LinkedList *associated_to_access_point4 = NULL;
	insert_list(&associated_to_access_point4, &device5, sizeof(MACAddress), NULL);

	TreeNode *root = NULL;
	insert_node(&root, &access_point1, sizeof(MACAddress), NULL, compare_mac_addresses);
	insert_node(&root, &access_point2, sizeof(MACAddress), NULL, compare_mac_addresses);
	insert_node(&root, &access_point3, sizeof(MACAddress), NULL, compare_mac_addresses);
	insert_node(&root, &access_point4, sizeof(MACAddress), NULL, compare_mac_addresses);
    insert_node(&root, &access_point5, sizeof(MACAddress), NULL, compare_mac_addresses);
    insert_node(&root, &access_point6, sizeof(MACAddress), NULL, compare_mac_addresses);


	TreeNode *ap_node = find_node(root, &access_point1, compare_mac_addresses);
	ap_node->associated = associated_to_access_point1;

	ap_node = find_node(root, &access_point2, compare_mac_addresses);
	ap_node->associated = associated_to_access_point2;

	ap_node = find_node(root, &access_point3, compare_mac_addresses);
	ap_node->associated = associated_to_access_point3;

	ap_node = find_node(root, &access_point4, compare_mac_addresses);
	ap_node->associated = associated_to_access_point4;

	printf("\n\n");

	printf("Process network:\n");
	process_tree(root, cb);
    
}

MyQueue *create_queue(const void *data, uint8_t data_size, copy_func_t copy_func)
{
	MyQueue *queue = malloc(sizeof(MyQueue));
	if (!queue) return NULL;

	queue->data = malloc(data_size);
	if (!queue->data)
	{
		free(queue);
		return NULL;
	}
	memcpy(queue->data, data, data_size);

	queue->next = NULL;

	return queue;
}

void destroy_queue(MyQueue *queue, destroy_func_t destroy_func)
{
	if (!queue) return;

	if (queue->next) 
	{
		destroy_queue(queue->next, destroy_func);
	}
	if (destroy_func)
	{
		destroy_func(queue->data);
	}
	else
	{
		free(queue->data);
	}

	free(queue);
}

void enqueue(MyQueue **head, const void *data, uint8_t data_size, copy_func_t copy_func)
{
	if (!*head)
	{
		*head = create_queue(data, data_size, copy_func);
		return;
	}

	enqueue(&(*head)->next, data, data_size, copy_func);
}

void *dequeue(MyQueue **head)
{
	if (!*head) return NULL;

	MyQueue *current = *head;
	*head = current->next;

	void *data = current->data;
	free(current);

	return data;
}