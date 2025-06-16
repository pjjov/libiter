/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_ERROR_H
#define LIBITER_ERROR_H

/** # Error codes

    For most functions that return `int`, `0` is returned on success and
    negated values of POSIX error codes otherwise.

    You should interpret returned values as:

    - `ITER_OK` - Function call succeeded.
    - `ITER_EINVAL` - Invalid parameter values passed.
    - `ITER_ENOMEM` - Allocation error occured, e.g. out of memory.
    - `ITER_EEXIST` - Key or value already exists.
    - `ITER_ENOENT` - Key or value doesn't exist.
    - `ITER_ENODATA` - No more items available.
    - `ITER_ENOSYS` - Feature is not available.

    Alongside these, boolean values `ITER_TRUE` and `ITER_FALSE` are defined.
**/

enum {
    ITER_TRUE = 1,
    ITER_FALSE = 0,
};

enum {
    ITER_OK = 0,
    ITER_ENOENT = -2,
    ITER_EINTR = -4,
    ITER_ENOMEM = -12,
    ITER_EEXIST = -17,
    ITER_EINVAL = -22,
    ITER_ENOSYS = -38,
    ITER_ENODATA = -61,
};

#endif
