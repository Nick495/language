#include "value.h"
struct Value_ {
	size_t refcount;
	enum type { INTEGER, VECTOR } type; /* TODO: Add other types. */
	union {
		struct { /* Vector */
			/* Inspired by Roger Hui's An Implementation of J */
			enum type vec_type; /* TODO: Add other types. */
			size_t elecount;
			long rank;
			long sd[2]; /* Shape & Data array. */
			/* Preallocated for singleton case. (Shape [1], Data: 1 value). */
		};
	};
};

Value value_make_number(size_t value)
{
	Value num = malloc(sizeof *num); /* Singleton values are presized. */
	assert(num); /* TODO: Error handling */
	num->refcount = 1;
	num->rank = 0;
	num->elecount = 1;
	num->vec_type = INTEGER;
	num->sd[0] = 1;
	num->sd[1] = value;
	return num;
}

Value value_make_vec(long *arr, size_t elecount, size_t rank, long *shape)
{
	/* NOTE: This is actually oversizing the array for many types. */
	Value vec = malloc(sizeof *vec + sizeof vec->sd[0] * (elecount + rank));
	assert(vec); /* TODO: Error handling */
	vec->refcount = 1;
	vec->rank = rank;
	vec->elecount = elecount;
	vec->vec_type = INTEGER; /* All that's implemented so far. */
	memcpy(&vec->sd[0], shape, sizeof *shape * rank);
	memcpy(&vec->sd[rank], arr, sizeof *arr * elecount);
	return vec;
}

Value add_values(Value a, Value w)
{
	if (a->rank > w->rank) {
		Value t = a;
		a = w;
		w = t;
	}
	/* Addition is commutative, so swap. */
	/* Now a->rank <= w->rank */
	/* TODO: Get rid of offsets */
	const size_t w_off = w->rank == 0 ? 1 : 0;
	const size_t a_off = a->rank == 0 ? 1 : 0;
	Value sum = value_make_vec(&w->sd[w_off], w->elecount, w->rank, &w->sd[0]);
	/* TODO: Implement prefix agreement (find rank up-to which a and w agree) */
	/* TODO: Implement for different ranks. */
	/* TODO: Index = 1, hack? */
	for (size_t i = 0, index = 1; i < (size_t)(w->rank - a->rank + 1); ++i) {
		for (size_t k = 0; k < (size_t)w->sd[i]; k++, index++) {
			sum->sd[index + w->rank] = w->sd[index + w->rank] + a->sd[index + a->rank];
		}
	}
	return sum;
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
	const size_t len = (UINT64_DIGITS + 1) * v->elecount + 2 * v->rank + 1;
	/* ' ' between values, 2 '\n's between dimensions, and '\0' terminator. */
	char *tmp = malloc(len);
	assert(tmp); /* TODO: Error handling */
	/* TODO: Fix me to use rank & shape. */
	for (size_t i = 0, pos = 0; i < v->elecount; ++i) {
		pos += snprintf(tmp, len -pos, "%ld ", v->sd[i +v->rank == 0 ? 1 : v->rank]);
	}
	return tmp;
}
