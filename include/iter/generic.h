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

#ifdef PF_HAS_STMT_EXPR
    #define generic_ret_key(m_type, m_inst, m_ret) \
        ({                                         \
            m_type instance = (m_inst);            \
            (void)instance;                        \
            (m_ret);                               \
        })
#else
    #define generic_ret_key(m_type, m_inst, m_ret) (m_ret)
#endif

#define generic_check_key(m_cntr, m_key, m_inst)                         \
    generic_ret_key(                                                     \
        generic_container(                                               \
            m_cntr, typeof(*(m_key)), generic_value_type(m_cntr, m_inst) \
        ),                                                               \
        (m_inst),                                                        \
        (m_key)                                                          \
    )

/* placed here to reduce the number of header files included. */
#define iter(T) generic_container(iter_t, size_t, T)
typedef struct iter_t iter_t;

#define iter_type(m_iter) generic_value_type(iter_t, m_iter)
#define iter_type_ptr(m_iter) generic_value_ptr(iter_t, m_iter)
#define iter_type_size(m_iter) generic_value_size(iter_t, m_iter)
#define iter_as_base(m_iter) generic_check_container(iter_t, size_t, m_iter)
#define iter_check_type(m_iter, m_item)         \
    generic_check_value(iter_t, m_iter, m_item)

#endif
