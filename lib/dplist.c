/**
 * \author Zhewen Xue
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"

/*
 * definition of error codes
 * */
#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1 // error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2 //error due to a list operation applied on a NULL list 

#ifdef DEBUG
#define DEBUG_PRINTF(...) 									                                        \
        do {											                                            \
            fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	    \
            fprintf(stderr,__VA_ARGS__);								                            \
            fflush(stderr);                                                                         \
                } while(0)
#else
#define DEBUG_PRINTF(...) (void)0
#endif


#define DPLIST_ERR_HANDLER(condition, err_code)                         \
    do {                                                                \
            if ((condition)) DEBUG_PRINTF(#condition " failed\n");      \
            assert(!(condition));                                       \
        } while(0)


/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;

    void *(*element_copy)(void *src_element);

    void (*element_free)(void **element);

    int (*element_compare)(void *x, void *y);
};


dplist_t *dpl_create(// callback functions
        void *(*element_copy)(void *src_element),
        void (*element_free)(void **element),
        int (*element_compare)(void *x, void *y)
) {
    dplist_t *list;
    list = malloc(sizeof(struct dplist));
    DPLIST_ERR_HANDLER(list == NULL, DPLIST_MEMORY_ERROR);
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {
	dplist_t * new_list = *list;
	dplist_node_t * node1, *node2;
	if(*list!=NULL){
		if(new_list->head!=NULL){
			node1 = new_list->head;
			node2 = new_list->head;
			while(node1->next!=NULL){
				node2 = node1->next;
				if(free_element == true) (*((*list)->element_free))(&(node1->element));
				free(node1);
				node1 = node2;
				new_list->head = node1;
			}
			if(free_element == true) (*((*list)->element_free))(&(node1->element));
                        new_list->head = NULL;
			free(node1);
			free(*list);

		}else free(*list);
	}
	*list = NULL;
    //TODO: add your code here

}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {
    dplist_node_t *ref_at_index, *list_node;
    if (list == NULL) return NULL;
    if(element == NULL) return list;
    list_node = malloc(sizeof(dplist_node_t));
    DPLIST_ERR_HANDLER(list_node == NULL, DPLIST_MEMORY_ERROR);
    if(insert_copy == true)	list_node->element = (*list->element_copy)(element);
    else	list_node->element = element;
    // pointer drawing breakpoint
    if (list->head == NULL) { // covers case 1
        list_node->prev = NULL;
        list_node->next = NULL;
        list->head = list_node;
        // pointer drawing breakpoint
    } else if (index <= 0) { // covers case 2
        list_node->prev = NULL;
        list_node->next = list->head;
        list->head->prev = list_node;
        list->head = list_node;
        // pointer drawing breakpoint
    } else {
        ref_at_index = dpl_get_reference_at_index(list, index);
        assert(ref_at_index != NULL);
        // pointer drawing breakpoint
        if (index < dpl_size(list)) { // covers case 4
            list_node->prev = ref_at_index->prev;
            list_node->next = ref_at_index;
            ref_at_index->prev->next = list_node;
            ref_at_index->prev = list_node;
            // pointer drawing breakpoint
        } else { // covers case 3
            assert(ref_at_index->next == NULL);
            list_node->next = NULL;
            list_node->prev = ref_at_index;
            ref_at_index->next = list_node;
            // pointer drawing breakpoint
        }
    }
    return list;

    //TODO: add your code here

}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {
	dplist_node_t *ref_at_index;
   if (list == NULL) return NULL;
   else if (list->head == NULL) return list;  //covers case 1

   if (index <= 0) { // covers case 2
   ref_at_index = list->head;
   list->head = list->head->next;
   if(list->head != NULL) list->head->prev = NULL;  // in case that only one element in list
   }
   else {
        ref_at_index = dpl_get_reference_at_index(list, index);
        assert(ref_at_index != NULL);

   if(index < (dpl_size(list)-1)){ // covers case 4
        ref_at_index->prev->next = ref_at_index->next;
        ref_at_index->next->prev = ref_at_index->prev;
   }
   else { //covers case 3
  	    assert(ref_at_index->next == NULL);
	    if(ref_at_index->prev != NULL) ref_at_index->prev->next = NULL;    // in case that the last node
	    else list->head = NULL;
    	}
}

	if(free_element == true) (*(list->element_free))(&(ref_at_index->element));  //free_element if true, call element_free() on the element of the list node to remove
    	free(ref_at_index);
    	return list;
    //TODO: add your code here

}

int dpl_size(dplist_t *list) {
	int count = 1;
	dplist_node_t * node;
	if(list == NULL) return -1;
	else if(list->head == NULL) return 0;
	else{
		node = list->head;
		while(node->next!=NULL){
			node = node->next;
			count++;
		}

	}return count;
    //TODO: add your code here

}

void *dpl_get_element_at_index(dplist_t *list, int index) {
	dplist_node_t * node;
	if(list == NULL || list->head == NULL)	return NULL;//case 1 and 2
	else{
		node = dpl_get_reference_at_index(list,index);
		return node->element;
	}
    //TODO: add your code here

}
/** Returns an index to the first list node in the list containing 'element'.
 * - the first list node has index 0.
 * - Use 'element_compare()' to search 'element' in the list, a match is found when 'element_compare()' returns 0.
 * - If 'element' is not found in the list, -1 is returned.
 * - If 'list' is NULL, NULL is returned.
 * \param list a pointer to the list
 * \param element the element to look for
 * \return the index of the element that matches 'element'
 */

int dpl_get_index_of_element(dplist_t *list, void *element) {
	if(list == NULL||list->head == NULL||element==NULL)	return -1;
	dplist_node_t * node = list->head;
	int count = 0;
	int size = dpl_size(list);
	while(count<size){
		if((*list->element_compare)(element,node->element)==0)	return count;
		node = node->next;
		count++;
	}return -1;
    //TODO: add your code here

}
// return the node that it is the (index-1)th in the list
dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {
	dplist_node_t * node;
	if(list == NULL|| list->head ==NULL)	return NULL;//case 1 and 2
	else{
		int sum = dpl_size(list);
		if(index<=0){// case index<=0
			return list->head;
		}else if(index>=sum-1){//case index is bigger than the size of the list
			node = list->head;
			while(node->next!=NULL)	{node=node->next;}
			return node;
		}else{//normal case
			node = list->head;
			for(int i =0; i<index; i++){
				node = node->next;
			}
			return node;
		}
	}
	return NULL;    //TODO: add your code here

}
/** Returns the element contained in the list node with reference 'reference' in the list.
 * - If the list is empty, NULL is returned.
 * - If 'list' is is NULL, NULL is returned.
 * - If 'reference' is NULL, NULL is returned.
 * - If 'reference' is not an existing reference in the list, NULL is returned.
 * \param list a pointer to the list
 * \param reference a pointer to a certain node in the list
 * \return the element contained in the list node or NULL
 */

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {
	if(list == NULL||list->head == NULL||reference==NULL)	return NULL;
	dplist_node_t * node;
	node = list->head;
	while(node->next!=NULL){
	if(node==reference)	return node->element;
	else	node = node->next;
	}
	if(node==reference) return node->element;//test the last node
	return NULL;
    //TODO: add your code here

}


