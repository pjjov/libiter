/*  libiter - Generic container and iterator library for C.

    Copyright 2025-2026 Предраг Јовановић
    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0
*/

#include <pf_test.h>

extern pf_test_t suite_vec[];
extern pf_test_t suite_map[];

int main(int argc, char *argv[]) {
    static const pf_suite_t suites[] = {
        { "vec", 1, suite_vec },
        { "map", 1, suite_map },
        { 0 },
    };

    return pf_test_main(argc, argv, suites);
}
