#pragma once

// Platform compatibility layer for Unix-specific APIs.
// Currently supports macOS and Linux. Windows support is not yet implemented.

#ifdef _WIN32
    #error "Windows is not yet supported. See README.md for platform requirements."
#endif

#include <sys/ioctl.h>
#include <unistd.h>

// gmtime_r and timegm are available on macOS and Linux.
// On Windows, use _gmtime64_s and _mkgmtime64 respectively.
