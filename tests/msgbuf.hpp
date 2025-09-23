/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
#ifndef PLDM_MSGBUF_HPP
#define PLDM_MSGBUF_HPP

#include <libpldm/compiler.h>

#include "msgbuf/core.h"

/*
 * Use the C++ Function Overload to keep pldm_msgbuf related function consistent
 * and to produce compile-time errors when the wrong pldm_msgbuf type is passed.
 *
 * Previously we cast away `const` in `pldm_msgbuf_init_error()`, which was a
 * hack. Instead, introduce:
 *   - pldm_msgbuf_ro: read-only buffer with a `const` cursor
 *   - pldm_msgbuf_rw: read-write buffer with a non-const cursor
 *
 * `pldm_msgbuf_ro` is used by decode APIs to extract payloads into PLDM
 * structures. `pldm_msgbuf_rw` is used by encode APIs to insert payloads from
 * PLDM structures.
 */

#include <cstdint>
#include <cstdio>
#include <type_traits>

// NOLINTBEGIN(bugprone-macro-parentheses)
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#define PLDM__MSGBUF_DEFINE_P(name, mode)                                      \
    struct pldm_msgbuf_##mode _##name LIBPLDM_CC_CLEANUP(                      \
        pldm__msgbuf_##mode##_cleanup) = {NULL, INTMAX_MIN};                   \
    auto* name = &(_##name)
// NOLINTEND(bugprone-macro-parentheses)

#define PLDM_MSGBUF_RO_DEFINE_P(name) PLDM__MSGBUF_DEFINE_P(name, ro)
#define PLDM_MSGBUF_RW_DEFINE_P(name) PLDM__MSGBUF_DEFINE_P(name, rw)

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_init_errno(struct pldm_msgbuf_ro* ctx,
                                                    size_t minsize,
                                                    const void* buf, size_t len)
{
    return pldm_msgbuf_ro_init_errno(ctx, minsize, buf, len);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_init_errno(struct pldm_msgbuf_rw* ctx,
                                                    size_t minsize,
                                                    const void* buf, size_t len)
{
    return pldm_msgbuf_rw_init_errno(ctx, minsize, buf, len);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_validate(struct pldm_msgbuf_ro* ctx)
{
    return pldm_msgbuf_ro_validate(ctx);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_validate(struct pldm_msgbuf_rw* ctx)
{
    return pldm_msgbuf_rw_validate(ctx);
}

// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
LIBPLDM_CC_ALWAYS_INLINE int pldm__msgbuf_invalidate(struct pldm_msgbuf_ro* ctx)
{
    return pldm__msgbuf_ro_invalidate(ctx);
}

// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
LIBPLDM_CC_ALWAYS_INLINE int pldm__msgbuf_invalidate(struct pldm_msgbuf_rw* ctx)
{
    return pldm__msgbuf_rw_invalidate(ctx);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_consumed(struct pldm_msgbuf_ro* ctx)
{
    return pldm_msgbuf_ro_consumed(ctx);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_consumed(struct pldm_msgbuf_rw* ctx)
{
    return pldm_msgbuf_rw_consumed(ctx);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_discard(struct pldm_msgbuf_ro* ctx,
                                                 int error)
{
    return pldm_msgbuf_ro_discard(ctx, error);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_discard(struct pldm_msgbuf_rw* ctx,
                                                 int error)
{
    return pldm_msgbuf_rw_discard(ctx, error);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_complete(struct pldm_msgbuf_ro* ctx)
{
    return pldm_msgbuf_ro_complete(ctx);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_complete(struct pldm_msgbuf_rw* ctx)
{
    return pldm_msgbuf_rw_complete(ctx);
}

LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_complete_consumed(struct pldm_msgbuf_ro* ctx)
{
    return pldm_msgbuf_ro_complete_consumed(ctx);
}

LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_complete_consumed(struct pldm_msgbuf_rw* ctx)
{
    return pldm_msgbuf_rw_complete_consumed(ctx);
}

LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_span_required(struct pldm_msgbuf_ro* ctx, size_t required,
                              const void** cursor)
{
    return pldm_msgbuf_ro_span_required(ctx, required, cursor);
}

LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_span_required(struct pldm_msgbuf_rw* ctx, size_t required,
                              void** cursor)
{
    return pldm_msgbuf_rw_span_required(ctx, required, cursor);
}

LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_span_string_ascii(struct pldm_msgbuf_rw* ctx, void** cursor,
                                  size_t* length)
{
    return pldm_msgbuf_rw_span_string_ascii(ctx, cursor, length);
}

LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_span_string_ascii(struct pldm_msgbuf_ro* ctx,
                                  const void** cursor, size_t* length)
{
    return pldm_msgbuf_ro_span_string_ascii(ctx, cursor, length);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_span_string_utf16(pldm_msgbuf_ro* ctx,
                                                           const void** cursor,
                                                           size_t* length)
{
    return pldm_msgbuf_ro_span_string_utf16(ctx, cursor, length);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_span_string_utf16(pldm_msgbuf_rw* ctx,
                                                           void** cursor,
                                                           size_t* length)
{
    return pldm_msgbuf_rw_span_string_utf16(ctx, cursor, length);
}

LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_span_remaining(struct pldm_msgbuf_rw* ctx, void** cursor,
                               size_t* len)
{
    return pldm_msgbuf_rw_span_remaining(ctx, cursor, len);
}

LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_span_remaining(struct pldm_msgbuf_ro* ctx, const void** cursor,
                               size_t* len)
{
    return pldm_msgbuf_ro_span_remaining(ctx, cursor, len);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_span_until(struct pldm_msgbuf_ro* ctx,
                                                    size_t trailer,
                                                    const void** cursor,
                                                    size_t* length)
{
    return pldm_msgbuf_ro_span_until(ctx, trailer, cursor, length);
}

LIBPLDM_CC_ALWAYS_INLINE int pldm_msgbuf_span_until(struct pldm_msgbuf_rw* ctx,
                                                    size_t trailer,
                                                    void** cursor,
                                                    size_t* length)
{
    return pldm_msgbuf_rw_span_until(ctx, trailer, cursor, length);
}

#define pldm_msgbuf_extract_typecheck(ty, fn, dst, ...)                        \
    pldm_msgbuf_typecheck_##ty<decltype(dst)>(__VA_ARGS__)

#define pldm_msgbuf_extract_uint8(ctx, dst)                                    \
    pldm_msgbuf_extract_typecheck(uint8_t, pldm__msgbuf_extract_uint8, dst,    \
                                  ctx, (void*)&(dst))

#define pldm_msgbuf_extract_int8(ctx, dst)                                     \
    pldm_msgbuf_extract_typecheck(int8_t, pldm__msgbuf_extract_int8, dst, ctx, \
                                  (void*)&(dst))

#define pldm_msgbuf_extract_uint16(ctx, dst)                                   \
    pldm_msgbuf_extract_typecheck(uint16_t, pldm__msgbuf_extract_uint16, dst,  \
                                  ctx, (void*)&(dst))

#define pldm_msgbuf_extract_int16(ctx, dst)                                    \
    pldm_msgbuf_extract_typecheck(int16_t, pldm__msgbuf_extract_int16, dst,    \
                                  ctx, (void*)&(dst))

#define pldm_msgbuf_extract_uint32(ctx, dst)                                   \
    pldm_msgbuf_extract_typecheck(uint32_t, pldm__msgbuf_extract_uint32, dst,  \
                                  ctx, (void*)&(dst))

#define pldm_msgbuf_extract_int32(ctx, dst)                                    \
    pldm_msgbuf_extract_typecheck(int32_t, pldm__msgbuf_extract_int32, dst,    \
                                  ctx, (void*)&(dst))

#define pldm_msgbuf_extract_real32(ctx, dst)                                   \
    pldm_msgbuf_extract_typecheck(real32_t, pldm__msgbuf_extract_real32, dst,  \
                                  ctx, (void*)&(dst))

template <typename T>
LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_typecheck_uint8_t(struct pldm_msgbuf_ro* ctx, void* buf)
{
    static_assert(std::is_same<uint8_t, T>::value);
    return pldm__msgbuf_extract_uint8(ctx, buf);
}

template <typename T>
LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_typecheck_int8_t(struct pldm_msgbuf_ro* ctx, void* buf)
{
    static_assert(std::is_same<int8_t, T>::value);
    return pldm__msgbuf_extract_int8(ctx, buf);
}

template <typename T>
LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_typecheck_uint16_t(struct pldm_msgbuf_ro* ctx, void* buf)
{
    static_assert(std::is_same<uint16_t, T>::value);
    return pldm__msgbuf_extract_uint16(ctx, buf);
}

template <typename T>
LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_typecheck_int16_t(struct pldm_msgbuf_ro* ctx, void* buf)
{
    static_assert(std::is_same<int16_t, T>::value);
    return pldm__msgbuf_extract_int16(ctx, buf);
}

template <typename T>
LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_typecheck_uint32_t(struct pldm_msgbuf_ro* ctx, void* buf)
{
    static_assert(std::is_same<uint32_t, T>::value);
    return pldm__msgbuf_extract_uint32(ctx, buf);
}

template <typename T>
LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_typecheck_int32_t(struct pldm_msgbuf_ro* ctx, void* buf)
{
    static_assert(std::is_same<int32_t, T>::value);
    return pldm__msgbuf_extract_int32(ctx, buf);
}

template <typename T>
LIBPLDM_CC_ALWAYS_INLINE int
    pldm_msgbuf_typecheck_real32_t(struct pldm_msgbuf_ro* ctx, void* buf)
{
    static_assert(std::is_same<real32_t, T>::value);
    return pldm__msgbuf_extract_real32(ctx, buf);
}

#endif /* BUF_HPP */
