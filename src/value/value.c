#include "value.h"
struct Value_ {
	size_t refcount;
	enum type type;
	/* Inspired by Roger Hui's An Implementation of J */
	size_t rank;
	size_t ecount;		/* Number of elements used. */
	size_t acount;		/* Number of elements allocated. */
	size_t esize;		/* Size of element type in bytes. */
	union value_data sd[2]; /* Preallocated for the singleton case. */
};

static void print_value(Value v)
{
	fprintf(stderr, "DEBUG:\n");
	fprintf(stderr, "refcount: %zu\n", v->refcount);
	switch (v->type) {
	case VECTOR:
		fprintf(stderr, "type: vector\n");
		break;
	case INTEGER:
		fprintf(stderr, "type: integer\n");
		break;
	default:
		fprintf(stderr, "type: invalid\n");
		break;
	}
	fprintf(stderr, "ecount: %zu\n", v->ecount);
	fprintf(stderr, "acount: %zu\n", v->acount);
	fprintf(stderr, "rank: %zu\n", v->rank);
	for (size_t i = 0; i < v->ecount; ++i) {
		switch (v->type) {
		case INTEGER:
			fprintf(stderr, "%lu\n", v->sd[v->rank + i].integer);
			break;
		case VECTOR:
			print_value(v->sd[v->rank + i].vector);
			break;
		case SYMBOL:
			fprintf(stderr, "%zu\n", v->sd[v->rank + i].symbol);
			break;
		case FUNCTION:
			fprintf(stderr, "rank: %zu\n",
				v->sd[v->rank + i].function.func_rank);
			fprintf(stderr, "arity: %d\n",
				v->sd[v->rank + i].function.arity);
			fprintf(stderr, "hash: %zu\n",
				v->sd[v->rank + i].function.hash);
			break;
		}
	}
	return;
}

static void set_value_atom_data(union value_data *dst, union value_data *src)
{
	memcpy(dst, src, sizeof *dst);
}

Value value_make_single(struct value_atom value)
{
	Value atom =
	    mem_alloc(sizeof *atom); /* Singleton values are presized. */
	assert(atom);		     /* TODO: Error handling */
	atom->refcount = 1;
	atom->rank = 0;
	atom->ecount = 1;
	atom->acount = 1;
	atom->type = value.type;
	set_value_atom_data(&atom->sd[0], &value.data);
	return atom;
}

/* TODO: Put type and size inputs here. */
Value value_make_vector(struct value_atom value)
{
	const size_t init_count = 10;
	Value vec =
	    mem_alloc(sizeof *vec + sizeof vec->sd[0] * (init_count + 1));
	assert(vec); /* TODO: Error handling */
	vec->refcount = 1;
	vec->rank = 1;
	vec->ecount = 1;
	vec->acount = 1;
	vec->esize = sizeof value;
	vec->type = value.type;
	vec->sd[0].shape = 1;
	set_value_atom_data(&vec->sd[1], &value.data);
	return vec;
}

Value value_append(Value v, struct value_atom val)
{
	assert(v->type == val.type);
	if (v->ecount == v->acount) {
		Value n;
		v->acount *= 2;
		n = mem_realloc(v, sizeof *v +
				       sizeof v->sd[0] * (v->acount + v->rank));
		assert(n); /* TODO: Error handling. */
		v = n;
	}
	assert(v);
	v->sd[v->rank - 1].shape++;
	set_value_atom_data(&v->sd[v->rank - 1 + v->ecount++ + 1], &val.data);
	return v;
}

Value value_reference(Value v)
{
	v->refcount++;
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

/* TODO: Fixme */
char *value_stringify(Value v)
{
	const size_t UINT64_DIGITS = 20; /* log(2^64) / log(10) = 19.3 ~ 20. */
	/* I.e. the maximum length of an integer printed in decimal. */
	const size_t len =
	    (UINT64_DIGITS + 1) * (v->ecount + 2) * (v->rank + 1);
	/* ' ' between values, 2 '\n's between dimensions, and '\0' terminator.
	 */
	char *tmp = mem_alloc(len);
	assert(tmp); /* TODO: Error handling */
	size_t pos = 0;
	for (size_t i = 0; i < v->ecount; ++i) {
		pos += snprintf((tmp + pos), len - pos, "%ld ",
				v->sd[v->rank + i].integer);
	}
	tmp[--pos] = '\0'; /* Remove trailing space */
	return tmp;
}

/* Determines whether a and b agree in prefix. Assumes rank a >= rank w */
static int prefixes_agree(Value a, Value w)
{
	for (size_t i = 0; i < w->rank; ++i) {
		if (a->sd[i].shape != w->sd[i].shape) {
			return 0;
		}
	}
	return 1;
}

/* Creates a Value with the same shape, type, and ecount as that given. */
static Value copy_value_container(Value v)
{
	/* NOTE: This is actually oversizing the array for many types. */
	Value cpy =
	    mem_alloc(sizeof *cpy + sizeof v->sd[0] * (v->ecount + v->rank));
	assert(cpy); /* TODO: Error handling */
	cpy->refcount = 1;
	cpy->rank = v->rank;
	cpy->ecount = v->ecount;
	cpy->acount = v->ecount;
	cpy->type = v->type;
	memcpy(&cpy->sd, &v->sd, sizeof v->sd[0] * v->rank);
	return cpy;
}

typedef union value_data (*tmp_funcdef)(union value_data, union value_data);

static void apply_binop(union value_data *res, union value_data *a,
			union value_data *w, tmp_funcdef op, size_t cnt)
{
	for (size_t i = 0; i < cnt; ++i) {
		res[i] = op(a[i], w[i]);
	}
	return;
}

static Value binop(Value a, Value w, tmp_funcdef op)
{
	Value res = copy_value_container(a);
	assert(res); /* TODO: Error handling. */
	if (!prefixes_agree(a, w)) {
		fprintf(stdout, "Error: mismatched shapes.\n");
		exit(EXIT_FAILURE); /* TODO: Error handling. */
	}
	const size_t repitions = a->rank == w->rank ? 1 : a->rank - w->rank;
	for (size_t extrank = 0; extrank < repitions; ++extrank) {
		apply_binop(&res->sd[res->rank + (extrank * w->ecount)],
			    &a->sd[a->rank + (extrank * w->ecount)],
			    &w->sd[w->rank], op, w->ecount);
	}
	return res;
}

static union value_data tmp_add(union value_data a, union value_data w)
{
	union value_data res;
	res.integer = a.integer + w.integer;
	return res;
}

Value value_add(Value a, Value w)
{
	if (a->rank < w->rank) { /* Addition is commutative, so swap. */
		Value t = a;
		a = w;
		w = t;
	} /* Now a->rank >= w->rank */
	assert(w->rank <= a->rank);
	return binop(a, w, &tmp_add);
}
