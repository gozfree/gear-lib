/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#ifndef LIBDICT_H
#define LIBDICT_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _keypair_ {
    char *key;
    char *val;
    uint32_t hash;
} keypair ;

typedef struct _dict_ {
    uint32_t fill;
    uint32_t used;
    uint32_t size;
    keypair *table;
} dict ;

typedef struct _key_list_ {
    char *key;
    struct _key_list_ *next;
} key_list;

/*-------------------------------------------------------------------------*/
/**
  @brief    Allocate a new dictionary object
  @return   Newly allocated dict, to be freed with dict_free()

  Constructor for the dict object.
 */
/*--------------------------------------------------------------------------*/
dict * dict_new(void);


/*-------------------------------------------------------------------------*/
/**
  @brief    Deallocate a dictionary object
  @param    d   dict to deallocate
  @return   void

  This function will deallocate a dictionary and all data it holds.
 */
/*--------------------------------------------------------------------------*/
void   dict_free(dict * d);

/*-------------------------------------------------------------------------*/
/**
  @brief    Add an item to a dictionary
  @param    d       dict to add to
  @param    key     Key for the element to be inserted
  @param    val     Value to associate to the key
  @return   0 if Ok, something else in case of error

  Insert an element into a dictionary. If an element already exists with
  the same key, it is overwritten and the previous associated data are freed.
 */
/*--------------------------------------------------------------------------*/
int    dict_add(dict * d, char * key, char * val);

/*-------------------------------------------------------------------------*/
/**
  @brief    Get an item from a dictionary
  @param    d       dict to get item from
  @param    key     Key to look for
  @param    defval  Value to return if key is not found in dict
  @return   Element found, or defval

  Get the value associated to a given key in a dict. If the key is not found,
  defval is returned.
 */
/*--------------------------------------------------------------------------*/
char * dict_get(dict * d, char * key, char * defval);

/*-------------------------------------------------------------------------*/
/**
  @brief    Delete an item in a dictionary
  @param    d       dict where item is to be deleted
  @param    key     Key to look for
  @return   0 if Ok, something else in case of error

  Delete an item in a dictionary. Will return 0 if item was correctly
  deleted and -1 if the item could not be found or an error occurred.
 */
/*--------------------------------------------------------------------------*/
int dict_del(dict * d, char * key);

/*-------------------------------------------------------------------------*/
/**
  @brief    Enumerate a dictionary
  @param    d       dict to browse
  @param    rank    Rank to start the next (linear) search
  @param    key     Enumerated key (modified)
  @param    val     Enumerated value (modified)
  @return   int rank of the next item to enumerate, or -1 if end reached

  Enumerate a dictionary by returning all the key/value pairs it contains.
  Start the iteration by providing rank=0 and two pointers that will be
  modified to references inside the dict.
  The returned value is the immediate successor to the one being returned,
  or -1 if the end of dict was reached.
  Do not free or modify the returned key/val pointers.

  See dict_dump() for usage example.
 */
/*--------------------------------------------------------------------------*/
int dict_enumerate(dict * d, int rank, char ** key, char ** val);

/*-------------------------------------------------------------------------*/
/**
  @brief    Dump dict contents to an opened file pointer
  @param    d       dict to dump
  @param    out     File to output data to
  @return   void

  Dump the contents of a dictionary to an opened file pointer.
  It is Ok to pass 'stdout' or 'stderr' as file pointers.

  This function is mostly meant for debugging purposes.
 */
/*--------------------------------------------------------------------------*/
void dict_dump(dict * d, FILE * out);

void dict_get_key_list(dict * d, key_list **klist);

#ifdef __cplusplus
}
#endif
#endif
