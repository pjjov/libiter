/** Polyfill for C - Filling the gaps between C standards and compilers

    This file provides type utilities and checks from newer C standards
    and compiler extensions. Currently implemented standard features are:
    - alignof, alignas, typeof, typeof_unqual and offsetof.

    Compiler extensions and utilities that are also available are:
    - pf_container_of
    - pf_is_const, pf_if_const, pf_if_not_const
    - pf_is_type, pf_if_type, pf_if_not_type
    - pf_check_type, pf_check_const
    - PF_ALIGN_PAD, PF_ALIGN_DOWN, PF_ALIGN_UP

    If a feature is missing 'PF_NO_*feature*' will be defined and a
    warning can be emitted (by defining 'PF_WARN'). You can also define
    'PF_NO_*feature*' to disable the missing feature.

    SPDX-FileCopyrightText: 2025-2026 Предраг Јовановић
    SPDX-License-Identifier: Apache-2.0

    Copyright 2025-2026 Предраг Јовановић

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
**/

#ifndef POLYFILL_TYPES
#define POLYFILL_TYPES

#ifndef pf_has_builtin
    #ifdef __has_builtin
        #define pf_has_builtin(x) __has_builtin(x)
    #else
        #define pf_has_builtin(x) 0
    #endif
#endif

#ifndef pf_has_gnu_attribute
    #ifdef __has_attribute
        #define pf_has_gnu_attribute(x) __has_attribute(x)
    #else
        #define pf_has_gnu_attribute(x) 0
    #endif
#endif

#if __STDC_VERSION__ >= 201112L
    #include <stdalign.h>
#endif

#ifndef PF_HAS_STMT_EXPR
    #if !__STRICT_ANSI__ && __GNUC__ >= 3
        #define PF_HAS_STMT_EXPR
    #endif
#endif

#if !defined(PF_NO_ALIGNOF) && !defined(alignof)                    \
    && !defined(__alignof_is_defined) && __STDC_VERSION__ < 202311L

    #if __STDC_VERSION__ >= 201112L
        #define alignof _Alignof
    #elif defined(__alignof__)
        #define alignof __alignof__
    #else
        #ifdef PF_WARN
            #warning "Polyfill for 'alignof' is not available."
        #endif
        #define PF_NO_ALIGNOF
        #define alignof sizeof
    #endif

    #if __STDC_VERSION__ < 201112L
        #define _Alignof alignof
    #endif
#endif

#if !defined(PF_NO_ALIGNAS) && !defined(alignas)                    \
    && !defined(__alignas_is_defined) && __STDC_VERSION__ < 202311L

    #if __STDC_VERSION__ >= 201112L
        #define alignas _Alignas
    #elif pf_has_gnu_attribute(aligned)
        /* TODO: fix passing a type */
        #define alignas(...) __attribute__((aligned(__VA_ARGS__)))
    #else
        #ifdef PF_WARN
            #warning "Polyfill for 'alignas' is not available."
        #endif
        #define PF_NO_ALIGNAS
    #endif
#endif

#ifdef __BIGGEST_ALIGNMENT__
typedef struct {
    alignas(__BIGGEST_ALIGNMENT__) char _pf_;
} pf_max_align_t;
#else
typedef long double pf_max_align_t;
#endif

#if __STDC_VERSION__ < 201112L
typedef pf_max_align_t max_align_t;
#endif

#if !defined(PF_NO_TYPEOF) && !defined(typeof) && __STDC_VERSION__ < 202311L
    #if defined(__cplusplus) && (__cpp_decltype >= 200707L || _MSC_VER >= 1600)
        #define typeof decltype
    #elif defined(__MCST__) || defined(__GNUC__) || defined(__clang__) \
        || defined(__chibicc__)
        #define typeof __typeof__
    #else
        #ifdef PF_WARN
            #warning "Polyfill for 'typeof' is not available."
        #endif
        #define PF_NO_TYPEOF
    #endif
#endif

#if !defined(PF_NO_TYPEOF_UNQUAL) && !defined(typeof_unqual) \
    && __STDC_VERSION__ < 202311L
    #if defined(__typeof_unqual__)
        #define typeof_unqual(expr) __typeof_unqual__(expr)
    #else
        #ifdef PF_WARN
            #warning "Polyfill for 'typeof_unqual' is not available."
        #endif
        #define PF_NO_TYPEOF_UNQUAL
    #endif
#endif

#if !defined(PF_NO_OFFSETOF) && !defined(offsetof) && !defined(__STDC__)
    #define offsetof(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)
#endif

#if !defined(pf_container_of) && !defined(PF_NO_CONTAINEROF)
    #ifdef PF_HAS_STMT_EXPR
        #define pf_container_of(ptr, type, member)                 \
            ({                                                     \
                const typeof(((type *)0)->member) *__mptr = (ptr); \
                (type *)((char *)__mptr - offsetof(type, member)); \
            })
    #else
        #define pf_container_of(ptr, type, member)               \
            ((type *)((char *)(1 ? (ptr) : &((type *)0)->member) \
                      - offsetof(type, member)))
    #endif
#endif

#if !defined(PF_NO_IS_CONST) && !defined(pf_is_const)
    #if pf_has_builtin(__builtin_types_compatible_p)
        #define pf_is_const(type)                                              \
            __builtin_types_compatible_p(const typeof(type) *, typeof(type) *)
    #else
        #define PF_NO_IS_CONST
    #endif
#endif

#if pf_has_builtin(__builtin_choose_expr) && !defined(PF_NO_IS_CONST)
    #define pf_if_const(type, exp1, exp2)                        \
        __builtin_choose_expr(pf_is_const(type), (exp1), (exp2))
    #define pf_if_not_const(type, exp1, exp2)                     \
        __builtin_choose_expr(!pf_is_const(type), (exp1), (exp2))
#else
    #define pf_if_const(type, exp1, exp2) (exp1)
    #define pf_if_not_const(type, exp1, exp2) (exp2)
#endif

#if !defined(PF_NO_IS_TYPE) && !defined(pf_is_type)
    #if pf_has_builtin(__builtin_types_compatible_p)
        #define pf_is_type(type, expr)                           \
            __builtin_types_compatible_p(type *, typeof(expr) *)
    #else
        #define PF_NO_IS_TYPE
    #endif
#endif

#if pf_has_builtin(__builtin_choose_expr) && !defined(PF_NO_IS_TYPE)
    #define pf_if_type(type, exp1, exp2)                        \
        __builtin_choose_expr(pf_is_type(type), (exp1), (exp2))
    #define pf_if_not_type(type, exp1, exp2)                     \
        __builtin_choose_expr(!pf_is_type(type), (exp1), (exp2))
#else
    #define pf_if_type(type, exp1, exp2) (exp1)
    #define pf_if_not_type(type, exp1, exp2) (exp2)
#endif

#if !defined(pf_check_type) && !defined(PF_NO_CHECK_TYPE)
    #ifdef PF_HAS_STMT_EXPR
        #if !defined(PF_NO_IS_TYPE)                                     \
            && (defined(_Static_assert) || __STDC_VERSION__ >= 201112L)
            #define pf_check_type(type, expr)                            \
                ({                                                       \
                    _Static_assert(                                      \
                        pf_is_type(type, (expr)),                        \
                        "Incompatible type passed to 'pf_check_type': "  \
                        "expected '" #expr "' to be of type '" #type "'" \
                    );                                                   \
                    (expr);                                              \
                })
        #elif !defined(PF_NO_IS_TYPE) && pf_has_builtin(__builtin_choose_expr)
            #define pf_check_type(type, expr) \
                __builtin_choose_expr(pf_is_type(type, (expr)), (expr), (void)0)
        #else
            #define pf_check_type(type, expr) \
                ({                            \
                    type pf__tmp = (expr);    \
                    pf__tmp;                  \
                })
        #endif
    #else
        #define pf_check_type(type, expr)                           \
            (1 ? (expr) : ((struct { type pf__tmp; } *)0)->pf__tmp)
    #endif
#endif

#define pf_check_const(expr) pf_check_type(typeof_unqual(expr), expr)

#ifndef PF__ALIGN
    #define PF__ALIGN
    #define PF_ALIGN_PAD(x, a) ((~(x) + 1) & ((a) - 1))
    #define PF_ALIGN_DOWN(x, a) ((x) & ~((typeof(x))(a) - 1))
    #define PF_ALIGN_UP(x, a)                                  \
        (((x) + ((typeof(x))(a) - 1)) & ~((typeof(x))(a) - 1))

    #define PF_ALIGN_CEIL PF_ALIGN_UP
    #define PF_ALIGN_FLOOR PF_ALIGN_DOWN
#endif

#endif
