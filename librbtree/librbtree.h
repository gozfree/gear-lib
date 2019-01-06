/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    librbtree.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-10 21:42
 * updated: 2015-08-10 21:42
 *****************************************************************************/
#ifndef LIBRBTREE_H
#define LIBRBTREE_H

#ifdef __cplusplus
extern "C" {
#endif

struct rb_node {
    unsigned long  __rb_parent_color;
    struct rb_node *rb_right;
    struct rb_node *rb_left;
};

struct rb_root {
    struct rb_node *rb_node;
};


#define rb_parent(r)   ((struct rb_node *)((r)->__rb_parent_color & ~3))

#define RB_ROOT (struct rb_root) { NULL, }
#define rb_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root)  ((root)->rb_node == NULL)

/* 'empty' nodes are nodes that are known not to be inserted in an rbree */
#define RB_EMPTY_NODE(node)  \
        ((node)->__rb_parent_color == (unsigned long)(node))
#define RB_CLEAR_NODE(node)  \
        ((node)->__rb_parent_color = (unsigned long)(node))

void rb_insert_color(struct rb_node *, struct rb_root *);
void rb_erase(struct rb_node *, struct rb_root *);

/* Find logical next and previous nodes in a tree */
struct rb_node *rb_next(const struct rb_node *);
struct rb_node *rb_prev(const struct rb_node *);
struct rb_node *rb_first(const struct rb_root *);
struct rb_node *rb_last(const struct rb_root *);

/* Postorder iteration - always visit the parent after its children */
struct rb_node *rb_first_postorder(const struct rb_root *);
struct rb_node *rb_next_postorder(const struct rb_node *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
void rb_replace_node(struct rb_node *victim, struct rb_node *_new,
                struct rb_root *root);

static inline void rb_link_node(struct rb_node * node, struct rb_node * parent,
                struct rb_node ** rb_link)
{
    node->__rb_parent_color = (unsigned long)parent;
    node->rb_left = node->rb_right = NULL;
    *rb_link = node;
}

#define rb_entry_safe(ptr, type, member) \
        ({ typeof(ptr) ____ptr = (ptr); \
         ____ptr ? rb_entry(____ptr, type, member) : NULL; \
         })

/**
 * rbtree_postorder_for_each_entry_safe - iterate over rb_root in post order of
 * given type safe against removal of rb_node entry
 *
 * @pos:    the 'type *' to use as a loop cursor.
 * @n:      another 'type *' to use as temporary storage
 * @root:   'rb_root *' of the rbtree.
 * @field:  the name of the rb_node field within 'type'.
 */
#define rbtree_postorder_for_each_entry_safe(pos, n, root, field) \
    for (pos = rb_entry_safe(rb_first_postorder(root), typeof(*pos), field); \
         pos && ({ n = rb_entry_safe(rb_next_postorder(&pos->field), \
             typeof(*pos), field); 1; }); \
         pos = n)

#ifdef __cplusplus
}
#endif
#endif
