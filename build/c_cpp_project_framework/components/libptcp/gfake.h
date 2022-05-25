#ifndef __G_FAKE_H__
#define __G_FAKE_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <limits.h>

#ifdef  __cplusplus
#define G_BEGIN_DECLS  extern "C" {
#define G_END_DECLS    }
#else
#define G_BEGIN_DECLS
#define G_END_DECLS
#endif

#define GLIB_AVAILABLE_IN_ALL
#define GLIB_AVAILABLE_IN_2_34
#define G_GNUC_WARN_UNUSED_RESULT

#define G_MAXUINT32     UINT32_MAX

#define gchar           char

#define guint8          uint8_t
#define guint16         uint16_t
#define gint32          int32_t
#define guint32         uint32_t
#define gint64          int64_t
#define guint64         uint64_t

#define gint            int
#define guint           unsigned int

#define glong           long

#define gsize           size_t

#define gboolean        bool
#ifndef TRUE
#define TRUE            true
#endif
#ifndef FALSE
#define FALSE           false
#endif

#define gpointer        void *
#define gconstpointer   const void *

#define G_GUINT64_FORMAT "llu"

#define G_GSIZE_FORMAT "zu"
#define G_GSSIZE_FORMAT "zi"

#if defined(__GNUC__)
#define _BOOLEAN_EXPR(expr)                     \
    __extension__ ({                            \
        int _boolean_var_;                      \
        if (expr)                               \
            _boolean_var_ = 1;                  \
        else                                    \
            _boolean_var_ = 0;                  \
        _boolean_var_;                          \
    })
#define G_LIKELY(expr) (__builtin_expect (_BOOLEAN_EXPR((expr)), 1))
#define G_UNLIKELY(expr) (__builtin_expect (_BOOLEAN_EXPR((expr)), 0))
#else
#define G_LIKELY(expr) (expr)
#define G_UNLIKELY(expr) (expr)
#endif

typedef gpointer        (*GCopyFunc)            (gconstpointer  src,
                                                 gpointer       data);
typedef gint            (*GCompareFunc)         (gconstpointer  a,
                                                 gconstpointer  b);
typedef gint            (*GCompareDataFunc)     (gconstpointer  a,
                                                 gconstpointer  b,
                                                 gpointer       user_data);
typedef void            (*GDestroyNotify)       (gpointer       data);
typedef void            (*GFunc)                (gpointer       data,
                                                 gpointer       user_data);
typedef gboolean        (*GSourceFunc)          (gpointer       user_data);
typedef void *          GObject;
typedef void           GValue;
typedef void *          GParamSpec;

typedef gsize                           GType;
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


#define g_slice_alloc(size)                     malloc(size)
#define g_slice_free1(size, ptr)                free(ptr)
#define g_slice_free(type, ptr)                 free(ptr)
#define g_slice_new(type)                       malloc(sizeof(type))
#define g_slice_new0(type)                      calloc(1, sizeof(type))
#define g_malloc(size)                          malloc(size)
#define g_free(ptr)                             free(ptr)
#define g_new0(type, num)                       calloc(sizeof(type), num)

//#define g_object_ref(obj)                       ref(obj)
//#define g_object_unref(obj)                     deref(obj)
#define g_object_ref(a)             do {} while (0)
#define g_object_unref(a)           do {} while (0)

#define PSEUDO_TCP_SOCKET(obj) (PseudoTcpSocket *)(*obj)
#define G_OBJECT_CLASS(cls) (GObjectClass *)(cls)

#define g_value_set_uint(value, type)  do {*(guint32 *)value=type;}while (0)
#define g_value_set_pointer(value, type)  do {*(gpointer *)value=type;}while (0)
#define g_value_set_boolean(value, type)  do {*(gboolean *)value=type;}while (0)
#define g_value_get_uint(value)  *(guint32 *)value
#define g_value_get_pointer(value)  *(gpointer *)value
#define g_value_get_boolean(value)  *(gboolean *)value
#if 0
void
g_value_set_uint (GValue *value,
		  guint	  v_uint)
{
  g_return_if_fail (G_VALUE_HOLDS_UINT (value));
  
  value->data[0].v_uint = v_uint;
}
#endif


//#define g_log(tag, level, fmt, ...)             vlog(level, fmt, ## __VA_ARGS__)

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



//#define G_LOG_LEVEL_DEBUG                       VLOG_DEBUG

static inline
void
g_slice_free_chain_with_offset (gsize    mem_size,
                                gpointer mem_chain,
                                gsize    next_offset)
{
    void *ptr = mem_chain;

    while (ptr) {
        char *current = (char *)ptr;
        ptr = *(void **) (current + next_offset);
        free(current);
    }
}

#if (defined(__GNUC__)  && __GNUC__ >= 4) || defined (_MSC_VER)
#define G_STRUCT_OFFSET(struct_type, member) \
        ((glong) offsetof (struct_type, member))
#else
#define G_STRUCT_OFFSET(struct_type, member)	\
        ((glong) ((guint8*) &((struct_type*) 0)->member))
#endif

#define g_slice_free_chain(type, mem_chain, next)               \
    do {                                                        \
        if (1) g_slice_free_chain_with_offset (sizeof (type),	\
                (mem_chain), G_STRUCT_OFFSET (type, next)); 	\
        else   (void) ((type*) 0 == (mem_chain));               \
    } while (0)

#define g_debug(fmt, ...)                   \
    printf("gFake [%s:%d]: " fmt, __FILE__, __LINE__, ## __VA_ARGS__)

#define g_warning(fmt, ...)                 \
    printf("gFake [%s:%d]: " fmt, __FILE__, __LINE__, ## __VA_ARGS__)

#define g_error(fmt, ...)                   \
    printf("gFake [%s:%d]: " fmt, __FILE__, __LINE__, ## __VA_ARGS__)

#ifdef NDEBUG
#define g_assert(expr)         if (expr) {}
#define g_return_if_fail(expr) do{ (void)0; } while (0)
#define g_return_val_if_fail(expr,val) do { (void)0; } while (0)
#define g_assert_not_reached() do { (void) 0; } while (0)
#else
#define g_assert(expr)          assert(expr)
#define g_return_if_fail(expr) do {                     \
    if G_LIKELY(expr) { } else                          \
    {                                                   \
        g_warning (#expr);                              \
        return;                                         \
    };                                                  \
} while (0)

#define g_return_val_if_fail(expr,val) do {             \
    if G_LIKELY((expr)) { } else                        \
    {                                                   \
        g_warning (#expr);                              \
        return (val);                                   \
    };                                                  \
} while (0)

#define g_assert_not_reached()                          \
    assert(0 && __FILE__ && __LINE__)

#endif

#define g_assert_cmpuint(n1, cmp, n2)   do {            \
    guint64 __n1 = (n1), __n2 = (n2);                   \
    if (__n1 cmp __n2) ; else                           \
    assert(0 && __FILE__ && __LINE__ &&  #n1 " " #cmp " " #n2);        \
} while (0)

gint64 g_get_monotonic_time();

#endif /* __G_FAKE_H__ */
