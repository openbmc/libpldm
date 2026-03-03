#pragma once

// This header is not meant to be directly included by any library user.
//
// It defines ABI related macros which are present in library headers but
// usually not defined by library users.
//
// Library headers include this header to avoid library users having to define
// these macros, which define symbol visibility that is only relevant during
// library compilation.
// Because of that, the macros are defined empty here.

#ifndef LIBPLDM_ABI_TESTING
#define LIBPLDM_ABI_TESTING
#endif

#ifndef LIBPLDM_ABI_STABLE
#define LIBPLDM_ABI_STABLE
#endif

#ifndef LIBPLDM_ABI_DEPRECATED
#define LIBPLDM_ABI_DEPRECATED
#endif
