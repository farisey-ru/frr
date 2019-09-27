#ifndef _FRR_JANSSON_COMPAT_H_
#define _FRR_JANSSON_COMPAT_H_

/* compat layer, defined in jansson_compat.c */

#define JSON_C_TO_STRING_SPACED     (1<<0)
#define JSON_C_TO_STRING_PRETTY     (1<<1)

typedef int json_bool;

enum jc_type {
	jc_type_null,
	jc_type_boolean,
	jc_type_double,
	jc_type_int,
	jc_type_object,
	jc_type_array,
	jc_type_string
};

enum jc_type jc_object_get_type(const void *obj);

/* constructors */
void *jc_object_new_object(void);
void *jc_object_new_string(const char *s);
void *jc_object_new_boolean(json_bool b);
void *jc_object_new_array(void);
void *jc_object_new_int(int32_t i);
/* ... through load */
void *jc_object_from_file(const char *filename);
void *jc_tokener_parse(const char *str);

/* refcount */
void *jc_object_get(void *obj);
int jc_object_put(void *obj);

/* get */
json_bool jc_object_object_get_ex(void *obj, const char *key,
                                  void **value);
const char *jc_object_get_string(void *obj);
int32_t jc_object_get_int(const void *obj);
void *jc_object_array_get_idx(void *obj, int idx);
json_bool jc_object_get_boolean(void *obj);
int64_t jc_object_get_int64(void *obj);

/* add */
void jc_object_object_add(void *obj, const char *key, void *val);
void jc_object_int_add(void *obj, const char *key, int64_t i);
int jc_object_array_add(void *obj, void *val);


const char *jc_object_to_json_string_ext(void *obj, int flags);
int jc_object_array_length(void *obj);


/* iterators */
const char *jc_object_iter_key(void *iter);
void *jc_object_iter(void *object);
void *jc_object_iter_value(void *iter);
void *jc_object_key_to_iter(const char *key);
void *jc_object_iter_next(void *object, void *iter);

typedef struct jc_object_iterator {
	const char *key;
	void *value;
} jc_object_iterator;



/* all of below will be hidden from jansson_compat.c */

#ifndef HIDE_LIBJSON_DECLS
struct json_object;
typedef struct json_object json_object;

typedef enum json_type {
	json_type_null,
	json_type_boolean,
	json_type_double,
	json_type_int,
	json_type_object,
	json_type_array,
	json_type_string
} json_type;

/* a large set of simple wrappers */

static inline
enum json_type json_object_get_type(const struct json_object *obj)
{
	return (enum json_type)jc_object_get_type(obj);
}

static inline
struct json_object *json_object_new_object(void)
{
	return jc_object_new_object();
}

static inline
struct json_object *json_object_new_string(const char *s)
{
	return jc_object_new_string(s);
}

static inline
struct json_object *json_object_new_boolean(json_bool b)
{
	return jc_object_new_boolean(b);
}

static inline
struct json_object *json_object_new_array(void)
{
	return jc_object_new_array();
}

static inline
struct json_object *json_object_new_int(int32_t i)
{
	return jc_object_new_int(i);
}

static inline
struct json_object *json_object_from_file(const char *filename)
{
	return jc_object_from_file(filename);
}

static inline
struct json_object *json_tokener_parse(const char *str)
{
	return jc_tokener_parse(str);
}

static inline
struct json_object *json_object_get(struct json_object *obj)
{
	return jc_object_get(obj);
}

static inline
int json_object_put(struct json_object *obj)
{
	return jc_object_put(obj);
}

static inline
json_bool json_object_object_get_ex(struct json_object *obj,
                                    const char *key,
                                    struct json_object **value)
{
	return jc_object_object_get_ex(obj, key, (void *)value);
}

static inline
const char *json_object_get_string(struct json_object *obj)
{
	return jc_object_get_string(obj);
}

static inline
int32_t json_object_get_int(const struct json_object *obj)
{
	return jc_object_get_int(obj);
}

static inline
struct json_object *json_object_array_get_idx(struct json_object *obj, int idx)
{
	return jc_object_array_get_idx(obj, idx);
}

static inline
json_bool json_object_get_boolean(struct json_object *obj)
{
	return jc_object_get_boolean(obj);
}

static inline
int64_t json_object_get_int64(struct json_object *obj)
{
	return jc_object_get_int64(obj);
}

static inline
void json_object_object_add(struct json_object *obj, const char *key,
                            struct json_object *val)
{
	jc_object_object_add(obj, key, val);
}

static inline
const char *json_object_to_json_string_ext(struct json_object *obj,
        int flags)
{
	return jc_object_to_json_string_ext(obj, flags);
}

static inline
int json_object_array_length(struct json_object *obj)
{
	return jc_object_array_length(obj);
}


static inline
int json_object_array_add(struct json_object *obj,
                          struct json_object *val)
{
	return jc_object_array_add(obj, val);
}

static inline
const char *json_object_to_json_string(struct json_object *obj)
{
	return json_object_to_json_string_ext(obj, JSON_C_TO_STRING_SPACED);
}


/* json_object_foreach_safe() of jansson */
#define json_object_object_foreach(object, key, value) 							\
	const char *key = NULL;														\
	struct json_object *value = NULL;											\
	void *__n_tmp;																\
    for(key = jc_object_iter_key(jc_object_iter(object)),						\
            __n_tmp = jc_object_iter_next(object, jc_object_key_to_iter(key));	\
        key && (value = jc_object_iter_value(jc_object_key_to_iter(key)));		\
        key = jc_object_iter_key(__n_tmp),										\
            __n_tmp = jc_object_iter_next(object, jc_object_key_to_iter(key)))

struct json_object_iterator {
	const char *key;
	void *value;
};

/* json_object_foreach() of jansson
 * TODO: refactor with a stored 'iter'.
 */
#define JSON_FOREACH(jo, joi, join)							\
	/* struct json_object *jo; */							\
	/* struct json_object_iterator joi; */					\
	/* struct json_object_iterator join; */					\
    for((joi).key = jc_object_iter_key(jc_object_iter(jo)),	\
        (join).key = "unused";								\
															\
        (joi).key &&										\
        (join).key &&										\
        ((joi).value =										\
		  jc_object_iter_value(jc_object_key_to_iter((joi).key)));	\
															\
        (joi).key =											\
         jc_object_iter_key(jc_object_iter_next(jo, jc_object_key_to_iter((joi).key))))

static inline const char *
json_object_iter_peek_name(const struct json_object_iterator *iter)
{
	return iter->key;
}

static inline struct json_object *
json_object_iter_peek_value(const struct json_object_iterator *iter)
{
	return iter->value;
}

#endif	/* HIDE_LIBJSON_DECL */

#endif
