#pragma once

#include <assert.h>
#include "f2f/FilesystemError.hpp"

namespace f2f { namespace detail 
{

[[noreturn]]
inline void ThrowFilesystemError(ErrorCode code, const char * description)
{
  //assert(false);
  throw FileSystemError(code, description);
}

}}

#define F2F_STRINGIFY(s) #s

#define F2F_FORMAT_ASSERT(expression) \
  (void)((!!(expression)) || (f2f::detail::ThrowFilesystemError(ErrorCode::InvalidStorageFormat, \
    "Invalid storage format at " __FILE__ " (" F2F_STRINGIFY(__LINE__) ")"), false))

#define F2F_ASSERT(expression) \
  (void)((!!(expression)) || (f2f::detail::ThrowFilesystemError(ErrorCode::InternalExpectationFail, \
    "Internal expectation fail at " __FILE__ " (" F2F_STRINGIFY(__LINE__) ")"), false))
