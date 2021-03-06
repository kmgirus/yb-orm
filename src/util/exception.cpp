// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBUTIL_SOURCE

#include "util/exception.h"
#include <sstream>
#include "stack_trace.h"

namespace Yb {

const String BaseError::format_base(const String &msg)
{
    std::ostringstream out;
    print_stacktrace(out, 100, 2);
    if (out.str().compare("<stack trace not implemented>\n") != 0)
        return msg + _T("\nBacktrace:\n") + WIDEN(out.str());
    return msg;
}

BaseError::BaseError(const String &msg)
    : std::logic_error(NARROW(format_base(msg)))
{}

const String AssertError::format_assert(const char *file, int line,
        const char *expr)
{
    std::ostringstream out;
    out << "Assertion failed in file " << file << ":" << line
        << " (" << expr << ")";
    return WIDEN(out.str());
}

AssertError::AssertError(const char *file, int line, const char *expr)
    : BaseError(format_assert(file, line, expr))
{}

RunTimeError::RunTimeError(const String &msg)
    : BaseError(msg)
{}

KeyError::KeyError(const String &msg)
    : RunTimeError(msg)
{}

ValueError::ValueError(const String &msg)
    : RunTimeError(msg)
{}

ValueBadCast::ValueBadCast(const String &value, const String &type)
    : ValueError(_T("Can't cast value '") + value +
                 _T("' to type ") + type)
{}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
