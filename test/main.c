/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#include <pf_test.h>
#include <string.h>

extern pf_test suite_vector[];

const pf_test *suites[] = {
    suite_vector,
    NULL,
};

int main(int argc, char *argv[]) {
    if (argc > 2) {
        perror(
            "usage: libiter-test [suite]\n"
            "suites:\n"
            "\tvector\n"
        );
        return -1;
    } else if (argc == 1)
        return pf_suite_run_all(suites, 0);
    else if (0 == strcmp(argv[1], "vector"))
        return pf_suite_run(suite_vector, 0);
    else {
        fprintf(stderr, "libiter-test: unknown suite '%s'\n", argv[1]);
        return -1;
    }
}
