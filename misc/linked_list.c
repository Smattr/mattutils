/* An implementation of a linked list.
 *
 * The void* juggling is for two purposes. With regards to the list pointers,
 * it is to ensure the pointers passed back and forth to the caller are opaque
 * (the caller has no knowledge of struct list). The void* pointers for value
 * are to generalise the contents of the list. Note that the strict aliasing
 * rule is not violated by any of this casting as none of the void* pointers
 * are ever dereferenced.
 */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "linked_list.h"

typedef struct list {
    void* value;
    struct list* next;
} list_t;

inline void* list_create(void) {
    return (void*)0;
}

void* list_add(void* l, void* value) {
    list_t* node;

    node = (list_t*) malloc(sizeof(list_t));
    node->value = value;
    node->next = (list_t*)l;

    return (void*)node;
}

void* list_add_to_end(void* l, void* value) {
    list_t* node;

    node = (list_t*) malloc(sizeof(list_t));
    node->value = value;
    node->next = 0;
    if (!l)
        return (void*)node;
    else {
        list_t* end = (list_t*)l;
        while (end->next) end = end->next;
        end->next = node;
        return l;
    }
}

void* list_delete(void* l, void* value, int (*comparator)(void*, void*)) {
    list_t* node = (list_t*)l;
    if (!l)
        return (void*)0;
    else if (!comparator(node->value, value)) {
        /* We need to delete the head. */
        list_t* next = node->next;
        free(node);
        return (void*)next;
    } else {
        for (; node->next; node = node->next) {
            if (!comparator(value, node->next->value)) {
                /* Found the item we need to delete. */
                list_t* next = node->next;
                node->next = node->next->next;
                free(next);
                break;
            }
        }
        /* It's possible we didn't find the item to delete. */
        return l;
    }
}

unsigned int list_contains(void* l, void* value, int (*comparator)(void*, void*)) {
    return !!list_find(l, value, comparator);
}

void* list_find(void* l, void* value, int (*comparator)(void*, void*)) {
    list_t* iter;

    for (iter = (list_t*)l; iter; iter = iter->next)
        if (!comparator(iter->value, value))
            return iter->value;
    return (void*)0;
}

unsigned int list_length(void* l) {
    unsigned int i = 0;
    list_t* iter;

    for (iter = (list_t*)l; iter; iter = iter->next, ++i);
    return i;
}

void list_destroy(void* l) {
    /* This assumes that the caller will take care of any resources associated
     * with values.
     */
    list_t* prev, *iter = (list_t*)l;
    while (iter) {
        prev = iter;
        iter = iter->next;
        free(prev);
    }
}

/* Testing code */

#define LEN 10

int main(int argc, char** argv) {
    int i, j, k;
    unsigned int data[LEN];
    void* ll;
    int my_comparator(void* a, void* b) {
        unsigned int *x = (unsigned int*)a;
        unsigned int *y = (unsigned int*)b;
        return *x - *y;
    };
    void* (*add_fn[])(void*, void*) = {&list_add, &list_add_to_end, NULL};

    /* Populate data array */
    for (i = 0; i < LEN; ++i)
        data[i] = i;

    for (k = 0; add_fn[k]; ++k) {
        ll = list_create();

        /* Add each element and test the contents of the list in between. */
        for (i = 0; i < LEN; ++i) {
            for (j = 0; j < LEN; ++j)
                assert(!!(j < i) == !!list_contains(ll, (void*)&j, my_comparator));
            ll = add_fn[k](ll, (void*)(data+i));
            for (j = 0; j < LEN; ++j)
                assert(!!(j <= i) == !!list_contains(ll, (void*)&j, my_comparator));
            assert(list_length(ll) == i + 1);
        }

        /* Delete elements one by one. */
        for (i = 0; i < LEN; ++i) {
            for (j = 0; j < LEN; ++j)
                assert(!!(j < i) != !!list_contains(ll, (void*)&j, my_comparator));
            ll = list_delete(ll, (void*)(data+i), my_comparator);
            for (j = 0; j < LEN; ++j)
                assert(!!(j <= i) != !!list_contains(ll, (void*)&j, my_comparator));
            assert(list_length(ll) == LEN - i - 1);
        }

        /* Add them again in reverse */
        for (i = LEN - 1; i >= 0; --i) {
            for (j = 0; j < LEN; ++j)
                assert(!!(j > i) == !!list_contains(ll, (void*)&j, my_comparator));
            ll = add_fn[k](ll, (void*)(data+i));
            for (j = 0; j < LEN; ++j)
                assert(!!(j >= i) == !!list_contains(ll, (void*)&j, my_comparator));
            assert(list_length(ll) == LEN - i);
        }

        list_destroy(ll);
    }

    return 0;
}
