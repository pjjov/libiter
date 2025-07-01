/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <pf_test.h>
#include <string.h>

extern pf_test suite_hashmap[];
extern pf_test suite_iter[];
extern pf_test suite_vector[];

static const pf_test *suites[] = {
    suite_hashmap,
    suite_iter,
    suite_vector,
    NULL,
};

static const char *names[] = { "hashmap", "iter", "vector", NULL };

int main(int argc, char *argv[]) {
    if (argc > 2) {
        fputs("usage: libiter-test [suite]\nsuites:", stderr);

        for (int i = 0; names[i]; i++) {
            fputc(' ', stderr);
            fputs(names[i], stderr);
        }

        fputc('\n', stderr);
        return -1;
    }

    if (argc == 1)
        return pf_suite_run_all(suites, 0, NULL);

    for (int i = 0; names[i]; i++) {
        if (0 == strcmp(argv[1], names[i])) {
            pf_suite_run_tap(suites[i], 0, NULL);
            return 0;
        }
    }

    fprintf(stderr, "libiter-test: unknown suite '%s'\n", argv[1]);
    return -1;
}
