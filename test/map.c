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
#include <iter/map.h>

#include <allocator.h>
#include <allocator_flags.h>
#include <allocator_std.h>
#include <allocator_tracker.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

/* An open-addressing table can spin forever when it is broken, so every
   test arms an alarm. With the forking runner a hang is reported as a
   failure ("alarm clock") instead of blocking the whole suite. */
#ifdef PF__TEST_POSIX
    #define WATCHDOG() ((void)alarm(30))
#else
    #define WATCHDOG() ((void)0)
#endif

#define TEST(name) static int test_##name(int seed, int rep)
#define BEGIN ((void)seed, (void)rep, WATCHDOG())

/* Item types used throughout the suite. */
struct pr {
    int key;
    int val;
};

struct named {
    char name[24];
    int val;
};

struct wide {
    long double ld; /* forces a stricter alignment than int */
    int key;
};

struct big {
    int key;
    unsigned char pad[252];
};

typedef map(int) imap;
typedef map(struct pr) pmap;
typedef map(struct named) nmap;
typedef map(struct wide) wmap;
typedef map(struct big) bmap;

/* tracker_allocator_t (from allocator_t's allocator_tracker.h) tracks
   every block, can be told to start failing via `budget`, and verifies
   the `old` size the library reports on free/realloc. */
#define TRACKER(name, budget_)                            \
    tracker_allocator_t name;                              \
    tracker_allocator_init(&name, &standard_allocator);    \
    name.budget = (budget_)

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

/* Hash callbacks. */

static map_hash_t mix(size_t x) {
    return (map_hash_t)(x * (size_t)0x9E3779B97F4A7C15ull);
}

static map_hash_t hash_int(const void *a, const void *b) {
    int x = *(const int *)a;
    if (!b)
        return mix((size_t)(unsigned)x);
    return x == *(const int *)b ? 0 : 1;
}

/* every key collides */
static map_hash_t hash_const(const void *a, const void *b) {
    if (!b)
        return 0x5A;
    return *(const int *)a == *(const int *)b ? 0 : 1;
}

/* keys are the raw bytes, hashed with the library's own maphash() */
static map_hash_t hash_bytes(const void *a, const void *b) {
    if (!b)
        return (map_hash_t)maphash(a, sizeof(int));
    return *(const int *)a == *(const int *)b ? 0 : 1;
}

static map_hash_t hash_pair(const void *a, const void *b) {
    const struct pr *x = a;
    if (!b)
        return mix((size_t)(unsigned)x->key);
    return x->key == ((const struct pr *)b)->key ? 0 : 1;
}

static map_hash_t hash_named(const void *a, const void *b) {
    const struct named *x = a;
    if (!b)
        return (map_hash_t)maphash(x->name, strlen(x->name));
    return strcmp(x->name, ((const struct named *)b)->name) == 0 ? 0 : 1;
}

static map_hash_t hash_wide(const void *a, const void *b) {
    const struct wide *x = a;
    if (!b)
        return mix((size_t)(unsigned)x->key);
    return x->key == ((const struct wide *)b)->key ? 0 : 1;
}

static map_hash_t hash_big(const void *a, const void *b) {
    const struct big *x = a;
    if (!b)
        return mix((size_t)(unsigned)x->key);
    return x->key == ((const struct big *)b)->key ? 0 : 1;
}

/* Small wrappers to keep the tests readable. */

static int has(imap m, int key) { return mapptr(m, &key) != NULL; }

static int setk(imap m, int key) { return mapset(m, &key); }

static int setp(pmap m, int key, int val) {
    struct pr p = { key, val };
    return mapset(m, &p);
}

static int insp(pmap m, int key, int val) {
    struct pr p = { key, val };
    return mapinsert(m, &p);
}

static int updp(pmap m, int key, int val) {
    struct pr p = { key, val };
    return mapupdate(m, &p);
}

/* Returns the value stored for `key`, or -1 if it is absent. */
static int valp(pmap m, int key) {
    struct pr probe = { key, 0 }, out = { 0, -1 };
    if (mapget(m, &probe, &out))
        return -1;
    return out.val;
}

static int hasp(pmap m, int key) {
    struct pr probe = { key, 0 };
    return mapptr(m, &probe) != NULL;
}

/* ------------------------------------------------------------------ */
/* Creation and destruction                                           */
/* ------------------------------------------------------------------ */

TEST(new_is_empty) {
    BEGIN;
    imap m = mapnew(int, hash_int, NULL);
    pf_assert_not_null(m);
    pf_assert(mapcount(m) == 0);
    pf_assert(mapisempty(m));
    mapfree(m);
    return 0;
}

TEST(new_uses_global_allocator) {
    BEGIN;
    TRACKER(t, -1);

    libiter_use_allocator(&t.alloc);
    imap m = mapnew(int, hash_int, NULL);
    libiter_use_allocator(&standard_allocator);

    pf_assert_not_null(m);
    pf_assert(mapallocator(m) == &t.alloc);
    pf_assert(t.blocks == 1);
    pf_assert_ok(setk(m, 1));
    pf_assert(t.blocks >= 2);
    mapfree(m);
    pf_assert(t.blocks == 0);
    pf_assert(t.badSizes == 0);
    return 0;
}

TEST(new_uses_given_allocator) {
    BEGIN;
    TRACKER(t, -1);
    imap m = mapnew(int, hash_int, &t.alloc);
    pf_assert_not_null(m);
    pf_assert(mapallocator(m) == &t.alloc);
    pf_assert(t.blocks == 1);
    mapfree(m);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(new_allocation_failure) {
    BEGIN;
    TRACKER(t, 0);
    imap m = mapnew(int, hash_int, &t.alloc);
    pf_assert_null(m);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(new_invalid_arguments) {
    BEGIN;
    TRACKER(t, -1);
    pf_assert_null(mapnew(int, NULL, NULL));
    pf_assert_null(mapnew(int, NULL, &t.alloc));

    allocator_t *tagged = allocator_flags_set(&t.alloc, 1);
    pf_assert_null(mapnew(int, hash_int, tagged));
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(init_uses_caller_storage) {
    BEGIN;
    TRACKER(t, -1);
    map_t raw;
    memset(&raw, 0xAB, sizeof raw);

    imap m = mapinit(int, &raw, hash_int, &t.alloc);
    pf_assert(map_base(m) == &raw);
    pf_assert(t.blocks == 0);
    pf_assert(mapcount(m) == 0);
    pf_assert(mapallocator(m) == &t.alloc);

    for (int i = 0; i < 20; i++)
        pf_assert_ok(setk(m, i));
    pf_assert(mapcount(m) == 20);
    pf_assert(t.blocks >= 1);

    /* only what the map allocated may be released, `raw` is on the stack */
    mapfree(m);
    pf_assert(t.blocks == 0);
    pf_assert(t.badSizes == 0);
    return 0;
}

TEST(init_invalid_arguments) {
    BEGIN;
    TRACKER(t, -1);
    map_t raw;
    pf_assert_null(mapinit(int, NULL, hash_int, NULL));
    pf_assert_null(mapinit(int, &raw, NULL, NULL));

    allocator_t *tagged = allocator_flags_set(&t.alloc, 1);
    pf_assert_null(mapinit(int, &raw, hash_int, tagged));
    return 0;
}

TEST(free_null_is_noop) {
    BEGIN;
    imap m = NULL;
    mapfree(m);
    return 0;
}

TEST(free_releases_everything) {
    BEGIN;
    TRACKER(t, -1);
    imap m = mapnew(int, hash_int, &t.alloc);
    for (int i = 0; i < 300; i++)
        pf_assert_ok(setk(m, i));
    mapfree(m);
    pf_assert(t.blocks == 0);
    pf_assert(t.bytes == 0);
    pf_assert(t.badSizes == 0);
    return 0;
}

TEST(null_map_is_safe_to_query) {
    BEGIN;
    imap m = NULL;
    int k = 1;
    pf_assert(mapcount(m) == 0);
    pf_assert(mapcap(m) == 0);
    pf_assert(mapisempty(m));
    pf_assert_null(mapallocator(m));
    pf_assert_null(mapptr(m, &k));
    return 0;
}

static int cb_never(void *item, void *user) {
    (void)item;
    (void)user;
    return 0;
}

TEST(null_map_operations_fail) {
    BEGIN;
    imap m = NULL;
    int k = 1, out = 0;
    pf_assert(mapreserve(m, 1) == ITER_EINVAL);
    pf_assert(mapset(m, &k) == ITER_EINVAL);
    pf_assert(mapinsert(m, &k) == ITER_EINVAL);
    pf_assert(mapupdate(m, &k) == ITER_EINVAL);
    pf_assert(mapremove(m, &k, &out) == ITER_EINVAL);
    pf_assert(mapget(m, &k, &out) == ITER_EINVAL);
    pf_assert(mapeach(m, cb_never, NULL) == ITER_EINVAL);
    return 0;
}

TEST(null_key_operations_fail) {
    BEGIN;
    imap m = mapnew(int, hash_int, NULL);
    setk(m, 1);
    int *none = NULL;
    int out = 0;
    pf_assert(mapset(m, none) == ITER_EINVAL);
    pf_assert(mapinsert(m, none) == ITER_EINVAL);
    pf_assert(mapupdate(m, none) == ITER_EINVAL);
    pf_assert(mapremove(m, none, &out) == ITER_EINVAL);
    pf_assert(mapget(m, none, &out) == ITER_EINVAL);
    pf_assert_null(mapptr(m, none));
    pf_assert(mapcount(m) == 1);
    mapfree(m);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Reserve                                                            */
/* ------------------------------------------------------------------ */

TEST(reserve_makes_room) {
    BEGIN;
    imap m = mapnew(int, hash_int, NULL);
    pf_assert_ok(mapreserve(m, 10));
    pf_assert(mapcap(m) >= 10);
    pf_assert(mapcount(m) == 0);
    pf_assert(mapisempty(m));
    mapfree(m);
    return 0;
}

TEST(reserve_zero_is_invalid) {
    BEGIN;
    imap m = mapnew(int, hash_int, NULL);
    pf_assert(mapreserve(m, 0) == ITER_EINVAL);
    mapfree(m);
    return 0;
}

TEST(reserve_avoids_later_reallocation) {
    BEGIN;
    TRACKER(t, -1);
    imap m = mapnew(int, hash_int, &t.alloc);
    pf_assert_ok(mapreserve(m, 200));
    size_t before = t.allocs + t.reallocs;

    for (int i = 0; i < 200; i++)
        pf_assert_ok(setk(m, i));

    pf_assert(t.allocs + t.reallocs == before);
    pf_assert(mapcount(m) == 200);
    mapfree(m);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(reserve_keeps_existing_items) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    for (int i = 0; i < 20; i++)
        pf_assert_ok(setp(m, i, i * 10));

    pf_assert_ok(mapreserve(m, 1000));
    pf_assert(mapcap(m) >= 1000);
    pf_assert(mapcount(m) == 20);
    for (int i = 0; i < 20; i++)
        pf_assert(valp(m, i) == i * 10);
    mapfree(m);
    return 0;
}

TEST(reserve_never_shrinks) {
    BEGIN;
    imap m = mapnew(int, hash_int, NULL);
    pf_assert_ok(mapreserve(m, 500));
    size_t cap = mapcap(m);
    pf_assert_ok(mapreserve(m, 1));
    pf_assert_ok(mapreserve(m, 10));
    pf_assert(mapcap(m) == cap);
    mapfree(m);
    return 0;
}

TEST(reserve_out_of_memory_changes_nothing) {
    BEGIN;
    TRACKER(t, -1);
    imap m = mapnew(int, hash_int, &t.alloc);
    for (int i = 0; i < 10; i++)
        pf_assert_ok(setk(m, i));
    size_t cap = mapcap(m);

    t.budget = 0;
    pf_assert(mapreserve(m, cap * 10) == ITER_ENOMEM);
    pf_assert(mapcap(m) == cap);
    pf_assert(mapcount(m) == 10);
    for (int i = 0; i < 10; i++)
        pf_assert(has(m, i));

    t.budget = -1;
    mapfree(m);
    pf_assert(t.blocks == 0);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Lookups on an empty map                                            */
/* ------------------------------------------------------------------ */

TEST(empty_map_has_no_items) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    struct pr probe = { 1, 0 }, out = { 0, -1 };

    pf_assert_null(mapptr(m, &probe));
    pf_assert(mapget(m, &probe, &out) == ITER_ENOENT);
    pf_assert(out.val == -1);
    pf_assert(mapupdate(m, &probe) == ITER_ENOENT);
    pf_assert(mapremove(m, &probe, &out) == ITER_ENOENT);
    pf_assert(mapremove(m, &probe, NULL) == ITER_ENOENT);
    pf_assert(mapcount(m) == 0);
    mapfree(m);
    return 0;
}

TEST(empty_map_iteration) {
    BEGIN;
    imap m = mapnew(int, hash_int, NULL);
    pf_assert(mapeach(m, cb_never, NULL) == ITER_OK);
    mapfree(m);
    return 0;
}

TEST(reserved_but_empty_map_has_no_items) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(mapreserve(m, 16));
    struct pr probe = { 3, 0 };
    pf_assert_null(mapptr(m, &probe));
    pf_assert(mapupdate(m, &probe) == ITER_ENOENT);
    pf_assert(mapremove(m, &probe, NULL) == ITER_ENOENT);
    mapfree(m);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Set / insert / update                                              */
/* ------------------------------------------------------------------ */

TEST(set_adds_new_items) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    pf_assert(mapcount(m) == 1);
    pf_assert(!mapisempty(m));
    pf_assert_ok(setp(m, 2, 20));
    pf_assert_ok(setp(m, 3, 30));
    pf_assert(mapcount(m) == 3);
    pf_assert(valp(m, 1) == 10);
    pf_assert(valp(m, 2) == 20);
    pf_assert(valp(m, 3) == 30);
    mapfree(m);
    return 0;
}

TEST(set_overwrites_existing_items) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    pf_assert_ok(setp(m, 1, 11));
    pf_assert_ok(setp(m, 1, 12));
    pf_assert(mapcount(m) == 1);
    pf_assert(valp(m, 1) == 12);
    mapfree(m);
    return 0;
}

TEST(set_copies_the_item) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    struct pr p = { 1, 10 };
    pf_assert_ok(mapset(m, &p));
    p.val = 99;
    pf_assert(valp(m, 1) == 10);
    mapfree(m);
    return 0;
}

TEST(set_out_of_memory_changes_nothing) {
    BEGIN;
    TRACKER(t, -1);
    pmap m = mapnew(struct pr, hash_pair, &t.alloc);
    t.budget = 0;

    pf_assert(setp(m, 1, 10) == ITER_ENOMEM);
    pf_assert(mapcount(m) == 0);
    pf_assert(!hasp(m, 1));

    t.budget = -1;
    pf_assert_ok(setp(m, 1, 10));
    pf_assert(valp(m, 1) == 10);
    mapfree(m);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(insert_adds_new_items) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(insp(m, 1, 10));
    pf_assert_ok(insp(m, 2, 20));
    pf_assert(mapcount(m) == 2);
    pf_assert(valp(m, 1) == 10);
    pf_assert(valp(m, 2) == 20);
    mapfree(m);
    return 0;
}

TEST(insert_refuses_duplicates) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(insp(m, 1, 10));
    pf_assert(insp(m, 1, 99) == ITER_EEXIST);
    pf_assert(mapcount(m) == 1);
    pf_assert(valp(m, 1) == 10);
    mapfree(m);
    return 0;
}

TEST(insert_out_of_memory_changes_nothing) {
    BEGIN;
    TRACKER(t, -1);
    pmap m = mapnew(struct pr, hash_pair, &t.alloc);
    t.budget = 0;
    pf_assert(insp(m, 1, 10) == ITER_ENOMEM);
    pf_assert(mapcount(m) == 0);
    t.budget = -1;
    mapfree(m);
    return 0;
}

TEST(update_replaces_existing_items) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    pf_assert_ok(setp(m, 2, 20));
    pf_assert_ok(updp(m, 1, 11));
    pf_assert(valp(m, 1) == 11);
    pf_assert(valp(m, 2) == 20);
    pf_assert(mapcount(m) == 2);
    mapfree(m);
    return 0;
}

TEST(update_never_adds_items) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    pf_assert(updp(m, 2, 20) == ITER_ENOENT);
    pf_assert(mapcount(m) == 1);
    pf_assert(!hasp(m, 2));
    mapfree(m);
    return 0;
}

TEST(update_does_not_touch_removed_items) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    struct pr probe = { 1, 0 };
    pf_assert_ok(mapremove(m, &probe, NULL));
    pf_assert(updp(m, 1, 11) == ITER_ENOENT);
    pf_assert(mapcount(m) == 0);
    mapfree(m);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Get / ptr                                                          */
/* ------------------------------------------------------------------ */

TEST(get_copies_the_stored_item) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 7, 70));
    struct pr probe = { 7, 0 }, out = { 0, 0 };
    pf_assert_ok(mapget(m, &probe, &out));
    pf_assert(out.key == 7 && out.val == 70);
    mapfree(m);
    return 0;
}

TEST(get_missing_leaves_out_untouched) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    struct pr probe = { 2, 0 }, out = { -5, -5 };
    pf_assert(mapget(m, &probe, &out) == ITER_ENOENT);
    pf_assert(out.key == -5 && out.val == -5);
    mapfree(m);
    return 0;
}

TEST(get_without_out_checks_existence) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    struct pr yes = { 1, 0 }, no = { 2, 0 };

    /* mapget() type-checks `out`, which rejects a literal NULL at compile
       time, so the typeless function is called instead */
    pf_assert_ok(map_get(map_base(m), &yes, NULL));
    pf_assert(map_get(map_base(m), &no, NULL) == ITER_ENOENT);
    mapfree(m);
    return 0;
}

TEST(get_allows_key_and_out_to_alias) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 4, 40));
    struct pr both = { 4, 0 };
    pf_assert_ok(mapget(m, &both, &both));
    pf_assert(both.key == 4 && both.val == 40);
    mapfree(m);
    return 0;
}

TEST(get_returns_a_copy) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    struct pr probe = { 1, 0 }, out;
    pf_assert_ok(mapget(m, &probe, &out));
    out.val = 999;
    pf_assert(valp(m, 1) == 10);
    mapfree(m);
    return 0;
}

TEST(ptr_gives_access_to_the_stored_item) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    struct pr probe = { 1, 0 };

    struct pr *p = mapptr(m, &probe);
    pf_assert_not_null(p);
    pf_assert(p->key == 1 && p->val == 10);

    p->val = 55; /* edit in place, the key must stay untouched */
    pf_assert(valp(m, 1) == 55);
    pf_assert(mapptr(m, &probe) == p);
    mapfree(m);
    return 0;
}

TEST(ptr_of_missing_item_is_null) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    struct pr probe = { 2, 0 };
    pf_assert_null(mapptr(m, &probe));
    mapfree(m);
    return 0;
}

TEST(ptr_is_stable_while_capacity_suffices) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(mapreserve(m, 100));
    pf_assert_ok(setp(m, 1, 10));
    struct pr probe = { 1, 0 };
    struct pr *first = mapptr(m, &probe);

    for (int i = 2; i < 60; i++)
        pf_assert_ok(setp(m, i, i));

    pf_assert(mapptr(m, &probe) == first);
    pf_assert(first->val == 10);
    mapfree(m);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Remove                                                             */
/* ------------------------------------------------------------------ */

TEST(remove_copies_the_item_out) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    pf_assert_ok(setp(m, 2, 20));

    struct pr probe = { 1, 0 }, out = { 0, 0 };
    pf_assert_ok(mapremove(m, &probe, &out));
    pf_assert(out.key == 1 && out.val == 10);
    pf_assert(mapcount(m) == 1);
    pf_assert(!hasp(m, 1));
    pf_assert(valp(m, 2) == 20);
    mapfree(m);
    return 0;
}

TEST(remove_without_out) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    struct pr probe = { 1, 0 };
    pf_assert_ok(mapremove(m, &probe, NULL));
    pf_assert(mapcount(m) == 0);
    pf_assert(mapisempty(m));
    mapfree(m);
    return 0;
}

TEST(remove_allows_key_and_out_to_alias) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 3, 30));
    struct pr both = { 3, 0 };
    pf_assert_ok(mapremove(m, &both, &both));
    pf_assert(both.key == 3 && both.val == 30);
    mapfree(m);
    return 0;
}

TEST(remove_missing_item) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    struct pr probe = { 2, 0 }, out = { -5, -5 };
    pf_assert(mapremove(m, &probe, &out) == ITER_ENOENT);
    pf_assert(out.key == -5 && out.val == -5);
    pf_assert(mapcount(m) == 1);
    mapfree(m);
    return 0;
}

TEST(remove_twice) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(setp(m, 1, 10));
    struct pr probe = { 1, 0 };
    pf_assert_ok(mapremove(m, &probe, NULL));
    pf_assert(mapremove(m, &probe, NULL) == ITER_ENOENT);
    pf_assert(mapcount(m) == 0);
    mapfree(m);
    return 0;
}

TEST(removed_items_can_be_inserted_again) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    pf_assert_ok(insp(m, 1, 10));
    struct pr probe = { 1, 0 };
    pf_assert_ok(mapremove(m, &probe, NULL));
    pf_assert_ok(insp(m, 1, 11));
    pf_assert(mapcount(m) == 1);
    pf_assert(valp(m, 1) == 11);
    mapfree(m);
    return 0;
}

TEST(remove_keeps_the_other_items) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    for (int i = 0; i < 100; i++)
        pf_assert_ok(setp(m, i, i * 2));

    for (int i = 0; i < 100; i += 2) {
        struct pr probe = { i, 0 };
        pf_assert_ok(mapremove(m, &probe, NULL));
    }

    pf_assert(mapcount(m) == 50);
    for (int i = 0; i < 100; i++) {
        if (i % 2)
            pf_assert(valp(m, i) == i * 2);
        else
            pf_assert(!hasp(m, i));
    }
    mapfree(m);
    return 0;
}

TEST(map_is_reusable_after_removing_everything) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 30; i++)
            pf_assert_ok(insp(m, i, i + round));
        pf_assert(mapcount(m) == 30);

        for (int i = 0; i < 30; i++) {
            struct pr probe = { i, 0 }, out;
            pf_assert_ok(mapremove(m, &probe, &out));
            pf_assert(out.val == i + round);
        }
        pf_assert(mapcount(m) == 0);
        pf_assert(mapisempty(m));
    }
    mapfree(m);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Iteration                                                          */
/* ------------------------------------------------------------------ */

#define SEEN_MAX 256

struct each_ctx {
    int seen[SEEN_MAX];
    size_t calls;
    long sum;
    size_t stop_after;
};

static int each_record(void *item, void *user) {
    struct each_ctx *c = user;
    struct pr *p = item;
    if (p->key >= 0 && p->key < SEEN_MAX)
        c->seen[p->key]++;
    c->calls++;
    c->sum += p->val;
    return 0;
}

static int each_stop(void *item, void *user) {
    struct each_ctx *c = user;
    (void)item;
    c->calls++;
    return c->calls == c->stop_after;
}

static int each_negative(void *item, void *user) {
    struct each_ctx *c = user;
    (void)item;
    c->calls++;
    return -1;
}

static int each_add_one(void *item, void *user) {
    (void)user;
    ((struct pr *)item)->val += 1;
    return 0;
}

TEST(each_visits_every_item_exactly_once) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    long expect = 0;
    for (int i = 0; i < 50; i++) {
        pf_assert_ok(setp(m, i, i * 3));
        expect += i * 3;
    }

    struct each_ctx c;
    memset(&c, 0, sizeof c);
    pf_assert(mapeach(m, each_record, &c) == ITER_OK);
    pf_assert(c.calls == 50);
    pf_assert(c.sum == expect);
    for (int i = 0; i < 50; i++)
        pf_assert(c.seen[i] == 1);
    for (int i = 50; i < SEEN_MAX; i++)
        pf_assert(c.seen[i] == 0);
    mapfree(m);
    return 0;
}

TEST(each_stops_when_callback_returns_non_zero) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    for (int i = 0; i < 20; i++)
        pf_assert_ok(setp(m, i, i));

    struct each_ctx c;
    memset(&c, 0, sizeof c);
    c.stop_after = 5;
    pf_assert(mapeach(m, each_stop, &c) == ITER_EINTR);
    pf_assert(c.calls == 5);

    memset(&c, 0, sizeof c);
    pf_assert(mapeach(m, each_negative, &c) == ITER_EINTR);
    pf_assert(c.calls == 1);
    mapfree(m);
    return 0;
}

TEST(each_can_modify_values) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    for (int i = 0; i < 10; i++)
        pf_assert_ok(setp(m, i, i));
    pf_assert(mapeach(m, each_add_one, NULL) == ITER_OK);
    for (int i = 0; i < 10; i++)
        pf_assert(valp(m, i) == i + 1);
    mapfree(m);
    return 0;
}

TEST(each_skips_removed_items) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    for (int i = 0; i < 40; i++)
        pf_assert_ok(setp(m, i, 1));
    for (int i = 0; i < 40; i += 3) {
        struct pr probe = { i, 0 };
        pf_assert_ok(mapremove(m, &probe, NULL));
    }

    struct each_ctx c;
    memset(&c, 0, sizeof c);
    pf_assert(mapeach(m, each_record, &c) == ITER_OK);
    pf_assert(c.calls == mapcount(m));
    for (int i = 0; i < 40; i++)
        pf_assert(c.seen[i] == (i % 3 != 0));
    mapfree(m);
    return 0;
}

TEST(each_on_a_map_emptied_by_removals) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    for (int i = 0; i < 10; i++)
        pf_assert_ok(setp(m, i, i));
    for (int i = 0; i < 10; i++) {
        struct pr probe = { i, 0 };
        pf_assert_ok(mapremove(m, &probe, NULL));
    }
    struct each_ctx c;
    memset(&c, 0, sizeof c);
    pf_assert(mapeach(m, each_record, &c) == ITER_OK);
    pf_assert(c.calls == 0);
    mapfree(m);
    return 0;
}

TEST(each_invalid_arguments) {
    BEGIN;
    imap m = mapnew(int, hash_int, NULL);
    pf_assert(mapeach(m, NULL, NULL) == ITER_EINVAL);
    mapfree(m);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Growth, churn and collisions                                       */
/* ------------------------------------------------------------------ */

TEST(many_insertions) {
    BEGIN;
    enum { N = 3000 };
    imap m = mapnew(int, hash_int, NULL);
    for (int i = 0; i < N; i++)
        pf_assert_ok(setk(m, i));

    pf_assert(mapcount(m) == N);
    pf_assert(mapcap(m) >= N);
    for (int i = 0; i < N; i++)
        pf_assert(has(m, i));
    for (int i = N; i < N + 500; i++)
        pf_assert(!has(m, i));
    for (int i = -500; i < 0; i++)
        pf_assert(!has(m, i));
    mapfree(m);
    return 0;
}

TEST(growth_is_proportional_to_the_item_count) {
    BEGIN;
    imap m = mapnew(int, hash_int, NULL);
    for (int i = 0; i < 1000; i++) {
        pf_assert_ok(setk(m, i));
        pf_assert(mapcap(m) <= 16 * mapcount(m) + 16);
    }
    mapfree(m);
    return 0;
}

TEST(growth_is_amortized) {
    BEGIN;
    TRACKER(t, -1);
    imap m = mapnew(int, hash_int, &t.alloc);
    for (int i = 0; i < 2000; i++)
        pf_assert_ok(setk(m, i));
    pf_assert(t.allocs + t.reallocs < 64);
    mapfree(m);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(growth_keeps_all_values) {
    BEGIN;
    pmap m = mapnew(struct pr, hash_pair, NULL);
    for (int i = 0; i < 1500; i++) {
        pf_assert_ok(setp(m, i * 7, i));
        /* spot check what was inserted before, after every growth step */
        if (i % 97 == 0)
            for (int j = 0; j <= i; j += 13)
                pf_assert(valp(m, j * 7) == j);
    }
    for (int i = 0; i < 1500; i++)
        pf_assert(valp(m, i * 7) == i);
    mapfree(m);
    return 0;
}

TEST(insert_and_remove_churn) {
    BEGIN;
    imap m = mapnew(int, hash_int, NULL);
    enum { LIVE = 8, ROUNDS = 20000 };

    for (int i = 0; i < ROUNDS; i++) {
        pf_assert_ok(setk(m, i));
        if (i >= LIVE) {
            int old = i - LIVE;
            pf_assert_ok(mapremove(m, &old, NULL));
        }
    }

    pf_assert(mapcount(m) == LIVE);
    for (int i = ROUNDS - LIVE; i < ROUNDS; i++)
        pf_assert(has(m, i));
    for (int i = 0; i < ROUNDS - LIVE; i += 101)
        pf_assert(!has(m, i));
    mapfree(m);
    return 0;
}

TEST(churn_does_not_grow_memory_without_bound) {
    BEGIN;
    TRACKER(t, -1);
    imap m = mapnew(int, hash_int, &t.alloc);
    enum { LIVE = 8, ROUNDS = 20000 };

    for (int i = 0; i < ROUNDS; i++) {
        pf_assert_ok(setk(m, i));
        if (i >= LIVE) {
            int old = i - LIVE;
            pf_assert_ok(mapremove(m, &old, NULL));
        }
    }

    /* a handful of live items must not need a big table... */
    pf_assert(mapcap(m) <= 64 * LIVE);
    pf_assert(t.bytes < 64 * 1024);
    /* ...and clearing out dead slots must not happen on every insert */
    pf_assert(t.allocs + t.reallocs < ROUNDS / 8);
    mapfree(m);
    pf_assert(t.blocks == 0);
    return 0;
}

TEST(all_keys_colliding) {
    BEGIN;
    imap m = mapnew(int, hash_const, NULL);
    for (int i = 0; i < 60; i++)
        pf_assert_ok(setk(m, i));
    pf_assert(mapcount(m) == 60);
    for (int i = 0; i < 60; i++)
        pf_assert(has(m, i));
    for (int i = 60; i < 80; i++)
        pf_assert(!has(m, i));

    /* duplicates are still detected */
    int dup = 30;
    pf_assert(mapinsert(m, &dup) == ITER_EEXIST);
    pf_assert(mapcount(m) == 60);
    mapfree(m);
    return 0;
}

TEST(all_keys_colliding_with_removals) {
    BEGIN;
    imap m = mapnew(int, hash_const, NULL);
    for (int i = 0; i < 60; i++)
        pf_assert_ok(setk(m, i));

    /* removing from the middle of a probe chain must not hide the rest */
    for (int i = 0; i < 60; i += 2) {
        int k = i;
        pf_assert_ok(mapremove(m, &k, NULL));
    }
    pf_assert(mapcount(m) == 30);
    for (int i = 0; i < 60; i++)
        pf_assert(has(m, i) == (i % 2));

    for (int i = 0; i < 60; i += 2)
        pf_assert_ok(setk(m, i));
    pf_assert(mapcount(m) == 60);
    for (int i = 0; i < 60; i++)
        pf_assert(has(m, i));
    mapfree(m);
    return 0;
}

TEST(keys_at_integer_limits) {
    BEGIN;
    imap m = mapnew(int, hash_int, NULL);
    int keys[] = { 0, 1, -1, INT_MAX, INT_MIN, 255, 256, 65535, 65536, -256 };
    size_t n = sizeof keys / sizeof keys[0];

    for (size_t i = 0; i < n; i++)
        pf_assert_ok(setk(m, keys[i]));
    pf_assert(mapcount(m) == n);
    for (size_t i = 0; i < n; i++)
        pf_assert(has(m, keys[i]));
    int missing = 2;
    pf_assert(!has(m, missing));
    mapfree(m);
    return 0;
}

TEST(hash_bytes_helper_works_as_a_hash_function) {
    BEGIN;
    imap m = mapnew(int, hash_bytes, NULL);
    for (int i = 0; i < 500; i++)
        pf_assert_ok(setk(m, i * 31 - 7));
    pf_assert(mapcount(m) == 500);
    for (int i = 0; i < 500; i++)
        pf_assert(has(m, i * 31 - 7));
    mapfree(m);
    return 0;
}

/* ------------------------------------------------------------------ */
/* maphash                                                            */
/* ------------------------------------------------------------------ */

TEST(maphash_is_deterministic) {
    BEGIN;
    char a[] = "hello world";
    char b[] = "hello world";
    pf_assert(maphash(a, sizeof a) == maphash(b, sizeof b));
    pf_assert(maphash(a, 5) == maphash(a, 5));
    return 0;
}

TEST(maphash_depends_on_content_and_length) {
    BEGIN;
    pf_assert(maphash("a", 1) != maphash("b", 1));
    pf_assert(maphash("ab", 2) != maphash("ba", 2));
    pf_assert(maphash("a", 1) != maphash("a\0", 2));
    pf_assert(maphash("abc", 3) != maphash("abd", 3));
    return 0;
}

TEST(maphash_only_reads_the_given_size) {
    BEGIN;
    pf_assert(maphash("abc", 2) == maphash("abx", 2));
    pf_assert(maphash("x", 0) == maphash("y", 0));
    pf_assert(maphash(NULL, 0) == maphash("y", 0));
    return 0;
}

TEST(maphash_spreads_integers) {
    BEGIN;
    enum { N = 2000 };
    static int hashes[N];
    for (int i = 0; i < N; i++)
        hashes[i] = maphash(&i, sizeof i);

    int distinct = 0;
    for (int i = 0; i < N; i++) {
        int seen = 0;
        for (int j = 0; j < i && !seen; j++)
            seen = hashes[j] == hashes[i];
        distinct += !seen;
    }
    pf_assert(distinct > N * 99 / 100);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Item types                                                         */
/* ------------------------------------------------------------------ */

TEST(string_keys) {
    BEGIN;
    nmap m = mapnew(struct named, hash_named, NULL);
    const char *names[] = { "alpha", "beta", "gamma", "al", "alph", "Alpha" };
    size_t n = sizeof names / sizeof names[0];

    for (size_t i = 0; i < n; i++) {
        struct named item;
        memset(&item, 0, sizeof item);
        strcpy(item.name, names[i]);
        item.val = (int)i;
        pf_assert_ok(mapset(m, &item));
    }
    pf_assert(mapcount(m) == n);

    for (size_t i = 0; i < n; i++) {
        struct named probe, out;
        memset(&probe, 0, sizeof probe);
        strcpy(probe.name, names[i]);
        pf_assert_ok(mapget(m, &probe, &out));
        pf_assert(out.val == (int)i);
        pf_assert(strcmp(out.name, names[i]) == 0);
    }

    struct named missing;
    memset(&missing, 0, sizeof missing);
    strcpy(missing.name, "delta");
    pf_assert_null(mapptr(m, &missing));

    struct named upd;
    memset(&upd, 0, sizeof upd);
    strcpy(upd.name, "beta");
    upd.val = 100;
    pf_assert_ok(mapupdate(m, &upd));
    struct named *found = mapptr(m, &upd);
    pf_assert_not_null(found);
    pf_assert(found->val == 100);
    mapfree(m);
    return 0;
}

TEST(items_respect_their_alignment) {
    BEGIN;
    wmap m = mapnew(struct wide, hash_wide, NULL);
    for (int i = 0; i < 100; i++) {
        struct wide w;
        memset(&w, 0, sizeof w);
        w.key = i;
        w.ld = i * 1.5L;
        pf_assert_ok(mapset(m, &w));
    }
    for (int i = 0; i < 100; i++) {
        struct wide probe;
        memset(&probe, 0, sizeof probe);
        probe.key = i;
        struct wide *p = mapptr(m, &probe);
        pf_assert_not_null(p);
        pf_assert((uintptr_t)p % alignof(struct wide) == 0);
        pf_assert(p->ld == i * 1.5L);
    }
    mapfree(m);
    return 0;
}

TEST(large_items) {
    BEGIN;
    bmap m = mapnew(struct big, hash_big, NULL);
    for (int i = 0; i < 100; i++) {
        struct big b;
        memset(&b, i, sizeof b);
        b.key = i;
        pf_assert_ok(mapset(m, &b));
    }
    for (int i = 0; i < 100; i++) {
        struct big probe, out;
        memset(&probe, 0, sizeof probe);
        probe.key = i;
        pf_assert_ok(mapget(m, &probe, &out));
        pf_assert(out.key == i);
        for (size_t j = 0; j < sizeof out.pad; j++)
            pf_assert(out.pad[j] == (unsigned char)i);
    }
    mapfree(m);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Memory behaviour                                                   */
/* ------------------------------------------------------------------ */

TEST(everything_is_released_after_heavy_use) {
    BEGIN;
    TRACKER(t, -1);
    imap m = mapnew(int, hash_int, &t.alloc);
    for (int i = 0; i < 1000; i++)
        pf_assert_ok(setk(m, i));
    for (int i = 0; i < 1000; i += 2) {
        int k = i;
        pf_assert_ok(mapremove(m, &k, NULL));
    }
    for (int i = 2000; i < 2700; i++)
        pf_assert_ok(setk(m, i));
    pf_assert_ok(mapreserve(m, 5000));

    mapfree(m);
    pf_assert(t.blocks == 0);
    pf_assert(t.bytes == 0);
    pf_assert(t.badSizes == 0);
    return 0;
}

TEST(initialized_map_releases_everything) {
    BEGIN;
    TRACKER(t, -1);
    map_t raw;
    imap m = mapinit(int, &raw, hash_int, &t.alloc);
    for (int i = 0; i < 500; i++)
        pf_assert_ok(setk(m, i));
    mapfree(m);
    pf_assert(t.blocks == 0);
    pf_assert(t.bytes == 0);
    pf_assert(t.badSizes == 0);
    return 0;
}

TEST(out_of_memory_during_growth_keeps_the_map_intact) {
    BEGIN;
    TRACKER(t, -1);
    pmap m = mapnew(struct pr, hash_pair, &t.alloc);
    for (int i = 0; i < 10; i++)
        pf_assert_ok(setp(m, i, i * 5));

    t.budget = 0;
    int stored = 10;
    int rc = ITER_OK;
    for (int i = 10; i < 10000 && rc == ITER_OK; i++) {
        rc = setp(m, i, i * 5);
        if (rc == ITER_OK)
            stored++;
    }

    /* the table has to run out of room at some point, and say so */
    pf_assert(rc == ITER_ENOMEM);
    pf_assert(mapcount(m) == (size_t)stored);
    for (int i = 0; i < stored; i++)
        pf_assert(valp(m, i) == i * 5);
    pf_assert(!hasp(m, stored));

    /* once memory is back the map keeps working */
    t.budget = -1;
    pf_assert_ok(setp(m, stored, stored * 5));
    pf_assert(valp(m, stored) == stored * 5);

    mapfree(m);
    pf_assert(t.blocks == 0);
    pf_assert(t.badSizes == 0);
    return 0;
}

TEST(everything_is_released_after_failures) {
    BEGIN;
    for (long budget = 0; budget < 6; budget++) {
        TRACKER(t, budget);
        imap m = mapnew(int, hash_int, &t.alloc);
        if (m) {
            for (int i = 0; i < 200; i++) {
                if (setk(m, i))
                    break;
            }
            mapfree(m);
        }
        pf_assert(t.blocks == 0);
        pf_assert(t.bytes == 0);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Randomized comparison against a reference table                    */
/* ------------------------------------------------------------------ */

#define KEYS 48

struct model {
    int present[KEYS];
    int value[KEYS];
    size_t count;
};

static int each_check(void *item, void *user) {
    struct each_ctx *c = user;
    return each_record(item, c);
}

TEST(random_operations_match_a_reference) {
    BEGIN;
    for (int round = 0; round < 6; round++) {
        uint32_t rs = rng_seed(seed, rep, round);
        TRACKER(t, -1);
        pmap m = mapnew(struct pr, hash_pair, &t.alloc);
        struct model ref;
        memset(&ref, 0, sizeof ref);

        for (int step = 0; step < 3000; step++) {
            int op = (int)(rng_next(&rs) % 13);
            int k = (int)(rng_next(&rs) % KEYS);
            int v = step + 1;
            struct pr p = { k, v }, out = { k, -1 };

            switch (op) {
            case 0:
            case 1:
            case 2: /* set */
                pf_assert_ok(mapset(m, &p));
                ref.count += !ref.present[k];
                ref.present[k] = 1;
                ref.value[k] = v;
                break;
            case 3: /* insert */
                if (ref.present[k]) {
                    pf_assert(mapinsert(m, &p) == ITER_EEXIST);
                } else {
                    pf_assert_ok(mapinsert(m, &p));
                    ref.present[k] = 1;
                    ref.value[k] = v;
                    ref.count++;
                }
                break;
            case 4: /* update */
                if (ref.present[k]) {
                    pf_assert_ok(mapupdate(m, &p));
                    ref.value[k] = v;
                } else {
                    pf_assert(mapupdate(m, &p) == ITER_ENOENT);
                }
                break;
            case 5:
            case 6:
            case 7: /* remove */
                if (ref.present[k]) {
                    pf_assert_ok(mapremove(m, &p, &out));
                    pf_assert(out.key == k && out.val == ref.value[k]);
                    ref.present[k] = 0;
                    ref.count--;
                } else {
                    pf_assert(mapremove(m, &p, &out) == ITER_ENOENT);
                    pf_assert(out.val == -1);
                }
                break;
            case 8:
            case 9: /* get */
                if (ref.present[k]) {
                    pf_assert_ok(mapget(m, &p, &out));
                    pf_assert(out.key == k && out.val == ref.value[k]);
                } else {
                    pf_assert(mapget(m, &p, &out) == ITER_ENOENT);
                }
                break;
            case 10: { /* ptr */
                struct pr *found = mapptr(m, &p);
                if (ref.present[k]) {
                    pf_assert_not_null(found);
                    pf_assert(found->key == k && found->val == ref.value[k]);
                } else {
                    pf_assert_null(found);
                }
                break;
            }
            case 11: /* reserve */
                pf_assert_ok(mapreserve(m, 1 + rng_next(&rs) % 40));
                break;
            default: { /* each */
                struct each_ctx c;
                memset(&c, 0, sizeof c);
                pf_assert(mapeach(m, each_check, &c) == ITER_OK);
                pf_assert(c.calls == ref.count);
                for (int i = 0; i < KEYS; i++)
                    pf_assert(c.seen[i] == ref.present[i]);
                break;
            }
            }

            pf_assert(mapcount(m) == ref.count);
            pf_assert(mapisempty(m) == (ref.count == 0));
        }

        for (int i = 0; i < KEYS; i++)
            pf_assert(valp(m, i) == (ref.present[i] ? ref.value[i] : -1));

        mapfree(m);
        pf_assert(t.blocks == 0);
        pf_assert(t.badSizes == 0);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Suite                                                              */
/* ------------------------------------------------------------------ */

#define T(name) { test_##name, #name }

/* clang-format off */
pf_test_t suite_map[] = {
    /* creation and destruction */
    T(new_is_empty),
    T(new_uses_global_allocator),
    T(new_uses_given_allocator),
    T(new_allocation_failure),
    T(new_invalid_arguments),
    T(init_uses_caller_storage),
    T(init_invalid_arguments),
    T(free_null_is_noop),
    T(free_releases_everything),
    T(null_map_is_safe_to_query),
    T(null_map_operations_fail),
    T(null_key_operations_fail),

    /* reserve */
    T(reserve_makes_room),
    T(reserve_zero_is_invalid),
    T(reserve_avoids_later_reallocation),
    T(reserve_keeps_existing_items),
    T(reserve_never_shrinks),
    T(reserve_out_of_memory_changes_nothing),

    /* empty map */
    T(empty_map_has_no_items),
    T(empty_map_iteration),
    T(reserved_but_empty_map_has_no_items),

    /* set, insert, update */
    T(set_adds_new_items),
    T(set_overwrites_existing_items),
    T(set_copies_the_item),
    T(set_out_of_memory_changes_nothing),
    T(insert_adds_new_items),
    T(insert_refuses_duplicates),
    T(insert_out_of_memory_changes_nothing),
    T(update_replaces_existing_items),
    T(update_never_adds_items),
    T(update_does_not_touch_removed_items),

    /* get, ptr */
    T(get_copies_the_stored_item),
    T(get_missing_leaves_out_untouched),
    T(get_without_out_checks_existence),
    T(get_allows_key_and_out_to_alias),
    T(get_returns_a_copy),
    T(ptr_gives_access_to_the_stored_item),
    T(ptr_of_missing_item_is_null),
    T(ptr_is_stable_while_capacity_suffices),

    /* remove */
    T(remove_copies_the_item_out),
    T(remove_without_out),
    T(remove_allows_key_and_out_to_alias),
    T(remove_missing_item),
    T(remove_twice),
    T(removed_items_can_be_inserted_again),
    T(remove_keeps_the_other_items),
    T(map_is_reusable_after_removing_everything),

    /* iteration */
    T(each_visits_every_item_exactly_once),
    T(each_stops_when_callback_returns_non_zero),
    T(each_can_modify_values),
    T(each_skips_removed_items),
    T(each_on_a_map_emptied_by_removals),
    T(each_invalid_arguments),

    /* growth, churn, collisions */
    T(many_insertions),
    T(growth_is_proportional_to_the_item_count),
    T(growth_is_amortized),
    T(growth_keeps_all_values),
    T(insert_and_remove_churn),
    T(churn_does_not_grow_memory_without_bound),
    T(all_keys_colliding),
    T(all_keys_colliding_with_removals),
    T(keys_at_integer_limits),
    T(hash_bytes_helper_works_as_a_hash_function),

    /* maphash */
    T(maphash_is_deterministic),
    T(maphash_depends_on_content_and_length),
    T(maphash_only_reads_the_given_size),
    T(maphash_spreads_integers),

    /* item types */
    T(string_keys),
    T(items_respect_their_alignment),
    T(large_items),

    /* memory behaviour */
    T(everything_is_released_after_heavy_use),
    T(initialized_map_releases_everything),
    T(out_of_memory_during_growth_keeps_the_map_intact),
    T(everything_is_released_after_failures),

    /* randomized */
    T(random_operations_match_a_reference),

    { 0 }
};
/* clang-format on */
