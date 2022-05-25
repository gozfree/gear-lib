/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#ifndef LIBPTCP_H
#define LIBPTCP_H

#include <libposix.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DISABLE_COMPILE  0

#define G_BEGIN_DECLS
#define G_END_DECLS

//typedef void *          GObjectClass;
typedef void *          GObject;
typedef void *          GType;
typedef void *          GInputVector;
typedef void *          GValue;
typedef void *          GParamSpec;

typedef void *          gpointer;
typedef bool            gboolean;
typedef char            gchar;
typedef int             gint;
typedef unsigned char   guchar;
typedef unsigned short  gushort;
typedef unsigned int    guint;
typedef unsigned long   gulong;
typedef size_t          gsize;
typedef int8_t          gint8;
typedef uint8_t         guint8;
typedef int16_t         gint16;
typedef uint16_t        guint16;
typedef int32_t         gint32;
typedef uint32_t        guint32;
typedef uint64_t        guint64;
typedef int64_t         gint64;
typedef struct _GTypeClass              GTypeClass;
typedef struct _GSList GSList;

struct _GSList
{
  gpointer data;
  GSList *next;
};

struct _GTypeClass
{
  /*< private >*/
  GType g_type;
};

struct _GObjectConstructParam
{
  GParamSpec *pspec;
  GValue     *value;
};
typedef struct _GObjectConstructParam    GObjectConstructParam;

struct  _GObjectClass
{
  GTypeClass   g_type_class;

  /*< private >*/
  GSList      *construct_properties;

  /*< public >*/
  /* seldom overidden */
  GObject*   (*constructor)     (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties);
  /* overridable methods */
  void       (*set_property)		(GObject        *object,
                                         guint           property_id,
                                         const GValue   *value,
                                         GParamSpec     *pspec);
  void       (*get_property)		(GObject        *object,
                                         guint           property_id,
                                         GValue         *value,
                                         GParamSpec     *pspec);
  void       (*dispose)			(GObject        *object);
  void       (*finalize)		(GObject        *object);
  /* seldom overidden */
  void       (*dispatch_properties_changed) (GObject      *object,
					     guint	   n_pspecs,
					     GParamSpec  **pspecs);
  /* signals */
  void	     (*notify)			(GObject	*object,
					 GParamSpec	*pspec);

  /* called when done constructing */
  void	     (*constructed)		(GObject	*object);

  /*< private >*/
  gsize		flags;

  /* padding */
  gpointer	pdummy[6];
};
typedef struct _GObjectClass             GObjectClass;

#define G_MAXUINT32	((guint32) 0xffffffff)

typedef struct {
    void *data;
    struct list_head entry;
} GList;

typedef struct {
    GList member;
    guint length;
} GQueue;

void g_queue_init(GQueue *q);
void g_queue_clear(GQueue *q);
guint g_queue_get_length(GQueue *q);
gpointer g_queue_pop_head(GQueue *q);
gpointer g_queue_peek_head(GQueue *q);
gpointer g_queue_peek_tail(GQueue *q);
void g_queue_push_tail(GQueue *q, gpointer data);
GList *g_queue_find(GQueue *q, const void *data);
void g_queue_insert_after(GQueue *q, GList *sibling, gpointer data);
GList *g_queue_peek_head_link(GQueue *q);
GList *g_queue_peek_tail_link(GQueue *q);


#ifndef TRUE
#define TRUE (1 == 1)
#endif

#ifndef FALSE
#define FALSE (0 == 1)
#endif

#define G_DEFINE_TYPE(TN, t_n, T_P)	void __func_dummy() {};
#define g_new0(type, num)           calloc(sizeof(type), num)
#define g_slice_new0(type)          malloc(sizeof(type))
#define g_slice_alloc               malloc
#define g_slice_free(arg0, arg1)    free(arg1)
#define g_slice_free1(arg0, arg1)   free(arg1)
#define g_assert                    assert
#define g_assert_not_reached()      assert(0)
#define G_UNLIKELY                  UNLIKELY
#define G_LIKELY                    LIKELY
#define g_object_ref(a)             do {} while (0)
#define g_object_unref(a)           do {} while (0)

#define G_GSIZE_FORMAT "I64u"
gint64 g_get_monotonic_time();

typedef enum
{
  /* log flags */
  G_LOG_FLAG_RECURSION          = 1 << 0,
  G_LOG_FLAG_FATAL              = 1 << 1,

  /* GLib log levels */
  G_LOG_LEVEL_ERROR             = 1 << 2,       /* always fatal */
  G_LOG_LEVEL_CRITICAL          = 1 << 3,
  G_LOG_LEVEL_WARNING           = 1 << 4,
  G_LOG_LEVEL_MESSAGE           = 1 << 5,
  G_LOG_LEVEL_INFO              = 1 << 6,
  G_LOG_LEVEL_DEBUG             = 1 << 7,

  G_LOG_LEVEL_MASK              = ~(G_LOG_FLAG_RECURSION | G_LOG_FLAG_FATAL)
} GLogLevelFlags;

void g_log (const gchar *log_domain, GLogLevelFlags log_level,
                const gchar *format, ...);

#define g_assert_cmpuint(n1, cmp, n2)   do { guint64 __n1 = (n1), __n2 = (n2); \
                                             if (__n1 cmp __n2) ; else \
                                               g_log(NULL, G_LOG_LEVEL_ERROR, \
                                                 #n1 " " #cmp " " #n2, __n1, #cmp, __n2, 'i'); } while (0)
#ifdef __cplusplus
}
#endif
#endif
