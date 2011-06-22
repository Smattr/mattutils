
#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

/* Note all list operations return a (opaque) pointer to the head unless
 * otherwise noted.
 */

/* Create a list and initialise relevant structures. */
void* list_create(void);

/* Add an item to the list (efficiently). */
void* list_add(void* l, void* value);

/* Add an item to the end of the list. This may be less efficient than the
 * default add function.
 */
void* list_add_to_end(void* l, void* value);

/* Delete an item matching value from the list */
void* list_delete(void* l, void* value, int (*comparator)(void*, void*));

/* Returns 1 if the item exists in the list and 0 otherwise. */
unsigned int list_contains(void* l, void* value, int (*comparator)(void*, void*));

/* Returns a pointer to the value in the list if it exists or NULL otherwise. */
void* list_find(void* l, void* value, int (*comparator)(void*, void*));

/* Returns the length of the list. */
unsigned int list_length(void* l);

/* Destroys the list and frees all associated resources. It is left to the
 * caller to free any dynamic memory associated with the contained values.
 */
void list_destroy(void* l);

#endif /* !_LINKED_LIST_H_ */

