/*  libiter - Generic container and iterator library for C.

    Copyright 2025 Predrag Jovanović
    SPDX-FileCopyrightText: 2025 Predrag Jovanović
    SPDX-License-Identifier: Apache-2.0
*/

#ifndef LIBITER_GENERIC_H
#define LIBITER_GENERIC_H

#include <pf_types.h>

/** # Generic Containers

    **generic.h** defines macros used for implementing generic containers
    The macros themselves rely on heavily on the polyfill library.
**/

#define generic_container(m_cntr, m_key, m_value)         \
    typeof(m_value * (**)(m_cntr * container, m_key key))

#define generic_value_type(m_type, m_cntr) typeof(*(*(m_cntr))((m_type *)0, 0))
#define generic_value_ptr(m_type, m_cntr) typeof((*(m_cntr))((m_type *)0, 0))
#define generic_value_size(m_type, m_cntr)     \
    sizeof(generic_value_type(m_type, m_cntr))

#define generic_check_value(m_type, m_cntr, m_value)          \
    pf_check_type(generic_value_ptr(m_type, m_cntr), m_value)

#define generic_check_container(m_type, m_key, m_cntr)                        \
    ((m_type *)pf_check_type(                                                 \
        generic_container(m_type, m_key, generic_value_type(m_type, m_cntr)), \
        m_cntr                                                                \
    ))

#endif
