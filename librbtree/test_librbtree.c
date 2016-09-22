/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_librbtree.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-10 22:35
 * updated: 2015-08-10 22:35
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <libmacro.h>
#include "librbtree.h"

#ifndef false
#define false   (0)
#endif

#ifndef true
#define true    (!(false))
#endif

struct my_rbnode {
    struct rb_node node;
    int key;
};
static struct rb_root mytree_uk = RB_ROOT;

#if 0
static struct my_rbnode *my_search(struct rb_root *root, int val)
{
    struct rb_node *node = root->rb_node;

    while (node) {
        struct my_rbnode *data = container_of(node, struct my_rbnode, node);
        int result;

        result = val - data->key;

        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            return data;
    }
    return NULL;
}
#endif

static int my_insert(struct rb_root *root, struct my_rbnode *data)
{
    struct rb_node **_new = &(root->rb_node), *parent = NULL;

    /* Figure out where to put new node */
    while (*_new) {
        struct my_rbnode *_this = container_of(*_new, struct my_rbnode, node);
        int result = (data->key - _this->key);

        parent = *_new;
        if (result < 0)
            _new = &((*_new)->rb_left);
//        else if (result > 0)
        else
            _new = &((*_new)->rb_right);
//        else
//            return false;
    }

    /* Add new node and rebalance tree. */
    rb_link_node(&data->node, parent, _new);
    rb_insert_color(&data->node, root);

    return true;
}

static int input1[] = {-84,170,-85,142,-17,41,170,-85,-17,-71,170,152,-31,161,22,104,-85,160,120,-31,144,115};
static int input2[] = {-84,-84,-84,-84,-84,-84,-84,-84,-84,-84,-84,-84,-84,-84,-84,-84,-84,-84,};
static int input3[] = {170,-85,170,-85,170,-85,170,-85,170,-85,170,-85,170,-85,170,-85,170,-85,170,-85,170,-85};
static int input4[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};

static void test(struct rb_root *mytree, int *input, size_t len)
{
    struct my_rbnode *entry;
    struct rb_node *node;
    int res;
    int i;
    for (i = 0; i < (int)len; i++) {
        struct my_rbnode *entry = CALLOC(1, struct my_rbnode);
        entry->key = input[i];
        res = my_insert(mytree, entry);
        printf("insert %d: %d, res %d\n", i, input[i], res);
        if (res == false) {
            free(entry);
        }
    }
    // test rb_next
    printf("RB_NEXT========================\n");
    node = rb_first(mytree);
    i = 0;
    while (node) {
        entry = rb_entry(node, struct my_rbnode, node);
        printf("%d: %d\n", i++, entry->key);
        node = rb_next(node);
    }
    printf("\n");

    // test rb_prev & rb_last
    printf("RB_PREV========================\n");
    i = 0;
    node = rb_last(mytree);
    while (node) {
        entry = rb_entry(node, struct my_rbnode, node);
        printf("%d: %d\n", i++, entry->key);
        node = rb_prev(node);
    }
    printf("\n");
    i = 0;
    printf("RB_ERASE========================\n");
    //test rb_erase
    while (!RB_EMPTY_ROOT(mytree)) {
        node = rb_first(mytree);
        entry = rb_entry(node, struct my_rbnode, node);
        printf("%d: %d\n", i++, entry->key);
        rb_erase(node, mytree);
        free(entry);
    }
}

int main(int argc, char **argv)
{
    printf("input 1=====================\n");
    test(&mytree_uk, input1, (sizeof(input1)/sizeof(input1[0])));

    printf("input 2=====================\n");
    test(&mytree_uk, input2, (sizeof(input2)/sizeof(input2[0])));

    printf("input 3=====================\n");
    test(&mytree_uk, input3, (sizeof(input3)/sizeof(input3[0])));

    printf("input 4=====================\n");
    test(&mytree_uk, input4, (sizeof(input4)/sizeof(input4[0])));
    return 0;
}
