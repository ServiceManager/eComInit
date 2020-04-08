/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/SYSTEMXVI.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2020 David MacKay.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Linked list
 */

#include <stddef.h>

#ifndef List_h_
#define List_h_

#ifdef __cplusplus
#define INLINE static
extern "C"
{
#else
#include <stdbool.h>
#define INLINE static
#endif

#ifndef S16_MEM_
#define S16_MEM_
    void * s16mem_alloc (unsigned long nbytes);
    void * s16mem_calloc (size_t cnt, unsigned long nbytes);
    char * s16mem_strdup (const char * str);
    void s16mem_free (void * ap);
#endif

/* Defines a list type.
 * name: Friendly name
 * type: Actual type
 * comp: Comparator function
 */
#define S16List(name, type)                                                    \
    typedef struct name##_list_internal_s                                      \
    {                                                                          \
        type val;                                                              \
        struct name##_list_internal_s * Link;                                  \
    } name##_list_internal_t;                                                  \
                                                                               \
    typedef struct name##_list_s                                               \
    {                                                                          \
        name##_list_internal_t * List;                                         \
    } name##_list_t;                                                           \
                                                                               \
    typedef name##_list_internal_t * name##_list_it;                           \
    INLINE name##_list_it name##_list_find_eq (name##_list_t * n, type data);  \
                                                                               \
    INLINE name##_list_t name##_list_new ()                                    \
    {                                                                          \
        name##_list_t l;                                                       \
        l.List = NULL;                                                         \
        return l;                                                              \
    }                                                                          \
                                                                               \
    INLINE name##_list_t * name##_list_add (name##_list_t * n, type data)      \
    {                                                                          \
        name##_list_internal_t *temp, *t;                                      \
                                                                               \
        if (n->List == 0)                                                      \
        {                                                                      \
            /* create new list */                                              \
            t = (name##_list_internal_t *)s16mem_alloc (                       \
                sizeof (name##_list_internal_t));                              \
            t->val = data;                                                     \
            t->Link = NULL;                                                    \
            n->List = t;                                                       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            t = n->List;                                                       \
            temp = (name##_list_internal_t *)s16mem_alloc (                    \
                sizeof (name##_list_internal_t));                              \
            while (t->Link != NULL)                                            \
                t = t->Link;                                                   \
            temp->val = data;                                                  \
            temp->Link = NULL;                                                 \
            t->Link = temp;                                                    \
        }                                                                      \
        return n;                                                              \
    }                                                                          \
                                                                               \
    INLINE void name##_list_add_if_absent (name##_list_t * n, type data)       \
    {                                                                          \
        if (!name##_list_find_eq (n, data))                                    \
            name##_list_add (n, data);                                         \
    }                                                                          \
                                                                               \
    INLINE void name##_list_del (name##_list_t * n, type data)                 \
    {                                                                          \
        name##_list_internal_t *current, *previous;                            \
                                                                               \
        previous = NULL;                                                       \
                                                                               \
        for (current = n->List; current != NULL;                               \
             previous = current, current = current->Link)                      \
        {                                                                      \
            if (current->val == data)                                          \
            {                                                                  \
                if (previous == NULL)                                          \
                {                                                              \
                    /* correct the first */                                    \
                    n->List = current->Link;                                   \
                }                                                              \
                else                                                           \
                {                                                              \
                    /* skip */                                                 \
                    previous->Link = current->Link;                            \
                }                                                              \
                                                                               \
                s16mem_free (current);                                         \
                return;                                                        \
            }                                                                  \
        }                                                                      \
    }                                                                          \
    INLINE bool name##_list_empty (name##_list_t * n) { return !n->List; }     \
    /* Transform each element e of a list to the value of fun(e) */            \
    INLINE void name##_list_trans (name##_list_t * n, type (*fun) (type))      \
    {                                                                          \
        for (name##_list_internal_t * it = n->List; it != NULL; it = it->Link) \
            it->val = fun (it->val);                                           \
    }                                                                          \
    /* For each element e of a list, run fun(e) */                             \
    INLINE void name##_list_do (name##_list_t * n, void (*fun) (type))         \
    {                                                                          \
        for (name##_list_internal_t * it = n->List; it != NULL; it = it->Link) \
            fun (it->val);                                                     \
    }                                                                          \
    typedef int (*name##_list_walk_fun) (type, void *);                        \
    /* For each element e of a list, run fun(e) */                             \
    INLINE int name##_list_walk (const name##_list_t * n,                      \
                                 name##_list_walk_fun fun, void * ex)          \
    {                                                                          \
        int r = 0;                                                             \
        list_foreach (name, n, it)                                             \
        {                                                                      \
            int fr = fun (it->val, ex);                                        \
            r = fr ? fr : 0;                                                   \
        }                                                                      \
        return r;                                                              \
    }                                                                          \
    /* destroy list */                                                         \
    INLINE void name##_list_destroy (name##_list_t * n)                        \
    {                                                                          \
        if (!n)                                                                \
            return;                                                            \
        for (name##_list_internal_t * it = n->List, *tmp; it != NULL;          \
             it = tmp)                                                         \
        {                                                                      \
            tmp = it->Link;                                                    \
            s16mem_free (it);                                                  \
        }                                                                      \
        n->List = NULL;                                                        \
    }                                                                          \
    /* destroy list with destructor @fun on each element */                    \
    INLINE void name##_list_deepdestroy (name##_list_t * n,                    \
                                         void (*fun) (type))                   \
    {                                                                          \
        list_foreach (name, n, it) { fun (it->val); }                          \
        name##_list_destroy (n);                                               \
    }                                                                          \
    INLINE name##_list_it name##_list_find_eq (name##_list_t * n, type data)   \
    {                                                                          \
        list_foreach (name, n, it)                                             \
        {                                                                      \
            if (it->val == data)                                               \
                return it;                                                     \
        }                                                                      \
        return NULL;                                                           \
    }                                                                          \
    typedef bool (*name##_list_find_fn) (type, void *);                        \
    /* Find the first element of a list for which fun returns true */          \
    INLINE name##_list_it name##_list_find (                                   \
        name##_list_t * n, name##_list_find_fn fun, void * extra)              \
    {                                                                          \
        list_foreach (name, n, it) if (fun (it->val, extra)) return it;        \
        return NULL;                                                           \
    }                                                                          \
                                                                               \
    INLINE name##_list_it name##_list_begin (name##_list_t * n)                \
    {                                                                          \
        if (!n)                                                                \
            return 0;                                                          \
        return n->List;                                                        \
    }                                                                          \
                                                                               \
    INLINE name##_list_it name##_list_it_next (name##_list_it it)              \
    {                                                                          \
        return it->Link;                                                       \
    }                                                                          \
                                                                               \
    INLINE type name##_list_lpop (name##_list_t * n)                           \
    {                                                                          \
        type ret;                                                              \
        name##_list_internal_t * tmp;                                          \
                                                                               \
        if (n->List == NULL)                                                   \
        {                                                                      \
            return 0;                                                          \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            ret = n->List->val;                                                \
                                                                               \
            if (n->List->Link)                                                 \
            {                                                                  \
                tmp = n->List->Link;                                           \
                s16mem_free (n->List);                                         \
                n->List = tmp;                                                 \
            }                                                                  \
            else                                                               \
            {                                                                  \
                s16mem_free (n->List);                                         \
                n->List = NULL;                                                \
            }                                                                  \
                                                                               \
            return ret;                                                        \
        }                                                                      \
    }                                                                          \
                                                                               \
    INLINE type name##_list_lget (name##_list_t * n)                           \
    {                                                                          \
        if (n == NULL || n->List == NULL)                                      \
        {                                                                      \
            return 0;                                                          \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            return n->List->val;                                               \
        }                                                                      \
    }                                                                          \
    INLINE name##_list_t * name##_list_lpush (name##_list_t * n, type data)    \
    {                                                                          \
        name##_list_internal_t *temp, *t;                                      \
                                                                               \
        if (n->List == 0)                                                      \
        {                                                                      \
            /* create new list */                                              \
            t = (name##_list_internal_t *)s16mem_alloc (                       \
                sizeof (name##_list_internal_t));                              \
            t->val = data;                                                     \
            t->Link = NULL;                                                    \
            n->List = t;                                                       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            t = n->List;                                                       \
            temp = (name##_list_internal_t *)s16mem_alloc (                    \
                sizeof (name##_list_internal_t));                              \
            temp->val = data;                                                  \
            temp->Link = t;                                                    \
            n->List = temp;                                                    \
        }                                                                      \
        return n;                                                              \
    }                                                                          \
    INLINE int name##_list_size (name##_list_t * n)                            \
    {                                                                          \
        int cnt = 0;                                                           \
        list_foreach (name, n, it) cnt++;                                      \
        return cnt;                                                            \
    }                                                                          \
    /* and convenience wrappers are here */                                    \
    INLINE name##_list_it name##_list_find_cmp (                               \
        name##_list_t * n, bool (*fun) (const type, const type),               \
        const type extra)                                                      \
    {                                                                          \
        list_foreach (name, n, it) if (fun (it->val, extra)) return it;        \
        return NULL;                                                           \
    }                                                                          \
    INLINE name##_list_it name##_list_find_int (                               \
        name##_list_t * n, bool (*fun) (type, int), int extra)                 \
    {                                                                          \
        list_foreach (name, n, it)                                             \
        {                                                                      \
            if (fun (it->val, extra))                                          \
                return it;                                                     \
        }                                                                      \
        return NULL;                                                           \
    }                                                                          \
    typedef int (*name##_list_walk_cmp_fn) (type, type);                       \
    /* For each element e of a list, run fun(e). Duplicated from list_walk to  \
     * avoid warnings. */                                                      \
    INLINE int name##_list_walk_cmp (const name##_list_t * n,                  \
                                     name##_list_walk_cmp_fn fun, type ex)     \
    {                                                                          \
        int r = 0;                                                             \
        list_foreach (name, n, it)                                             \
        {                                                                      \
            int fr = fun (it->val, ex);                                        \
            r = fr ? fr : 0;                                                   \
        }                                                                      \
        return r;                                                              \
    }                                                                          \
    typedef type (*name##_list_map_fn) (const type);                           \
    INLINE name##_list_t name##_list_map (const name##_list_t * n,             \
                                          name##_list_map_fn fun)              \
    {                                                                          \
        name##_list_t nlist = name##_list_new ();                              \
        list_foreach (name, n, it)                                             \
        {                                                                      \
            name##_list_add (&nlist, fun (it->val));                           \
        }                                                                      \
        return nlist;                                                          \
    }

#define list_foreach(name, list, as)                                           \
    for (name##_list_it tmp, as = list_begin (list);                           \
         (as != NULL) && (tmp = list_next (as), list); as = tmp)
#define list_begin(x) (x)->List
#define list_next(x) (x)->Link
#define list_it_val(x) x ? x->val : NULL

/* New API? */
#define LL_each2(name, list, as) list_foreach (name, list, as)
#define LL_each(list, as)                                                      \
    for (__typeof__ ((list)->List) tmp, as = list_begin (list);                \
         (as != NULL) && (tmp = list_next (as), list); as = tmp)
#define LL_empty(list) (((list)->List) == NULL)
#define LL_print(list)                                                         \
    {                                                                          \
        int cnt = 0;                                                           \
        LL_each (list, it)                                                     \
        {                                                                      \
            printf ("%d: %p\n", cnt++, (void *)(uintptr_t)it->val);            \
        }                                                                      \
    }

#ifdef __cplusplus
}
#endif

#endif
