#include "value.h"
struct Value_ {
	size_t refcount;
	enum value_type type;
	/* Inspired by Roger Hui's An Implementation of J */
	size_t rank;
	size_t ecount;		/* Number of elements used. */
	size_t acount;		/* Number of elements allocated. */
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
	case SYMBOL:
		fprintf(stderr, "type: symbol\n");
		break;
	case FUNCTION:
		fprintf(stderr, "type: function\n");
		break;
	case ERROR:
		fprintf(stderr, "type: error\n");
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
		case ERROR:
			fprintf(stderr, "error: %d\n",
				v->sd[v->rank + i].error);
			break;
		}
	}
	return;
}

static void set_value_atom_data(union value_data *dst, union value_data *src)
{
	memcpy(dst, src, sizeof *dst);
}

static size_t alloc_size(Value v)
{
	return sizeof(*v) + sizeof(v->sd[0]) * v->acount;
}

static size_t used_size(Value v)
{
	if (v->rank == 0) { /* Singletons are presized */
		return sizeof(*v);
	}
	return sizeof(*v) + sizeof(v->sd[0]) * (v->rank + v->ecount);
}

/* TODO: Put type input here. */
Value value_make(Vm vm, struct value_atom value, size_t init_size)
{
	Value v;
	assert(vm);
	if (init_size == 1) {
		v = mem_pool_alloc(&vm->pool); /* Singleton values presized. */
	} else {
		v = mem_slab_alloc(
		    &vm->slab, sizeof(*v) + sizeof(v->sd[0]) * (init_size + 1));
	}
	assert(v); /* TODO: Error handling */
	if (init_size == 1) {
		v->rank = 0;
	} else {
		v->rank = 1;
		v->sd[0].shape = 1;
	}
	v->refcount = v->ecount = 1;
	v->acount = init_size;
	v->type = value.type;
	set_value_atom_data(&v->sd[v->rank], &value.data);
	return v;
}

Value value_append(Vm vm, Value v, struct value_atom val)
{
	assert(vm);
	assert(v);
	assert(v->type == val.type);
	if (v->ecount == v->acount) {
		Value n =
		    mem_slab_realloc(&vm->slab, alloc_size(v) * 2 - sizeof(*v),
				     v, alloc_size(v));
		if (n) {
			v = n;
		} else {
			v->type = ERROR;
			v->rank = 1;
			v->sd[0].shape = 1;
			v->sd[v->rank].error = MEM_ALLOC;
			return v;
		}
	}
	assert(v);
	v->acount *= 2;
	v->sd[v->rank - 1].shape++;
	set_value_atom_data(&v->sd[v->rank - 1 + v->ecount++ + 1], &val.data);
	return v;
}

Value value_reference(Value v)
{
	v->refcount++;
	return v;
}

void value_free(Vm vm, Value v)
{
	assert(v);
	v->refcount--;
	if (v->refcount == 0) {
		/*
		 * We can treat all of these as pool allocations, since our slab
		 * allocations are strictly bigger than our pool allocations.
		 * It's a little microoptimization which lets us ignore whether
		 * or not something is a slab or pool allocation.
		*/
		mem_pool_free(&vm->pool, v);
	}
}

/* TODO: Fixme */
char *value_stringify(Value v)
{
	const size_t DIGITS = 20; /* log(2^64) / log(10) = 19.3 ~ 20. */
	/* I.e. the maximum length of an integer printed in decimal. */
	const size_t len = (DIGITS + 1) * (v->ecount + 2) * (v->rank + 1);
	/* ' ' between values, 2 '\n's between dimensions, & '\0' terminator */
	char *tmp = malloc(len);
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
static Value copy_value_container(Vm vm, Value v)
{
	Value cpy = malloc(used_size(v));
	if (used_size(v) == sizeof(*v)) {
		cpy = mem_pool_alloc(&vm->pool);
	} else {
		cpy = mem_slab_alloc(&vm->slab, used_size(v));
	}
	if (!cpy) {
		return cpy;
	}
	assert(v);
	assert(cpy);
	memcpy(cpy, v, used_size(v));
	cpy->refcount = 1;
	return cpy;
}

typedef union value_data (*value_op)(union value_data, union value_data);

static void apply_binop(union value_data *res, union value_data *a,
			union value_data *w, value_op op, size_t cnt)
{
	for (size_t i = 0; i < cnt; ++i) {
		res[i] = op(a[i], w[i]);
	}
	return;
}

static Value binop(Vm vm, Value a, Value w, value_op op)
{
	Value res = copy_value_container(vm, a);
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

Value value_add(Vm vm, Value a, Value w)
{
	assert(a);
	assert(w);
	assert(vm);
	if (a->rank < w->rank) { /* Addition is commutative, so swap. */
		Value t = a;
		a = w;
		w = t;
	} /* Now a->rank >= w->rank */
	assert(w->rank <= a->rank);
	return binop(vm, a, w, &tmp_add);
}

Vm value_make_vm(void)
{
	Vm vm = malloc(sizeof *vm);
	if (!vm) {
		return vm;
	}
	assert(init_slab_alloc(&vm->slab, 8 * 4096) == 0);
	assert(init_pool_alloc(&vm->pool, &vm->slab, sizeof(struct Value_)) ==
	       0);
	return vm;
}

void value_free_vm(Vm vm)
{
	assert(vm);
	mem_pool_deinit(&vm->pool);
	mem_slab_deinit(&vm->slab);
	free(vm);
}
