#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_JANSSON_H)

#include <jansson.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#define HIDE_LIBJSON_DECLS
#include "lib/jansson_compat.h"

enum jc_type jc_object_get_type(const void *obj)
{
	json_t *js = (json_t *)obj;

	if (!js)
		return jc_type_null;

	switch (json_typeof(js)) {
		case JSON_OBJECT:
			return jc_type_object;

		case JSON_ARRAY:
			return jc_type_array;

		case JSON_STRING:
			return jc_type_string;

		case JSON_INTEGER:
			return jc_type_int;

		case JSON_REAL:
			return jc_type_double;

		case JSON_TRUE:
		case JSON_FALSE:
			return jc_type_boolean;

		case JSON_NULL:
			return jc_type_null;
	}

	return jc_type_null;
}

/* jansson constructors */
void *jc_object_new_object(void)
{
	return json_object();
}

void *jc_object_new_string(const char *s)
{
	return json_string(s);
}

void *jc_object_new_boolean(json_bool b)
{
	return json_boolean(b);
}

void *jc_object_new_array(void)
{
	return json_array();
}

void *jc_object_new_int(int32_t i)
{
	return json_integer(i);
}

/* load */
void *jc_object_from_file(const char *filename)
{
	json_error_t jerr;
	return json_load_file(filename, 0, &jerr);
}
void *jc_tokener_parse(const char *str)
{
	json_error_t jerr;
	return json_loads(str, 0, &jerr);
}

/* refcount */
void *jc_object_get(void *obj)
{
	return json_incref(obj);
}

/* return 1 if freed */
int jc_object_put(void *obj)
{
	json_t *js = (json_t *)obj;
	int ret = !!(js && (js->refcount == 1));
	json_decref(js);
	return ret;
}


/* get */
json_bool jc_object_object_get_ex(void *obj, const char *key,
                                  void **value)
{
	json_t *ret = json_object_get(obj, key);

	if (json_is_null(ret))
		ret = NULL;

	if (value)
		*value = ret;

	return ret ? 1 : 0;
}

const char *jc_object_get_string(void *obj)
{
	json_t *js = (json_t *)obj;

	if (!js)
		return NULL;

	if (json_typeof(js) == JSON_STRING)
		return json_string_value(js);

	return jc_object_to_json_string_ext(obj, JSON_C_TO_STRING_SPACED);
}

int32_t jc_object_get_int(const void *obj)
{
	json_t *js = (json_t *)obj;
	return json_integer_value(js);
}

void *jc_object_array_get_idx(void *obj, int idx)
{
	return json_array_get(obj, idx);
}

json_bool jc_object_get_boolean(void *obj)
{
	json_t *js = (json_t *)obj;

	if (!js)
		return 0;

	switch (json_typeof(js)) {
		case JSON_STRING:
			return (json_string_length(js) != 0);

		case JSON_INTEGER:
			return (json_integer_value(js) != 0);

		case JSON_REAL:
			return (json_real_value(js) != 0.0);

		case JSON_TRUE:
			return 1;

		//case JSON_FALSE:
		//	return 0;
		default:
			break;
	}

	return 0;
}

int64_t jc_object_get_int64(void *obj)
{
	json_t *js = (json_t *)obj;
	int64_t ret;

	if (!js)
		return 0;

	switch (json_typeof(js)) {
		case JSON_STRING:
			errno = 0;
			ret = strtoll(json_string_value(js), NULL, 0);

			if (errno == ERANGE)
				return 0;

			return ret;

		case JSON_INTEGER:
			return json_integer_value(js);

		case JSON_REAL: {
			double x = json_real_value(js);

			if (x <= INT64_MIN)
				return INT64_MIN;

			if (x >= INT64_MAX)
				return INT64_MAX;

			return (int64_t)x;
		}

		case JSON_TRUE:
			return 1;

		//case JSON_FALSE:
		//	return 0;
		default:
			break;
	}

	return 0;
}


/* add */

/* refcount will NOT incremented */
void jc_object_object_add(void *obj, const char *key, void *val)
{
	json_object_set_new(obj, key, val);
}

void jc_object_int_add(void *obj, const char *key, int64_t i)
{
	json_object_set_new(obj, key, json_integer(i));
}

int jc_object_array_add(void *obj, void *val)
{
	return json_array_append_new(obj, val);
}

/* dump*/

/*
 * jansson does not have internal print-buffer for objects.
 * Of course, I can wrap all ctors but cannot control dtors.
 * We also assume libjansson is vanilla, not patched.
 * Thus, let's use hacky "circular set" of pointers.
 */
#if !defined(__GNUC__) && !defined(__clang__)
#error need gcc or clang for ctor/dtor function attributes.
#endif

#define JC_STR_NUM	8
static char *jc_pbufs[JC_STR_NUM];	/* .bss */
static int jc_pb_idx = 0;

__attribute__((constructor))
static void
jc_init(void)
{
	/* redundant on Linux, since .bss is zeroed on start */
	memset(jc_pbufs, 0, sizeof(jc_pbufs));
}

__attribute__((destructor))
static void
jc_fini(void)
{
	int i;

	for (i = 0; i < JC_STR_NUM; i++)
		if (jc_pbufs[i])
			free(jc_pbufs[i]);
}

const char *jc_object_to_json_string_ext(void *obj, int flags)
{
	size_t jansson_flags = 0; /* ~ JSON_C_TO_STRING_SPACED */
	char *ret;

	if (!flags)
		jansson_flags |= JSON_COMPACT;
	else if (flags & JSON_C_TO_STRING_PRETTY)
		jansson_flags |= JSON_INDENT(2);

	ret = json_dumps(obj, jansson_flags);

	if (!ret)
		return "<NULL>"; /* avoid real NULL */

	if (jc_pbufs[jc_pb_idx])
		free(jc_pbufs[jc_pb_idx]);

	jc_pbufs[jc_pb_idx] = ret;

	if (++jc_pb_idx >= JC_STR_NUM)
		jc_pb_idx = 0;

	return ret;
}



int jc_object_array_length(void *obj)
{
	return json_array_size(obj);
}



/* iterators */
const char *jc_object_iter_key(void *iter)
{
	return json_object_iter_key(iter);
}
void *jc_object_iter(void *object)
{
	return json_object_iter(object);
}
void *jc_object_iter_value(void *iter)
{
	return json_object_iter_value(iter);
}
void *jc_object_key_to_iter(const char *key)
{
	return json_object_key_to_iter(key);
}
void *jc_object_iter_next(void *object, void *iter)
{
	return json_object_iter_next(object, iter);
}


#endif /* HAVE_JANSSON_H */
