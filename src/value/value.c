#include "value.h"
struct Value_ {
	size_t refcount;
	enum type { INTEGER, VECTOR } type; /* TODO: Add other types. */
	union {
		struct { /* Vector */
			/* Inspired by Roger Hui's An Implementation of J */
			enum type vec_type; /* TODO: Add other types. */
			size_t ecount; /* Number of elements used. */
			size_t acount; /* Number of elements allocated. */
			unsigned long rank;
			unsigned long sd[1]; /* Shape & Data array. */
			/* Preallocated for singleton case. (Data: 1 value). */
		};
	};
};

static void print_value(Value v)
{
	fprintf(stderr, "DEBUG:\n");
	fprintf(stderr, "refcount: %zu\n", v->refcount);
	switch(v->type) {
	case VECTOR:
		fprintf(stderr, "type: vector\n");
		fprintf(stderr, "vec_type: %d\n", v->vec_type);
		fprintf(stderr, "ecount: %zu\n", v->ecount);
		fprintf(stderr, "acount: %zu\n", v->acount);
		fprintf(stderr, "rank: %lu\n", v->rank);
		break;
	case INTEGER:
		fprintf(stderr, "type: integer\n");
		break;
	default:
		fprintf(stderr, "type: invalid\n");
	}
	return;
}

Value value_make_number(unsigned long value)
{
	Value num = mem_alloc(sizeof *num); /* Singleton values are presized. */
	assert(num); /* TODO: Error handling */
	num->refcount = 1;
	num->rank = 0;
	num->ecount = 1;
	num->acount = 1;
	num->vec_type = INTEGER;
	num->type = INTEGER;
	num->sd[0] = value;
	return num;
}

Value value_make_vector(unsigned long value)
{
	const size_t init_count = 10;
	const unsigned long init_rank = 1;
	Value vec = mem_alloc(sizeof *vec + sizeof vec->sd[0] * (init_count + 1));
	assert(vec); /* TODO: Error handling */
	vec->refcount = 1;
	vec->rank = init_rank;
	vec->ecount = 1;
	vec->acount = init_count;
	vec->vec_type = INTEGER;
	vec->type = VECTOR;
	vec->sd[0] = 1;
	vec->sd[1] = value;
	return vec;
}

Value value_append(Value v, unsigned long val)
{
	if (v->ecount == v->acount) {
		v->acount *= 2;
		v = mem_realloc(v, sizeof *v + sizeof v->sd[0] * (v->acount + v->rank));
	}
	assert(v);
	v->sd[v->rank - 1]++;
	v->sd[v->rank - 1 + v->ecount++ + 1] = val;
	return v;
}

void value_free(Value v)
{
	assert(v);
	v->refcount--;
	if (v->refcount == 0) {
		mem_dealloc(v);
	}
}

char *value_stringify(Value v)
{
	const size_t UINT64_DIGITS = 20; /* log(2^64) / log(10) = 19.3 ~ 20. */
	/* I.e. the maximum length of an integer printed in decimal. */
	const size_t len = (UINT64_DIGITS + 1) * (v->ecount + 2) * (v->rank + 1);
	/* ' ' between values, 2 '\n's between dimensions, and '\0' terminator. */
	char *tmp = mem_alloc(len);
	assert(tmp); /* TODO: Error handling */
	size_t pos = 0;
	for (size_t i = 0; i < v->ecount; ++i) {
		pos += snprintf((tmp + pos), len - pos, "%ld ", v->sd[v->rank + i]);
	}
	tmp[--pos] = '\0'; /* Remove trailing space */
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

/* Creates a Value with the same shape, type, and ecount as that given. */
static Value copy_value_container(Value v)
{
	/* NOTE: This is actually oversizing the array for many types. */
	Value cpy = mem_alloc(sizeof *cpy +sizeof v->sd[0] *(v->ecount +v->rank));
	assert(cpy); /* TODO: Error handling */
	cpy->refcount = 1;
	cpy->rank = v->rank;
	cpy->ecount = v->ecount;
	cpy->acount = v->ecount;
	cpy->vec_type = v->vec_type;
	memcpy(&cpy->sd[0], &v->sd[0], sizeof cpy->sd[0] * cpy->rank);
	return cpy;
}

static Value add_scalar(Value sum, Value vec, Value scalar)
{
	for (size_t i = 0; i < vec->ecount; ++i) {
		sum->sd[sum->rank + i] = vec->sd[vec->rank + i] + scalar->sd[0];
	}
	return sum;
}

Value value_add(Value a, Value w)
{
	if (a->rank < w->rank) { /* Addition is commutative, so swap. */
		Value t = a;
		a = w;
		w = t;
	} /* Now a->rank >= w->rank */
	Value sum = copy_value_container(w);
	assert(sum); /* TODO: Error handling. */
	if (w->rank == 0) {
		return add_scalar(sum, a, w);
	}
	if (agreed_prefix(a, w) != w->rank) {
		fprintf(stdout, "Error: mismatched shapes.\n");
		exit(EXIT_FAILURE); /* TODO: Error handling */
	}
	/* TODO: Error handling. */
	/* TODO:
	 * Still not correct for prefix agreed case.
	 * (Need to duplicated operation).
	*/
	for (size_t shape = 0, i = 0; shape < agreed_prefix(a, w); ++shape) {
		for (size_t j = 0; j < a->sd[shape]; j++, ++i) {
			sum->sd[sum->rank + i] = a->sd[a->rank + i] + w->sd[w->rank + i];
		}
	}
	return sum;
}

Value value_reference(Value v)
{
	v->refcount++;
	return v;
}
