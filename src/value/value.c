#include "value.h"
struct Value_ {
	size_t refcount;
	enum type { INTEGER, VECTOR } type; /* TODO: Add other types. */
	union {
		struct { /* Vector */
			/* Inspired by Roger Hui's An Implementation of J */
			enum type vec_type; /* TODO: Add other types. */
			size_t elecount;
			unsigned long rank;
			size_t alloccount;
			unsigned long sd[1]; /* Shape & Data array. */
			/* Preallocated for singleton case. (Data: 1 value). */
		};
	};
};

Value value_make_number(unsigned long value)
{
	Value num = malloc(sizeof *num); /* Singleton values are presized. */
	assert(num); /* TODO: Error handling */
	num->refcount = 1;
	num->rank = 0;
	num->elecount = 1;
	num->alloccount = 1;
	num->vec_type = INTEGER;
	num->sd[0] = value;
	return num;
}

Value value_make_vector(unsigned long value)
{
	const unsigned long init_rank = 1;
	const unsigned long init_alloccount = 10;
	Value vec = malloc(sizeof *vec + sizeof vec->sd[0] * (10 + 1));
	assert(vec); /* TODO: Error handling */
	vec->refcount = 1;
	vec->rank = init_rank;
	vec->elecount = 1;
	vec->alloccount = init_alloccount;
	vec->vec_type = INTEGER;
	vec->sd[0] = 1;
	vec->sd[1] = value;
	return vec;
}

Value value_extend_vector(Value v, unsigned long val)
{
	if (v->elecount == v->alloccount) {
		v->alloccount *= 2;
		v = realloc(v, sizeof *v + sizeof v->sd[0] * (v->alloccount + v->rank));
	}
	assert(v);
	v->sd[v->rank - 1]++;
	v->sd[v->rank - 1 + v->elecount++ + 1] = val;
	return v;
}

void value_free(Value v)
{
	assert(v);
	v->refcount--;
	if (v->refcount == 0) {
		free(v);
	}
}

char *value_stringify(Value v)
{
	const size_t UINT64_DIGITS = 20; /* log(2^64) / log(10) = 19.3 ~ 20. */
	/* I.e. the maximum length of an integer printed in decimal. */
	const size_t len = (UINT64_DIGITS + 1) * (v->elecount + 2) * (v->rank + 1);
	/* ' ' between values, 2 '\n's between dimensions, and '\0' terminator. */
	char *tmp = malloc(len);
	assert(tmp); /* TODO: Error handling */
	for (size_t i = 0, pos = 0; i < v->elecount; ++i) {
		pos += snprintf((tmp + pos), len - pos, "%ld ", v->sd[v->rank + i]);
	}
	return tmp;
}

/* Determines rank before which a and w agree. Assumes rank a >= rank w */
static size_t agreed_prefix(Value a, Value w)
{
	for (size_t i = 0; i < w->rank; ++i) {
		if (a->sd[i] != w->sd[i]) {
			return i;
		}
	}
	return w->rank;
}

/* Creates a Value with the same shape, type, and elecount as that given. */
static Value copy_value_container(Value v)
{
	/* NOTE: This is actually oversizing the array for many types. */
	Value cpy = malloc(sizeof *cpy + sizeof v->sd[0] * (v->elecount + v->rank));
	assert(cpy); /* TODO: Error handling */
	cpy->refcount = 1;
	cpy->rank = v->rank;
	cpy->elecount = v->elecount;
	cpy->alloccount = v->elecount;
	cpy->vec_type = v->vec_type;
	memcpy(&cpy->sd[0], &v->sd[0], sizeof cpy->sd[0] * cpy->rank);
	return cpy;
}

Value add_values(Value a, Value w)
{
	if (a->rank > w->rank) { /* Addition is commutative, so swap. */
		Value t = a;
		a = w;
		w = t;
	} /* Now a->rank <= w->rank */
	Value sum = copy_value_container(w);
	assert(sum);
	if (a->rank == 0 && w->rank == 0) {
		sum->sd[0] = w->sd[0] + a->sd[0];
		return sum;
	}
	assert(agreed_prefix(a, w) > 0); /* TODO: Error handling mismatched ranks */
	for (size_t shape = 0, i = 0; shape < agreed_prefix(a, w); ++shape) {
		for (size_t k = 0; k < w->sd[shape]; k++, i++) {
			sum->sd[sum->rank + i] = w->sd[w->rank + i] + a->sd[a->rank + i];
		}
	}
	return sum;
}
