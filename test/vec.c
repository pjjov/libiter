/*  libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

/* The assertions ARE the tests, never let NDEBUG compile them out. */
#ifdef NDEBUG
    #undef NDEBUG
#endif

#include <pf_assert.h>
#include <pf_test.h>

#include <iter/global.h>
#include <iter/vec.h>

#include <allocator.h>
#include <allocator_flags.h>
#include <allocator_std.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

#define TEST(name) static int test_##name(int seed, int rep)
#define UNUSED ((void)seed, (void)rep)

typedef vec(int) ivec;
typedef vec(char) cvec;

struct rec {
    int id;
    char tag[3];
    double weight;
};
typedef vec(struct rec) rvec;

struct blob {
    unsigned char bytes[1000];
};
typedef vec(struct blob) bvec;

/* Allocator that tracks every block, can be told to start failing, and
   verifies the `old` size that the library reports on free/realloc. */
typedef struct tracker_t {
    allocator_t base;
    long budget; /* allocations that may still succeed, < 0 = unlimited */
    size_t allocs;
    size_t reallocs;
    size_t frees;
    size_t blocks;  /* live blocks */
    size_t bytes;   /* live bytes */
    size_t badsize; /* frees/reallocs called with a wrong `old` size */
} tracker_t;

#define TRACKER_HDR 16

static void *tracker_fn(
    allocator_t *self, void *ptr, size_t old, size_t size, size_t zalign
) {
    tracker_t *t = (tracker_t *)self;
    unsigned char *raw = NULL;
    size_t stored = 0;

    if (self == ptr)
        return NULL;

    if (ptr) {
        raw = (unsigned char *)ptr - TRACKER_HDR;
        memcpy(&stored, raw, sizeof stored);
        if (stored != old)
            t->badsize++;
    }

    if (size == 0) {
        if (ptr) {
            t->frees++;
            t->blocks--;
            t->bytes -= stored;
            free(raw);
        }
        return NULL;
    }

    if (t->budget == 0)
        return NULL;

    if (ptr) {
        unsigned char *grown = realloc(raw, size + TRACKER_HDR);
        if (!grown)
            return NULL;
        if (t->budget > 0)
            t->budget--;
        if ((zalign & 1) && size > stored)
            memset(grown + TRACKER_HDR + stored, 0, size - stored);
        memcpy(grown, &size, sizeof size);
        t->reallocs++;
        t->bytes = t->bytes - stored + size;
        return grown + TRACKER_HDR;
    }

    raw = (zalign & 1) ? calloc(1, size + TRACKER_HDR)
                       : malloc(size + TRACKER_HDR);
    if (!raw)
        return NULL;
    if (t->budget > 0)
        t->budget--;
    memcpy(raw, &size, sizeof size);
    t->allocs++;
    t->blocks++;
    t->bytes += size;
    return raw + TRACKER_HDR;
}

#define TRACKER(name, budget_)                                       \
    tracker_t name = { { tracker_fn }, (budget_), 0, 0, 0, 0, 0, 0 }

/* xorshift32, good enough for shuffling test inputs. */
static uint32_t rng_next(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}

static uint32_t rng_seed(int seed, int rep, int round) {
    uint32_t s = (uint32_t)seed * 2654435761u + (uint32_t)rep * 40503u
        + (uint32_t)round * 9973u + 1u;
    return s ? s : 1u;
}

/* Compares a vec(int) with an array and dumps both on mismatch. */
static int same_ints(ivec v, const int *want, size_t n) {
    size_t len = veclen(v);
    int ok = len == n;

    for (size_t i = 0; ok && i < n; i++)
        ok = vecget(v, i) == want[i];

    if (!ok) {
        size_t i;
        fprintf(stderr, "vec mismatch: have %zu items [", len);
        for (i = 0; i < len; i++)
            fprintf(stderr, i ? ", %d" : "%d", vecget(v, i));
        fprintf(stderr, "], want %zu items [", n);
        for (i = 0; i < n; i++)
            fprintf(stderr, i ? ", %d" : "%d", want[i]);
        fprintf(stderr, "]\n");
    }

    return ok;
}

#define EXPECT_INTS(v, ...)                                                   \
    do {                                                                      \
        static const int expect_[] = { __VA_ARGS__ };                         \
        pf_assert(same_ints((v), expect_, sizeof expect_ / sizeof *expect_)); \
    } while (0)

#define EXPECT_EMPTY(v) pf_assert(veclen(v) == 0)

/* Builds a vec(int) holding the given values. */
static ivec ints_with(allocator_t *alloc, size_t n, const int *vals) {
    ivec v = vecnew(int, alloc);
    pf_assert_not_null(v);
    if (n)
        pf_assert_ok(vecpush(v, (int *)vals, n));
    return v;
}

#define INTS(...)                                      \
    ints_with(                                         \
        NULL,                                          \
        sizeof((int[]) { __VA_ARGS__ }) / sizeof(int), \
        (int[]) { __VA_ARGS__ }                        \
    )

#define INTS_ON(alloc, ...)                            \
    ints_with(                                         \
        (alloc),                                       \
        sizeof((int[]) { __VA_ARGS__ }) / sizeof(int), \
        (int[]) { __VA_ARGS__ }                        \
    )

/* Statement helpers that push/insert literal values and require success. */
#define PUSH(v, ...)                                                  \
    do {                                                              \
        int a_[] = { __VA_ARGS__ };                                   \
        pf_assert_ok(vecpush((v), &a_[0], sizeof a_ / sizeof a_[0])); \
    } while (0)

#define INSERT(v, i, ...)                                                    \
    do {                                                                     \
        int a_[] = { __VA_ARGS__ };                                          \
        pf_assert_ok(vecinsert((v), &a_[0], (i), sizeof a_ / sizeof a_[0])); \
    } while (0)

/* Fills v with 0, 1, ..., n-1. */
static void fill_seq(ivec v, int n) {
    for (int i = 0; i < n; i++)
        pf_assert_ok(vecpush(v, &i, 1));
}

static struct rec mkrec(int id) {
    struct rec r;
    memset(&r, 0, sizeof r);
    r.id = id;
    r.tag[0] = (char)('a' + id % 26);
    r.tag[1] = (char)('A' + id % 26);
    r.weight = id * 0.5;
    return r;
}

static int rec_is(const struct rec *r, int id) {
    struct rec want = mkrec(id);
    return r->id == want.id && r->tag[0] == want.tag[0]
        && r->tag[1] == want.tag[1] && r->weight == want.weight;
}

/* ------------------------------------------------------------------ */
/* Creation and destruction                                           */
/* ------------------------------------------------------------------ */

TEST(new_is_empty) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    pf_assert_not_null(v);
    pf_assert(veclen(v) == 0);
    pf_assert(veccap(v) == 0);
    pf_assert(vecisempty(v));
    pf_assert_null(vecitems(v));
    pf_assert_null(vecstart(v));
    pf_assert_null(vecend(v));
    vecfree(v);
    return 0;
}

TEST(new_uses_global_allocator) {
    UNUSED;
    TRACKER(t, -1);
    int x = 7;

    libiter_use_allocator(&t.base);
    ivec v = vecnew(int, NULL);
    libiter_use_allocator(&standard_allocator);

    pf_assert_not_null(v);
    pf_assert(vecallocator(v) == &t.base);
    pf_assert(t.blocks == 1);
    pf_assert_ok(vecpush(v, &x, 1));
    pf_assert(t.blocks == 2);
    vecfree(v);
    pf_assert(t.blocks == 0);
    pf_assert(t.badsize == 0);
    return 0;
}

TEST(new_uses_given_allocator) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);
    pf_assert_not_null(v);
    pf_assert(vecallocator(v) == &t.base);
    pf_assert(t.blocks == 1);
    vecfree(v);
    pf_assert(t.blocks == 0);
    pf_assert(t.frees == 1);
    return 0;
}

TEST(new_allocation_failure) {
    UNUSED;
    TRACKER(t, 0);
    ivec v = vecnew(int, &t.base);
    pf_assert_null(v);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(init_uses_caller_storage) {
    UNUSED;
    TRACKER(t, -1);
    vec_t raw;
    memset(&raw, 0xAB, sizeof raw);

    ivec v = vecinit(int, &raw, &t.base);
    pf_assert(vec_base(v) == &raw);
    pf_assert(t.blocks == 0);
    pf_assert(veclen(v) == 0);
    pf_assert(veccap(v) == 0);
    pf_assert_null(vecitems(v));
    pf_assert(vecallocator(v) == &t.base);

    fill_seq(v, 10);
    pf_assert(t.blocks == 1);

    /* Only the item buffer may be released, `raw` lives on the stack. */
    vecfree(v);
    pf_assert(t.blocks == 0);
    pf_assert(t.badsize == 0);
    return 0;
}

TEST(init_invalid_arguments) {
    UNUSED;
    TRACKER(t, -1);
    vec_t raw;
    pf_assert_null(vecinit(int, NULL, NULL));
    pf_assert_null(vecinit(int, NULL, &t.base));

    /* Allocators carrying flag bits cannot be used to allocate. */
    allocator_t *tagged = allocator_flags_set(&t.base, 1);
    pf_assert_null(vecinit(int, &raw, tagged));
    return 0;
}

TEST(new_array_copies_source) {
    UNUSED;
    int src[] = { 4, 5, 6 };
    ivec v = vecnew_array(src, 3, NULL);
    pf_assert_not_null(v);
    EXPECT_INTS(v, 4, 5, 6);

    src[0] = 99;
    EXPECT_INTS(v, 4, 5, 6);
    pf_assert(vecitems(v) != (void *)src);
    vecfree(v);
    return 0;
}

TEST(new_array_empty) {
    UNUSED;
    int src[] = { 1 };
    ivec v = vecnew_array(src, 0, NULL);
    pf_assert_not_null(v);
    pf_assert(vecisempty(v));
    vecfree(v);
    return 0;
}

TEST(new_array_null_source) {
    UNUSED;
    TRACKER(t, -1);
    int *none = NULL;
    ivec v = vecnew_array(none, 3, &t.base);
    pf_assert_null(v);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(new_array_allocation_failure_leaks_nothing) {
    UNUSED;
    int src[] = { 1, 2, 3 };

    /* The vec itself can be allocated, the item buffer cannot. */
    TRACKER(t, 1);
    ivec v = vecnew_array(src, 3, &t.base);
    pf_assert_null(v);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(new_arrayp_copies_pointees) {
    UNUSED;
    int a = 1, b = 2, c = 3;
    int *ptrs[] = { &a, &b, &c };

    ivec v = (ivec)vec_new_arrayp((void **)ptrs, 3, NULL, sizeof(int));
    pf_assert_not_null(v);
    EXPECT_INTS(v, 1, 2, 3);

    a = 100;
    EXPECT_INTS(v, 1, 2, 3);
    vecfree(v);
    return 0;
}

TEST(new_arrayp_failure) {
    UNUSED;
    TRACKER(t, 1);
    int a = 1;
    int *ptrs[] = { &a };
    pf_assert_null(vec_new_arrayp((void **)ptrs, 1, &t.base, sizeof(int)));
    pf_assert(t.blocks == 0);
    pf_assert_null(vec_new_arrayp(NULL, 1, NULL, sizeof(int)));
    return 0;
}

TEST(init_array_copies_source) {
    UNUSED;
    TRACKER(t, -1);
    vec_t raw;
    int src[] = { 9, 8, 7, 6 };

    ivec v = vecinit_array(src, 4, &raw, &t.base);
    pf_assert(vec_base(v) == &raw);
    EXPECT_INTS(v, 9, 8, 7, 6);
    vecfree(v);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(init_array_allocation_failure) {
    UNUSED;
    TRACKER(t, 0);
    vec_t raw;
    int src[] = { 1, 2 };
    pf_assert_null(vecinit_array(src, 2, &raw, &t.base));
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(init_arrayp_copies_pointees) {
    UNUSED;
    TRACKER(t, -1);
    vec_t raw;
    int a = 5, b = 6;
    int *ptrs[] = { &a, &b };

    ivec v = (ivec)vec_init_arrayp(
        (void **)ptrs, 2, &raw, &t.base, sizeof(int)
    );
    pf_assert(vec_base(v) == &raw);
    EXPECT_INTS(v, 5, 6);
    vecfree(v);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(free_null_is_noop) {
    UNUSED;
    ivec v = NULL;
    vecfree(v);
    return 0;
}

TEST(free_releases_everything) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);
    fill_seq(v, 500);
    pf_assert(t.blocks == 2);
    vecfree(v);
    pf_assert(t.blocks == 0);
    pf_assert(t.bytes == 0);
    pf_assert(t.badsize == 0);
    return 0;
}

TEST(unwrap_heap_vec_returns_buffer) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);
    fill_seq(v, 5);
    size_t bytes = veccap(v) * sizeof(int);
    void *items = vecitems(v);

    int *buf = vecunwrap(v);
    pf_assert(buf == items);
    pf_assert(t.blocks == 1); /* the vec is gone, the buffer stays */
    for (int i = 0; i < 5; i++)
        pf_assert(buf[i] == i);

    deallocate(&t.base, buf, bytes);
    pf_assert(t.blocks == 0);
    pf_assert(t.badsize == 0);
    return 0;
}

TEST(unwrap_initialized_vec_returns_buffer) {
    UNUSED;
    TRACKER(t, -1);
    vec_t raw;
    ivec v = vecinit(int, &raw, &t.base);
    fill_seq(v, 3);
    size_t bytes = veccap(v) * sizeof(int);

    int *buf = vecunwrap(v);
    pf_assert_not_null(buf);
    pf_assert(t.blocks == 1);
    pf_assert(buf[2] == 2);
    deallocate(&t.base, buf, bytes);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(unwrap_empty_and_null) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);
    pf_assert_null(vecunwrap(v));
    pf_assert(t.blocks == 0);

    ivec none = NULL;
    pf_assert_null(vecunwrap(none));
    return 0;
}

/* ------------------------------------------------------------------ */
/* Properties                                                         */
/* ------------------------------------------------------------------ */

TEST(null_vec_is_safe_to_query) {
    UNUSED;
    ivec v = NULL;
    pf_assert(veclen(v) == 0);
    pf_assert(veccap(v) == 0);
    pf_assert(vecisempty(v));
    pf_assert_null(vecitems(v));
    pf_assert_null(vecstart(v));
    pf_assert_null(vecend(v));
    pf_assert_null(vecptr(v, 0));
    pf_assert_null(vecslot(v, 0));
    pf_assert_null(vecallocator(v));
    vecclear(v);
    vecsetlen(v, 3);
    vecsetcap(v, 3);
    return 0;
}

TEST(null_vec_operations_fail) {
    UNUSED;
    ivec v = NULL;
    int x = 1;
    int out = 0;
    pf_assert(vecresize(v, 4) == ITER_EINVAL);
    pf_assert(vecreserve(v, 4) == ITER_EINVAL);
    pf_assert(vecpush(v, &x, 1) == ITER_EINVAL);
    pf_assert(vecunshift(v, &x, 1) == ITER_EINVAL);
    pf_assert(vecinsert(v, &x, 0, 1) == ITER_EINVAL);
    pf_assert(vecfill(v, &x, 0, 1) == ITER_EINVAL);
    pf_assert(vecremove(v, &out, 0, 1) == ITER_EINVAL);
    pf_assert(vecpop(v, &out, 1) == ITER_EINVAL);
    pf_assert(vecshift(v, &out, 1) == ITER_EINVAL);
    pf_assert(vecswap(v, 0, 1, 1) == ITER_EINVAL);
    pf_assert(vecswap_remove(v, &out, 0, 1) == ITER_EINVAL);
    pf_assert(vec_insertp(NULL, NULL, 0, 1, sizeof(int)) == ITER_EINVAL);
    return 0;
}

TEST(length_tracks_operations) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    fill_seq(v, 10);
    pf_assert(veclen(v) == 10);
    pf_assert(!vecisempty(v));
    pf_assert(veccap(v) >= 10);
    pf_assert_ok(vecpop(v, NULL, 4));
    pf_assert(veclen(v) == 6);
    vecclear(v);
    pf_assert(veclen(v) == 0);
    pf_assert(vecisempty(v));
    vecfree(v);
    return 0;
}

TEST(ptr_and_slot_bounds) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    pf_assert_ok(vecreserve(v, 8));
    PUSH(v, 10, 20, 30);

    /* ptr is limited by length... */
    for (size_t i = 0; i < 3; i++)
        pf_assert(vecptr(v, i) == (vecitems(v) + i));
    pf_assert_null(vecptr(v, 3));
    pf_assert_null(vecptr(v, SIZE_MAX));
    pf_assert_null(vecptr(v, SIZE_MAX / 2));

    /* ...while slot is limited by capacity. */
    pf_assert(vecslot(v, 0) == vecitems(v));
    pf_assert_not_null(vecslot(v, 3));
    pf_assert_not_null(vecslot(v, veccap(v) - 1));
    pf_assert_null(vecslot(v, veccap(v)));
    pf_assert_null(vecslot(v, SIZE_MAX));
    vecfree(v);
    return 0;
}

TEST(start_and_end_delimit_items) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4);
    pf_assert(vecstart(v) == vecitems(v));
    pf_assert(
        (char *)vecend(v) - (char *)vecstart(v) == (long)(4 * sizeof(int))
    );
    vecclear(v);
    pf_assert_null(vecstart(v));
    pf_assert_null(vecend(v));
    vecfree(v);
    return 0;
}

TEST(foreach_visits_in_order) {
    UNUSED;
    ivec v = INTS(5, 6, 7, 8);
    int expect = 5;
    size_t visited = 0;

    VECFOREACH(v, it) {
        pf_assert(*it == expect++);
        visited++;
    }
    pf_assert(visited == 4);
    vecfree(v);
    return 0;
}

TEST(foreach_can_modify_items) {
    UNUSED;
    ivec v = INTS(1, 2, 3);
    VECFOREACH(v, it) { *it *= 10; }
    EXPECT_INTS(v, 10, 20, 30);
    vecfree(v);
    return 0;
}

TEST(foreach_on_empty_does_nothing) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    size_t visited = 0;
    VECFOREACH(v, it) {
        (void)it;
        visited++;
    }
    pf_assert(visited == 0);
    vecfree(v);
    return 0;
}

TEST(type_helpers) {
    UNUSED;
    ivec iv = NULL;
    cvec cv = NULL;
    rvec rv = NULL;
    bvec bv = NULL;
    pf_assert(vec_type_size(iv) == sizeof(int));
    pf_assert(vec_type_size(cv) == sizeof(char));
    pf_assert(vec_type_size(rv) == sizeof(struct rec));
    pf_assert(vec_type_size(bv) == sizeof(struct blob));
    return 0;
}

/* ------------------------------------------------------------------ */
/* Capacity                                                           */
/* ------------------------------------------------------------------ */

TEST(resize_grows_and_keeps_items) {
    UNUSED;
    ivec v = INTS(1, 2, 3);
    pf_assert_ok(vecresize(v, 100));
    pf_assert(veccap(v) == 100);
    EXPECT_INTS(v, 1, 2, 3);
    vecfree(v);
    return 0;
}

TEST(resize_shrink_truncates_length) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    fill_seq(v, 10);
    pf_assert_ok(vecresize(v, 4));
    pf_assert(veccap(v) == 4);
    EXPECT_INTS(v, 0, 1, 2, 3);
    vecfree(v);
    return 0;
}

TEST(resize_shrink_above_length_keeps_length) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    pf_assert_ok(vecresize(v, 50));
    PUSH(v, 1, 2, 3);
    pf_assert_ok(vecresize(v, 5));
    pf_assert(veccap(v) == 5);
    EXPECT_INTS(v, 1, 2, 3);
    vecfree(v);
    return 0;
}

TEST(resize_to_zero_releases_buffer) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);
    fill_seq(v, 8);
    pf_assert(t.blocks == 2);

    pf_assert_ok(vecresize(v, 0));
    pf_assert(veccap(v) == 0);
    pf_assert(veclen(v) == 0);
    pf_assert_null(vecitems(v));
    pf_assert(t.blocks == 1);

    /* the vec must remain usable */
    PUSH(v, 42);
    EXPECT_INTS(v, 42);
    vecfree(v);
    pf_assert(t.blocks == 0);
    pf_assert(t.badsize == 0);
    return 0;
}

TEST(resize_empty_vec_to_zero_is_fine) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    pf_assert_ok(vecresize(v, 0));
    pf_assert(veccap(v) == 0);
    vecfree(v);
    return 0;
}

TEST(resize_overflow) {
    UNUSED;
    ivec v = INTS(1, 2, 3);
    size_t cap = veccap(v);
    pf_assert(vecresize(v, SIZE_MAX) == ITER_EOVERFLOW);
    pf_assert(vecresize(v, SIZE_MAX / sizeof(int) + 1) == ITER_EOVERFLOW);
    pf_assert(veccap(v) == cap);
    EXPECT_INTS(v, 1, 2, 3);
    vecfree(v);
    return 0;
}

TEST(resize_out_of_memory_changes_nothing) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = INTS_ON(&t.base, 1, 2, 3, 4, 5);
    size_t cap = veccap(v);
    t.budget = 0;

    pf_assert(vecresize(v, 1000) == ITER_ENOMEM);
    pf_assert(veccap(v) == cap);
    EXPECT_INTS(v, 1, 2, 3, 4, 5);

    t.budget = -1;
    vecfree(v);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(reserve_on_empty) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    pf_assert_ok(vecreserve(v, 10));
    pf_assert(veccap(v) >= 10);
    pf_assert(veclen(v) == 0);
    pf_assert_not_null(vecitems(v));
    vecfree(v);
    return 0;
}

TEST(reserve_is_relative_to_length) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4);
    pf_assert_ok(vecreserve(v, 10));
    pf_assert(veccap(v) >= 14);
    EXPECT_INTS(v, 1, 2, 3, 4);
    vecfree(v);
    return 0;
}

TEST(reserve_is_noop_when_space_exists) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);
    pf_assert_ok(vecreserve(v, 32));
    size_t cap = veccap(v);
    size_t allocs = t.allocs, reallocs = t.reallocs;

    pf_assert_ok(vecreserve(v, 32));
    pf_assert_ok(vecreserve(v, 1));
    pf_assert_ok(vecreserve(v, 0));
    pf_assert(veccap(v) == cap);
    pf_assert(t.allocs == allocs);
    pf_assert(t.reallocs == reallocs);
    vecfree(v);
    return 0;
}

TEST(reserve_overflow) {
    UNUSED;
    ivec v = INTS(1);
    pf_assert(vecreserve(v, SIZE_MAX) == ITER_EOVERFLOW);
    pf_assert(vecreserve(v, SIZE_MAX / sizeof(int) + 1) == ITER_EOVERFLOW);
    EXPECT_INTS(v, 1);
    vecfree(v);
    return 0;
}

TEST(reserve_out_of_memory_changes_nothing) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = INTS_ON(&t.base, 1, 2, 3);
    size_t cap = veccap(v);
    t.budget = 0;
    pf_assert(vecreserve(v, cap + 100) == ITER_ENOMEM);
    pf_assert(veccap(v) == cap);
    EXPECT_INTS(v, 1, 2, 3);
    t.budget = -1;
    vecfree(v);
    return 0;
}

TEST(clear_keeps_capacity_and_buffer) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    fill_seq(v, 20);
    size_t cap = veccap(v);
    void *items = vecitems(v);

    vecclear(v);
    pf_assert(veclen(v) == 0);
    pf_assert(vecisempty(v));
    pf_assert(veccap(v) == cap);
    pf_assert(vecitems(v) == items);

    PUSH(v, 1, 2);
    EXPECT_INTS(v, 1, 2);
    vecfree(v);
    return 0;
}

TEST(setlen_is_clamped_to_capacity) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    pf_assert_ok(vecresize(v, 10));
    PUSH(v, 1, 2, 3);

    vecsetlen(v, 6);
    pf_assert(veclen(v) == 6);
    pf_assert(vecget(v, 0) == 1 && vecget(v, 2) == 3);

    vecsetlen(v, 1000);
    pf_assert(veclen(v) == veccap(v));

    vecsetlen(v, 2);
    pf_assert(veclen(v) == 2);

    vecsetlen(v, 0);
    pf_assert(vecisempty(v));

    vecsetlen(v, 3);
    vecsetlen(v, SIZE_MAX); /* overflow: ignored */
    pf_assert(veclen(v) == 3);
    vecfree(v);
    return 0;
}

TEST(setcap_truncates_length) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    fill_seq(v, 8);
    size_t cap = veccap(v);

    vecsetcap(v, 4);
    pf_assert(veccap(v) == 4);
    EXPECT_INTS(v, 0, 1, 2, 3);

    vecsetcap(v, SIZE_MAX); /* overflow: ignored */
    pf_assert(veccap(v) == 4);

    /* setcap only edits the bookkeeping, hand the real size back so that
       the buffer is released with the size it was allocated with */
    vecsetcap(v, cap);
    vecfree(v);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Push / unshift / insert / fill                                     */
/* ------------------------------------------------------------------ */

TEST(push_appends) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    PUSH(v, 1);
    EXPECT_INTS(v, 1);
    PUSH(v, 2, 3, 4);
    EXPECT_INTS(v, 1, 2, 3, 4);
    vecfree(v);
    return 0;
}

TEST(push_zero_items_is_noop) {
    UNUSED;
    ivec v = INTS(1, 2);
    int x = 9;
    pf_assert_ok(vecpush(v, &x, 0));
    EXPECT_INTS(v, 1, 2);

    ivec e = vecnew(int, NULL);
    pf_assert_ok(vecpush(e, &x, 0));
    pf_assert(vecisempty(e));
    vecfree(e);
    vecfree(v);
    return 0;
}

TEST(push_null_items) {
    UNUSED;
    ivec v = INTS(1);
    int *none = NULL;
    pf_assert(vecpush(v, none, 1) == ITER_EINVAL);
    pf_assert(vecinsert(v, none, 0, 1) == ITER_EINVAL);
    pf_assert(vecunshift(v, none, 1) == ITER_EINVAL);
    EXPECT_INTS(v, 1);
    vecfree(v);
    return 0;
}

TEST(push_count_overflow) {
    UNUSED;
    ivec v = INTS(1);
    int x = 0;
    pf_assert(vecpush(v, &x, SIZE_MAX) == ITER_EOVERFLOW);
    pf_assert(vecpush(v, &x, SIZE_MAX / sizeof(int) + 1) == ITER_EOVERFLOW);
    EXPECT_INTS(v, 1);
    vecfree(v);
    return 0;
}

TEST(push_grows_geometrically) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);

    for (int i = 0; i < 4000; i++)
        pf_assert_ok(vecpush(v, &i, 1));

    pf_assert(veclen(v) == 4000);
    pf_assert(veccap(v) >= 4000);
    for (int i = 0; i < 4000; i++)
        pf_assert(vecget(v, (size_t)i) == i);

    /* amortized growth: nowhere near one allocation per push */
    pf_assert(t.allocs + t.reallocs < 100);
    vecfree(v);
    pf_assert(t.blocks == 0);
    pf_assert(t.badsize == 0);
    return 0;
}

TEST(push_large_batch) {
    UNUSED;
    enum { N = 100000 };
    int *src = malloc(N * sizeof *src);
    pf_assert_not_null(src);
    for (int i = 0; i < N; i++)
        src[i] = i * 3;

    ivec v = vecnew(int, NULL);
    pf_assert_ok(vecpush(v, src, N));
    pf_assert(veclen(v) == N);
    pf_assert_memcmp(vecitems(v), src, N * sizeof *src);
    vecfree(v);
    free(src);
    return 0;
}

TEST(push_within_capacity_does_not_allocate) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);
    pf_assert_ok(vecresize(v, 8));
    void *items = vecitems(v);
    t.budget = 0;

    for (int i = 0; i < 8; i++)
        pf_assert_ok(vecpush(v, &i, 1));

    pf_assert(vecitems(v) == items);
    pf_assert(veclen(v) == 8);
    t.budget = -1;
    vecfree(v);
    return 0;
}

TEST(push_out_of_memory_changes_nothing) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);
    pf_assert_ok(vecresize(v, 3));
    PUSH(v, 1, 2, 3);
    t.budget = 0;

    int x = 4;
    pf_assert(vecpush(v, &x, 1) == ITER_ENOMEM);
    pf_assert(vecinsert(v, &x, 1, 1) == ITER_ENOMEM);
    pf_assert(vecunshift(v, &x, 1) == ITER_ENOMEM);
    pf_assert(vecfill(v, &x, 0, 1) == ITER_ENOMEM);
    EXPECT_INTS(v, 1, 2, 3);

    t.budget = -1;
    vecfree(v);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(unshift_prepends) {
    UNUSED;
    ivec v = INTS(4, 5);
    int a[] = { 3 };
    pf_assert_ok(vecunshift(v, &a[0], 1));
    EXPECT_INTS(v, 3, 4, 5);

    int b[] = { 1, 2 };
    pf_assert_ok(vecunshift(v, &b[0], 2));
    EXPECT_INTS(v, 1, 2, 3, 4, 5);
    vecfree(v);
    return 0;
}

TEST(insert_into_empty) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    INSERT(v, 0, 1, 2, 3);
    EXPECT_INTS(v, 1, 2, 3);
    vecfree(v);
    return 0;
}

TEST(insert_single_at_front_middle_and_end) {
    UNUSED;
    ivec v = INTS(2, 4);
    INSERT(v, 0, 1);
    EXPECT_INTS(v, 1, 2, 4);
    INSERT(v, 2, 3);
    EXPECT_INTS(v, 1, 2, 3, 4);
    INSERT(v, 4, 5);
    EXPECT_INTS(v, 1, 2, 3, 4, 5);
    vecfree(v);
    return 0;
}

TEST(insert_multiple_in_the_middle) {
    UNUSED;
    ivec v = INTS(1, 2, 5, 6);
    INSERT(v, 2, 3, 4);
    EXPECT_INTS(v, 1, 2, 3, 4, 5, 6);
    vecfree(v);
    return 0;
}

TEST(insert_multiple_at_the_front) {
    UNUSED;
    ivec v = INTS(4, 5, 6);
    INSERT(v, 0, 1, 2, 3);
    EXPECT_INTS(v, 1, 2, 3, 4, 5, 6);
    vecfree(v);
    return 0;
}

TEST(insert_multiple_in_the_middle_with_reallocation) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    pf_assert_ok(vecresize(v, 4));
    PUSH(v, 1, 2, 7, 8);
    pf_assert(veclen(v) == veccap(v)); /* full, insert must grow */
    INSERT(v, 2, 3, 4, 5, 6);
    EXPECT_INTS(v, 1, 2, 3, 4, 5, 6, 7, 8);
    vecfree(v);
    return 0;
}

TEST(insert_past_the_end_is_rejected) {
    UNUSED;
    ivec v = INTS(1, 2, 3);
    int x = 9;
    pf_assert(vecinsert(v, &x, 4, 1) == ITER_EINVAL);
    pf_assert(vecinsert(v, &x, 100, 1) == ITER_EINVAL);
    EXPECT_INTS(v, 1, 2, 3);

    ivec e = vecnew(int, NULL);
    pf_assert(vecinsert(e, &x, 1, 1) == ITER_EINVAL);
    pf_assert(vecisempty(e));
    vecfree(e);
    vecfree(v);
    return 0;
}

TEST(insert_index_overflow) {
    UNUSED;
    ivec v = INTS(1);
    int x = 0;
    pf_assert(vecinsert(v, &x, SIZE_MAX, 1) == ITER_EOVERFLOW);
    pf_assert(vecinsert(v, &x, 0, SIZE_MAX) == ITER_EOVERFLOW);
    EXPECT_INTS(v, 1);
    vecfree(v);
    return 0;
}

TEST(fill_at_the_end) {
    UNUSED;
    ivec v = INTS(1, 2);
    int x = 7;
    pf_assert_ok(vecfill(v, &x, 2, 3));
    EXPECT_INTS(v, 1, 2, 7, 7, 7);
    vecfree(v);
    return 0;
}

TEST(fill_at_the_front) {
    UNUSED;
    ivec v = INTS(1, 2);
    int x = 7;
    pf_assert_ok(vecfill(v, &x, 0, 3));
    EXPECT_INTS(v, 7, 7, 7, 1, 2);
    vecfree(v);
    return 0;
}

TEST(fill_single_in_the_middle) {
    UNUSED;
    ivec v = INTS(1, 3);
    int x = 2;
    pf_assert_ok(vecfill(v, &x, 1, 1));
    EXPECT_INTS(v, 1, 2, 3);
    vecfree(v);
    return 0;
}

TEST(fill_multiple_in_the_middle) {
    UNUSED;
    ivec v = INTS(1, 2, 9, 10);
    int x = 5;
    pf_assert_ok(vecfill(v, &x, 2, 3));
    EXPECT_INTS(v, 1, 2, 5, 5, 5, 9, 10);
    vecfree(v);
    return 0;
}

TEST(fill_zero_count_and_errors) {
    UNUSED;
    ivec v = INTS(1, 2);
    int x = 7;
    int *none = NULL;
    pf_assert_ok(vecfill(v, &x, 1, 0));
    EXPECT_INTS(v, 1, 2);
    pf_assert(vecfill(v, none, 0, 1) == ITER_EINVAL);
    pf_assert(vecfill(v, &x, 3, 1) == ITER_EINVAL);
    pf_assert(vecfill(v, &x, SIZE_MAX, 1) == ITER_EOVERFLOW);
    pf_assert(vecfill(v, &x, 0, SIZE_MAX) == ITER_EOVERFLOW);
    EXPECT_INTS(v, 1, 2);
    vecfree(v);
    return 0;
}

TEST(insertp_copies_pointees_in_order) {
    UNUSED;
    ivec v = INTS(1, 5);
    int a = 2, b = 3, c = 4;
    int *ptrs[] = { &a, &b, &c };

    pf_assert_ok(vec_insertp(vec_base(v), (void **)ptrs, 1, 3, sizeof(int)));
    EXPECT_INTS(v, 1, 2, 3, 4, 5);

    a = 100;
    EXPECT_INTS(v, 1, 2, 3, 4, 5);
    vecfree(v);
    return 0;
}

TEST(insertp_at_the_end_and_front) {
    UNUSED;
    ivec v = INTS(3);
    int a = 1, b = 2, c = 4;
    int *front[] = { &a, &b };
    int *back[] = { &c };

    pf_assert_ok(vec_insertp(vec_base(v), (void **)front, 0, 2, sizeof(int)));
    pf_assert_ok(vec_insertp(vec_base(v), (void **)back, 3, 1, sizeof(int)));
    EXPECT_INTS(v, 1, 2, 3, 4);
    vecfree(v);
    return 0;
}

TEST(insertp_errors) {
    UNUSED;
    ivec v = INTS(1);
    int a = 2;
    int *ptrs[] = { &a };
    pf_assert(vec_insertp(vec_base(v), NULL, 0, 1, sizeof(int)) == ITER_EINVAL);
    pf_assert(
        vec_insertp(vec_base(v), (void **)ptrs, 2, 1, sizeof(int))
        == ITER_EINVAL
    );
    pf_assert(vec_insertp(vec_base(v), (void **)ptrs, 0, 1, 0) == ITER_EINVAL);
    EXPECT_INTS(v, 1);
    vecfree(v);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Remove / pop / shift                                               */
/* ------------------------------------------------------------------ */

TEST(remove_copies_removed_items_out) {
    UNUSED;
    ivec v = INTS(10, 11, 12, 13, 14, 15);
    int out[2] = { 0, 0 };
    pf_assert_ok(vecremove(v, &out[0], 2, 2));
    pf_assert(out[0] == 12 && out[1] == 13);
    EXPECT_INTS(v, 10, 11, 14, 15);
    vecfree(v);
    return 0;
}

TEST(remove_without_out) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4);
    pf_assert_ok(vecremove(v, NULL, 1, 2));
    EXPECT_INTS(v, 1, 4);
    vecfree(v);
    return 0;
}

TEST(remove_first_and_last) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4);
    int out = 0;
    pf_assert_ok(vecremove(v, &out, 0, 1));
    pf_assert(out == 1);
    pf_assert_ok(vecremove(v, &out, 2, 1));
    pf_assert(out == 4);
    EXPECT_INTS(v, 2, 3);
    vecfree(v);
    return 0;
}

TEST(remove_everything) {
    UNUSED;
    ivec v = INTS(1, 2, 3);
    size_t cap = veccap(v);
    pf_assert_ok(vecremove(v, NULL, 0, 3));
    EXPECT_EMPTY(v);
    pf_assert(veccap(v) == cap);
    vecfree(v);
    return 0;
}

TEST(remove_zero_items) {
    UNUSED;
    ivec v = INTS(1, 2);
    pf_assert_ok(vecremove(v, NULL, 0, 0));
    pf_assert_ok(vecremove(v, NULL, 2, 0));
    EXPECT_INTS(v, 1, 2);
    vecfree(v);
    return 0;
}

TEST(remove_out_of_range) {
    UNUSED;
    ivec v = INTS(1, 2, 3);
    int out[4] = { -1, -1, -1, -1 };

    pf_assert(vecremove(v, &out[0], 3, 1) == ITER_EINVAL);
    pf_assert(vecremove(v, &out[0], 2, 2) == ITER_EINVAL);
    pf_assert(vecremove(v, &out[0], 0, 4) == ITER_EINVAL);
    pf_assert(vecremove(v, &out[0], 100, 1) == ITER_EINVAL);

    /* nothing may be copied or removed on failure */
    pf_assert(out[0] == -1 && out[1] == -1 && out[2] == -1 && out[3] == -1);
    EXPECT_INTS(v, 1, 2, 3);

    ivec e = vecnew(int, NULL);
    pf_assert(vecremove(e, NULL, 0, 1) == ITER_EINVAL);
    vecfree(e);
    vecfree(v);
    return 0;
}

TEST(remove_overflow) {
    UNUSED;
    ivec v = INTS(1, 2, 3);
    pf_assert(vecremove(v, NULL, SIZE_MAX, 1) == ITER_EOVERFLOW);
    pf_assert(vecremove(v, NULL, 0, SIZE_MAX) == ITER_EOVERFLOW);
    EXPECT_INTS(v, 1, 2, 3);
    vecfree(v);
    return 0;
}

TEST(pop_removes_from_the_end) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4, 5);
    int out = 0;
    pf_assert_ok(vecpop(v, &out, 1));
    pf_assert(out == 5);
    EXPECT_INTS(v, 1, 2, 3, 4);
    vecfree(v);
    return 0;
}

TEST(pop_multiple_keeps_original_order) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4, 5);
    int out[3] = { 0, 0, 0 };
    pf_assert_ok(vecpop(v, &out[0], 3));
    pf_assert(out[0] == 3 && out[1] == 4 && out[2] == 5);
    EXPECT_INTS(v, 1, 2);
    vecfree(v);
    return 0;
}

TEST(pop_without_out_and_zero) {
    UNUSED;
    ivec v = INTS(1, 2, 3);
    pf_assert_ok(vecpop(v, NULL, 0));
    EXPECT_INTS(v, 1, 2, 3);
    pf_assert_ok(vecpop(v, NULL, 2));
    EXPECT_INTS(v, 1);
    pf_assert_ok(vecpop(v, NULL, 1));
    EXPECT_EMPTY(v);
    vecfree(v);
    return 0;
}

TEST(pop_more_than_length) {
    UNUSED;
    ivec v = INTS(1, 2);
    int out[3] = { -1, -1, -1 };
    pf_assert(vecpop(v, &out[0], 3) == ITER_EINVAL);
    pf_assert(out[0] == -1);
    EXPECT_INTS(v, 1, 2);

    ivec e = vecnew(int, NULL);
    pf_assert(vecpop(e, NULL, 1) == ITER_EINVAL);
    vecfree(e);
    vecfree(v);
    return 0;
}

TEST(shift_removes_from_the_front) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4, 5);
    int out[2] = { 0, 0 };
    pf_assert_ok(vecshift(v, &out[0], 2));
    pf_assert(out[0] == 1 && out[1] == 2);
    EXPECT_INTS(v, 3, 4, 5);
    pf_assert_ok(vecshift(v, NULL, 1));
    EXPECT_INTS(v, 4, 5);
    vecfree(v);
    return 0;
}

TEST(shift_more_than_length) {
    UNUSED;
    ivec v = INTS(1, 2);
    pf_assert(vecshift(v, NULL, 3) == ITER_EINVAL);
    EXPECT_INTS(v, 1, 2);
    vecfree(v);
    return 0;
}

TEST(queue_behaviour) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    int next_in = 0, next_out = 0;

    for (int round = 0; round < 500; round++) {
        for (int k = 0; k < 3; k++, next_in++)
            pf_assert_ok(vecpush(v, &next_in, 1));
        for (int k = 0; k < 2; k++, next_out++) {
            int out = -1;
            pf_assert_ok(vecshift(v, &out, 1));
            pf_assert(out == next_out);
        }
    }
    pf_assert(veclen(v) == 500);
    vecfree(v);
    return 0;
}

TEST(stack_behaviour) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    for (int i = 0; i < 1000; i++)
        pf_assert_ok(vecpush(v, &i, 1));
    for (int i = 999; i >= 0; i--) {
        int out = -1;
        pf_assert_ok(vecpop(v, &out, 1));
        pf_assert(out == i);
    }
    pf_assert(vecisempty(v));
    vecfree(v);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Swap / swap_remove                                                 */
/* ------------------------------------------------------------------ */

TEST(swap_single_items) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4);
    pf_assert_ok(vecswap(v, 0, 3, 1));
    EXPECT_INTS(v, 4, 2, 3, 1);
    pf_assert_ok(vecswap(v, 2, 1, 1));
    EXPECT_INTS(v, 4, 3, 2, 1);
    vecfree(v);
    return 0;
}

TEST(swap_ranges) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4, 5, 6);
    pf_assert_ok(vecswap(v, 0, 3, 2));
    EXPECT_INTS(v, 4, 5, 3, 1, 2, 6);
    pf_assert_ok(vecswap(v, 3, 0, 2));
    EXPECT_INTS(v, 1, 2, 3, 4, 5, 6);
    vecfree(v);
    return 0;
}

TEST(swap_adjacent_ranges) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4);
    pf_assert_ok(vecswap(v, 0, 2, 2));
    EXPECT_INTS(v, 3, 4, 1, 2);
    vecfree(v);
    return 0;
}

TEST(swap_overlapping_ranges_are_rejected) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4);
    pf_assert(vecswap(v, 0, 1, 2) == ITER_EINVAL);
    pf_assert(vecswap(v, 1, 0, 3) == ITER_EINVAL);
    EXPECT_INTS(v, 1, 2, 3, 4);
    vecfree(v);
    return 0;
}

TEST(swap_out_of_range) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4);
    pf_assert(vecswap(v, 0, 4, 1) == ITER_EINVAL);
    pf_assert(vecswap(v, 4, 0, 1) == ITER_EINVAL);
    pf_assert(vecswap(v, 0, 3, 2) == ITER_EINVAL);
    pf_assert(vecswap(v, 0, 2, 3) == ITER_EINVAL);
    EXPECT_INTS(v, 1, 2, 3, 4);
    vecfree(v);
    return 0;
}

TEST(swap_overflow) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4);
    pf_assert(vecswap(v, SIZE_MAX, 0, 1) == ITER_EOVERFLOW);
    pf_assert(vecswap(v, 0, SIZE_MAX, 1) == ITER_EOVERFLOW);
    pf_assert(vecswap(v, 0, 1, SIZE_MAX) == ITER_EOVERFLOW);
    EXPECT_INTS(v, 1, 2, 3, 4);
    vecfree(v);
    return 0;
}

TEST(swap_on_a_full_vec_needs_scratch_space) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);
    pf_assert_ok(vecresize(v, 4));
    PUSH(v, 1, 2, 3, 4);
    pf_assert(veclen(v) == veccap(v));

    pf_assert_ok(vecswap(v, 0, 3, 1));
    EXPECT_INTS(v, 4, 2, 3, 1);
    pf_assert(veclen(v) == 4);
    vecfree(v);
    pf_assert(t.blocks == 0);
    pf_assert(t.badsize == 0);
    return 0;
}

TEST(swap_out_of_memory_changes_nothing) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);
    pf_assert_ok(vecresize(v, 4));
    PUSH(v, 1, 2, 3, 4);
    t.budget = 0;
    pf_assert(vecswap(v, 0, 3, 1) == ITER_ENOMEM);
    EXPECT_INTS(v, 1, 2, 3, 4);
    t.budget = -1;
    vecfree(v);
    return 0;
}

TEST(swap_remove_single_item) {
    UNUSED;
    ivec v = INTS(0, 1, 2, 3, 4, 5);
    int out = -1;
    pf_assert_ok(vecswap_remove(v, &out, 1, 1));
    pf_assert(out == 1);
    EXPECT_INTS(v, 0, 5, 2, 3, 4);
    vecfree(v);
    return 0;
}

TEST(swap_remove_last_item) {
    UNUSED;
    ivec v = INTS(0, 1, 2, 3);
    pf_assert_ok(vecswap_remove(v, NULL, 3, 1));
    EXPECT_INTS(v, 0, 1, 2);
    vecfree(v);
    return 0;
}

TEST(swap_remove_only_item) {
    UNUSED;
    ivec v = INTS(7);
    int out = -1;
    pf_assert_ok(vecswap_remove(v, &out, 0, 1));
    pf_assert(out == 7);
    EXPECT_EMPTY(v);
    vecfree(v);
    return 0;
}

TEST(swap_remove_range_from_the_front) {
    UNUSED;
    ivec v = INTS(0, 1, 2, 3, 4, 5);
    int out[2] = { -1, -1 };
    pf_assert_ok(vecswap_remove(v, &out[0], 0, 2));
    pf_assert(out[0] == 0 && out[1] == 1);
    EXPECT_INTS(v, 4, 5, 2, 3);
    vecfree(v);
    return 0;
}

TEST(swap_remove_range_touching_the_end) {
    UNUSED;
    ivec v = INTS(0, 1, 2, 3, 4);
    pf_assert_ok(vecswap_remove(v, NULL, 3, 2));
    EXPECT_INTS(v, 0, 1, 2);
    vecfree(v);
    return 0;
}

TEST(swap_remove_range_overlapping_the_tail) {
    UNUSED;
    ivec v = INTS(0, 1, 2, 3, 4);
    int out[2] = { -1, -1 };

    /* only one item lies behind the removed range: it fills the hole */
    pf_assert_ok(vecswap_remove(v, &out[0], 2, 2));
    pf_assert(out[0] == 2 && out[1] == 3);
    EXPECT_INTS(v, 0, 1, 4);
    vecfree(v);
    return 0;
}

TEST(swap_remove_errors) {
    UNUSED;
    ivec v = INTS(0, 1, 2);
    pf_assert(vecswap_remove(v, NULL, 3, 1) == ITER_EINVAL);
    pf_assert(vecswap_remove(v, NULL, 2, 2) == ITER_EINVAL);
    pf_assert(vecswap_remove(v, NULL, SIZE_MAX, 1) == ITER_EOVERFLOW);
    pf_assert(vecswap_remove(v, NULL, 0, SIZE_MAX) == ITER_EOVERFLOW);
    EXPECT_INTS(v, 0, 1, 2);
    vecfree(v);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Iteration                                                          */
/* ------------------------------------------------------------------ */

struct each_ctx {
    ivec v;
    size_t calls;
    long sum;
    int last;
    int stop_at;
    int addresses_ok;
};

static int each_sum(void *item, void *user) {
    struct each_ctx *c = user;
    int *p = item;
    if (p != (int *)vecptr(c->v, c->calls))
        c->addresses_ok = 0;
    c->calls++;
    c->sum += *p;
    c->last = *p;
    return 0;
}

static int each_stop(void *item, void *user) {
    struct each_ctx *c = user;
    c->calls++;
    return *(int *)item == c->stop_at;
}

static int each_negative(void *item, void *user) {
    struct each_ctx *c = user;
    (void)item;
    c->calls++;
    return -1;
}

static int each_increment(void *item, void *user) {
    (void)user;
    (*(int *)item)++;
    return 0;
}

TEST(each_visits_every_item_in_order) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4, 5);
    struct each_ctx c = { v, 0, 0, 0, 0, 1 };
    pf_assert(veceach(v, each_sum, &c) == ITER_OK);
    pf_assert(c.calls == 5);
    pf_assert(c.sum == 15);
    pf_assert(c.last == 5);
    pf_assert(c.addresses_ok);
    vecfree(v);
    return 0;
}

TEST(each_can_modify_items) {
    UNUSED;
    ivec v = INTS(1, 2, 3);
    pf_assert(veceach(v, each_increment, NULL) == ITER_OK);
    EXPECT_INTS(v, 2, 3, 4);
    vecfree(v);
    return 0;
}

TEST(each_stops_when_callback_returns_non_zero) {
    UNUSED;
    ivec v = INTS(1, 2, 3, 4, 5);
    struct each_ctx c = { v, 0, 0, 0, 3, 1 };
    pf_assert(veceach(v, each_stop, &c) == ITER_EINTR);
    pf_assert(c.calls == 3);

    struct each_ctx n = { v, 0, 0, 0, 0, 1 };
    pf_assert(veceach(v, each_negative, &n) == ITER_EINTR);
    pf_assert(n.calls == 1);
    vecfree(v);
    return 0;
}

TEST(each_on_empty_vec) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    struct each_ctx c = { v, 0, 0, 0, 0, 1 };
    pf_assert(veceach(v, each_sum, &c) == ITER_OK);
    pf_assert(c.calls == 0);
    vecfree(v);
    return 0;
}

TEST(each_invalid_arguments) {
    UNUSED;
    ivec v = INTS(1);
    ivec none = NULL;
    struct each_ctx c = { v, 0, 0, 0, 0, 1 };
    pf_assert(veceach(v, NULL, &c) == ITER_EINVAL);
    pf_assert(veceach(none, each_sum, &c) == ITER_EINVAL);
    pf_assert(c.calls == 0);
    vecfree(v);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Index                                                              */
/* ------------------------------------------------------------------ */

TEST(index_of_item_pointers) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    pf_assert_ok(vecreserve(v, 32));
    fill_seq(v, 10);
    pf_assert(veclen(v) < veccap(v));

    for (size_t i = 0; i < 10; i++)
        pf_assert(vecindex(v, vecptr(v, i)) == i);
    vecfree(v);
    return 0;
}

TEST(index_when_length_equals_capacity) {
    UNUSED;
    ivec v = vecnew(int, NULL);
    pf_assert_ok(vecresize(v, 6));
    fill_seq(v, 6);
    pf_assert(veclen(v) == veccap(v));

    for (size_t i = 0; i < 6; i++)
        pf_assert(vecindex(v, vecptr(v, i)) == i);
    vecfree(v);
    return 0;
}

TEST(index_of_struct_items) {
    UNUSED;
    rvec v = vecnew(struct rec, NULL);
    pf_assert_ok(vecreserve(v, 16));
    for (int i = 0; i < 5; i++) {
        struct rec r = mkrec(i);
        pf_assert_ok(vecpush(v, &r, 1));
    }
    for (size_t i = 0; i < 5; i++)
        pf_assert(vecindex(v, vecptr(v, i)) == i);
    vecfree(v);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Element types                                                      */
/* ------------------------------------------------------------------ */

TEST(elements_of_size_one) {
    UNUSED;
    cvec v = vecnew(char, NULL);
    char hello[] = "hello";
    pf_assert_ok(vecpush(v, &hello[0], 5));
    pf_assert(veclen(v) == 5);
    pf_assert_memcmp(vecitems(v), "hello", 5);

    char bang = '!';
    pf_assert_ok(vecpush(v, &bang, 1));
    char us = '_';
    pf_assert_ok(vecinsert(v, &us, 0, 1));
    pf_assert_memcmp(vecitems(v), "_hello!", 7);

    char out[3];
    pf_assert_ok(vecremove(v, &out[0], 1, 3));
    pf_assert_memcmp(out, "hel", 3);
    pf_assert_memcmp(vecitems(v), "_lo!", 4);
    vecfree(v);
    return 0;
}

TEST(elements_of_struct_type) {
    UNUSED;
    rvec v = vecnew(struct rec, NULL);
    for (int i = 0; i < 5; i++) {
        struct rec r = mkrec(i);
        pf_assert_ok(vecpush(v, &r, 1));
    }
    struct rec mid = mkrec(50);
    pf_assert_ok(vecinsert(v, &mid, 2, 1));
    pf_assert(veclen(v) == 6);

    int want[] = { 0, 1, 50, 2, 3, 4 };
    for (size_t i = 0; i < 6; i++)
        pf_assert(rec_is(&vecget(v, i), want[i]));

    struct rec out;
    pf_assert_ok(vecremove(v, &out, 2, 1));
    pf_assert(rec_is(&out, 50));

    pf_assert_ok(vecswap(v, 0, 4, 1));
    pf_assert(rec_is(&vecget(v, 0), 4));
    pf_assert(rec_is(&vecget(v, 4), 0));
    vecfree(v);
    return 0;
}

TEST(elements_of_struct_type_multi_insert_and_fill) {
    UNUSED;
    rvec v = vecnew(struct rec, NULL);
    struct rec base[] = { mkrec(1), mkrec(2), mkrec(7), mkrec(8) };
    pf_assert_ok(vecpush(v, &base[0], 4));

    struct rec mid[] = { mkrec(3), mkrec(4) };
    pf_assert_ok(vecinsert(v, &mid[0], 2, 2));
    struct rec fill = mkrec(9);
    pf_assert_ok(vecfill(v, &fill, 1, 2));

    int want[] = { 1, 9, 9, 2, 3, 4, 7, 8 };
    pf_assert(veclen(v) == 8);
    for (size_t i = 0; i < 8; i++)
        pf_assert(rec_is(&vecget(v, i), want[i]));
    vecfree(v);
    return 0;
}

TEST(elements_of_large_size) {
    UNUSED;
    bvec v = vecnew(struct blob, NULL);
    for (int i = 0; i < 10; i++) {
        struct blob b;
        memset(&b, i + 1, sizeof b);
        pf_assert_ok(vecpush(v, &b, 1));
    }
    pf_assert_ok(vecswap_remove(v, NULL, 0, 1));
    pf_assert(veclen(v) == 9);

    /* the last blob moved to the front */
    struct blob *first = vecptr(v, 0);
    for (size_t i = 0; i < sizeof first->bytes; i++)
        pf_assert(first->bytes[i] == 10);
    struct blob *second = vecptr(v, 1);
    pf_assert(second->bytes[0] == 2 && second->bytes[999] == 2);
    vecfree(v);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Memory behaviour                                                   */
/* ------------------------------------------------------------------ */

TEST(pointers_survive_pushes_within_capacity) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);
    pf_assert_ok(vecreserve(v, 100));
    PUSH(v, 1);
    int *first = vecptr(v, 0);
    size_t reallocs = t.reallocs;

    for (int i = 0; i < 90; i++)
        pf_assert_ok(vecpush(v, &i, 1));

    pf_assert(vecptr(v, 0) == first);
    pf_assert(t.reallocs == reallocs);
    vecfree(v);
    return 0;
}

TEST(allocator_sizes_are_reported_exactly) {
    UNUSED;
    TRACKER(t, -1);
    ivec v = vecnew(int, &t.base);

    fill_seq(v, 100);
    pf_assert_ok(vecresize(v, 500));
    pf_assert_ok(vecresize(v, 50));
    pf_assert_ok(vecreserve(v, 1000));
    pf_assert_ok(vecpop(v, NULL, 10));
    INSERT(v, 5, 1, 2, 3);
    pf_assert_ok(vecremove(v, NULL, 0, 20));
    pf_assert_ok(vecresize(v, 0));
    PUSH(v, 1);

    vecfree(v);
    pf_assert(t.badsize == 0);
    pf_assert(t.blocks == 0);
    pf_assert(t.bytes == 0);
    return 0;
}

TEST(everything_is_released_after_failures) {
    UNUSED;
    for (long budget = 0; budget < 6; budget++) {
        TRACKER(t, budget);
        ivec v = vecnew(int, &t.base);
        if (v) {
            for (int i = 0; i < 200; i++) {
                if (vecpush(v, &i, 1))
                    break;
            }
            vecfree(v);
        }
        pf_assert(t.blocks == 0);
        pf_assert(t.bytes == 0);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Randomized comparison against a plain array                        */
/* ------------------------------------------------------------------ */

#define MODEL_MAX 96

struct model {
    int a[MODEL_MAX + 8];
    size_t n;
};

static void model_insert(struct model *m, size_t i, const int *vals, size_t k) {
    memmove(&m->a[i + k], &m->a[i], (m->n - i) * sizeof(int));
    memcpy(&m->a[i], vals, k * sizeof(int));
    m->n += k;
}

static void model_remove(struct model *m, size_t i, size_t k) {
    memmove(&m->a[i], &m->a[i + k], (m->n - i - k) * sizeof(int));
    m->n -= k;
}

static void model_swap_remove(struct model *m, size_t i, size_t k) {
    size_t tail = m->n - (i + k);
    size_t moved = tail < k ? tail : k;
    memmove(&m->a[i], &m->a[m->n - moved], moved * sizeof(int));
    m->n -= k;
}

TEST(random_operations_match_a_plain_array) {
    for (int round = 0; round < 8; round++) {
        uint32_t rs = rng_seed(seed, rep, round);
        TRACKER(t, -1);
        ivec v = vecnew(int, &t.base);
        struct model m;
        int next = 1;
        m.n = 0;

        for (int step = 0; step < 1500; step++) {
            int op = (int)(rng_next(&rs) % 14);
            size_t k = 1 + rng_next(&rs) % 4;
            size_t i = m.n ? rng_next(&rs) % m.n : 0;
            int vals[4];
            for (size_t j = 0; j < k; j++)
                vals[j] = next++;

            switch (op) {
            case 0:
            case 1: /* push */
                if (m.n + k > MODEL_MAX)
                    break;
                pf_assert_ok(vecpush(v, &vals[0], k));
                model_insert(&m, m.n, vals, k);
                break;
            case 2: /* unshift */
                if (m.n + k > MODEL_MAX)
                    break;
                pf_assert_ok(vecunshift(v, &vals[0], k));
                model_insert(&m, 0, vals, k);
                break;
            case 3: /* insert anywhere, including the end */
                if (m.n + k > MODEL_MAX)
                    break;
                i = rng_next(&rs) % (m.n + 1);
                pf_assert_ok(vecinsert(v, &vals[0], i, k));
                model_insert(&m, i, vals, k);
                break;
            case 4: { /* fill */
                if (m.n + k > MODEL_MAX)
                    break;
                int same[4] = { vals[0], vals[0], vals[0], vals[0] };
                i = rng_next(&rs) % (m.n + 1);
                pf_assert_ok(vecfill(v, &vals[0], i, k));
                model_insert(&m, i, same, k);
                break;
            }
            case 5: { /* pop */
                int out[4];
                if (k > m.n) {
                    pf_assert(vecpop(v, NULL, k) == ITER_EINVAL);
                    break;
                }
                pf_assert_ok(vecpop(v, &out[0], k));
                pf_assert_memcmp(out, &m.a[m.n - k], k * sizeof(int));
                m.n -= k;
                break;
            }
            case 6: { /* shift */
                int out[4];
                if (k > m.n) {
                    pf_assert(vecshift(v, NULL, k) == ITER_EINVAL);
                    break;
                }
                pf_assert_ok(vecshift(v, &out[0], k));
                pf_assert_memcmp(out, &m.a[0], k * sizeof(int));
                model_remove(&m, 0, k);
                break;
            }
            case 7: { /* remove */
                int out[4];
                if (i + k > m.n) {
                    pf_assert(vecremove(v, NULL, i, k) == ITER_EINVAL);
                    break;
                }
                pf_assert_ok(vecremove(v, &out[0], i, k));
                pf_assert_memcmp(out, &m.a[i], k * sizeof(int));
                model_remove(&m, i, k);
                break;
            }
            case 8: { /* swap */
                size_t j = m.n ? rng_next(&rs) % m.n : 0;
                if (i + k > m.n || j + k > m.n) {
                    pf_assert(vecswap(v, i, j, k) == ITER_EINVAL);
                } else if ((i < j ? j - i : i - j) < k) {
                    pf_assert(vecswap(v, i, j, k) == ITER_EINVAL);
                } else {
                    int tmp[4];
                    pf_assert_ok(vecswap(v, i, j, k));
                    memcpy(tmp, &m.a[i], k * sizeof(int));
                    memcpy(&m.a[i], &m.a[j], k * sizeof(int));
                    memcpy(&m.a[j], tmp, k * sizeof(int));
                }
                break;
            }
            case 9: { /* swap_remove */
                int out[4];
                if (i + k > m.n) {
                    pf_assert(vecswap_remove(v, NULL, i, k) == ITER_EINVAL);
                    break;
                }
                pf_assert_ok(vecswap_remove(v, &out[0], i, k));
                pf_assert_memcmp(out, &m.a[i], k * sizeof(int));
                model_swap_remove(&m, i, k);
                break;
            }
            case 10: /* clear, but not too often */
                if (m.n > MODEL_MAX / 2) {
                    vecclear(v);
                    m.n = 0;
                }
                break;
            case 11: /* shrink to fit */
                pf_assert_ok(vecresize(v, m.n + rng_next(&rs) % 8));
                break;
            case 12: /* reserve */
                pf_assert_ok(vecreserve(v, k * 4));
                pf_assert(veccap(v) >= m.n + k * 4);
                break;
            default: /* invalid requests must not change anything */
                pf_assert(vecinsert(v, &vals[0], m.n + 1, k) == ITER_EINVAL);
                pf_assert(vecremove(v, NULL, m.n, 1) == ITER_EINVAL);
                break;
            }

            pf_assert(same_ints(v, m.a, m.n));
        }

        vecfree(v);
        pf_assert(t.blocks == 0);
        pf_assert(t.badsize == 0);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Suite                                                              */
/* ------------------------------------------------------------------ */

#define T(name) { test_##name, #name }

/* clang-format off */
pf_test_t suite_vec[] = {
    /* creation and destruction */
    T(new_is_empty),
    T(new_uses_global_allocator),
    T(new_uses_given_allocator),
    T(new_allocation_failure),
    T(init_uses_caller_storage),
    T(init_invalid_arguments),
    T(new_array_copies_source),
    T(new_array_empty),
    T(new_array_null_source),
    T(new_array_allocation_failure_leaks_nothing),
    T(new_arrayp_copies_pointees),
    T(new_arrayp_failure),
    T(init_array_copies_source),
    T(init_array_allocation_failure),
    T(init_arrayp_copies_pointees),
    T(free_null_is_noop),
    T(free_releases_everything),
    T(unwrap_heap_vec_returns_buffer),
    T(unwrap_initialized_vec_returns_buffer),
    T(unwrap_empty_and_null),

    /* properties */
    T(null_vec_is_safe_to_query),
    T(null_vec_operations_fail),
    T(length_tracks_operations),
    T(ptr_and_slot_bounds),
    T(start_and_end_delimit_items),
    T(foreach_visits_in_order),
    T(foreach_can_modify_items),
    T(foreach_on_empty_does_nothing),
    T(type_helpers),

    /* capacity */
    T(resize_grows_and_keeps_items),
    T(resize_shrink_truncates_length),
    T(resize_shrink_above_length_keeps_length),
    T(resize_to_zero_releases_buffer),
    T(resize_empty_vec_to_zero_is_fine),
    T(resize_overflow),
    T(resize_out_of_memory_changes_nothing),
    T(reserve_on_empty),
    T(reserve_is_relative_to_length),
    T(reserve_is_noop_when_space_exists),
    T(reserve_overflow),
    T(reserve_out_of_memory_changes_nothing),
    T(clear_keeps_capacity_and_buffer),
    T(setlen_is_clamped_to_capacity),
    T(setcap_truncates_length),

    /* push, unshift, insert, fill */
    T(push_appends),
    T(push_zero_items_is_noop),
    T(push_null_items),
    T(push_count_overflow),
    T(push_grows_geometrically),
    T(push_large_batch),
    T(push_within_capacity_does_not_allocate),
    T(push_out_of_memory_changes_nothing),
    T(unshift_prepends),
    T(insert_into_empty),
    T(insert_single_at_front_middle_and_end),
    T(insert_multiple_in_the_middle),
    T(insert_multiple_at_the_front),
    T(insert_multiple_in_the_middle_with_reallocation),
    T(insert_past_the_end_is_rejected),
    T(insert_index_overflow),
    T(fill_at_the_end),
    T(fill_at_the_front),
    T(fill_single_in_the_middle),
    T(fill_multiple_in_the_middle),
    T(fill_zero_count_and_errors),
    T(insertp_copies_pointees_in_order),
    T(insertp_at_the_end_and_front),
    T(insertp_errors),

    /* remove, pop, shift */
    T(remove_copies_removed_items_out),
    T(remove_without_out),
    T(remove_first_and_last),
    T(remove_everything),
    T(remove_zero_items),
    T(remove_out_of_range),
    T(remove_overflow),
    T(pop_removes_from_the_end),
    T(pop_multiple_keeps_original_order),
    T(pop_without_out_and_zero),
    T(pop_more_than_length),
    T(shift_removes_from_the_front),
    T(shift_more_than_length),
    T(queue_behaviour),
    T(stack_behaviour),

    /* swap, swap_remove */
    T(swap_single_items),
    T(swap_ranges),
    T(swap_adjacent_ranges),
    T(swap_overlapping_ranges_are_rejected),
    T(swap_out_of_range),
    T(swap_overflow),
    T(swap_on_a_full_vec_needs_scratch_space),
    T(swap_out_of_memory_changes_nothing),
    T(swap_remove_single_item),
    T(swap_remove_last_item),
    T(swap_remove_only_item),
    T(swap_remove_range_from_the_front),
    T(swap_remove_range_touching_the_end),
    T(swap_remove_range_overlapping_the_tail),
    T(swap_remove_errors),

    /* iteration and index */
    T(each_visits_every_item_in_order),
    T(each_can_modify_items),
    T(each_stops_when_callback_returns_non_zero),
    T(each_on_empty_vec),
    T(each_invalid_arguments),
    T(index_of_item_pointers),
    T(index_when_length_equals_capacity),
    T(index_of_struct_items),

    /* element types */
    T(elements_of_size_one),
    T(elements_of_struct_type),
    T(elements_of_struct_type_multi_insert_and_fill),
    T(elements_of_large_size),

    /* memory behaviour */
    T(pointers_survive_pushes_within_capacity),
    T(allocator_sizes_are_reported_exactly),
    T(everything_is_released_after_failures),

    /* randomized */
    T(random_operations_match_a_plain_array),

    { 0 }
};
/* clang-format on */
