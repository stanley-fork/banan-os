#pragma once
// Copyright (c) 2026 Dcraftbg
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// [ 8 bit major ] [ 8 minor ] [ 16 patch]
typedef unsigned int banos_version_t;
#define BANOS_VERSION_MAJOR_SHIFT 24
#define BANOS_VERSION_MINOR_SHIFT 16
#define BANOS_VERSION_PATCH_SHIFT 0

#define BANOS_VERSION_MAJOR_MASK 0xFF
#define BANOS_VERSION_MINOR_MASK 0xFF
#define BANOS_VERSION_PATCH_MASK 0xFFFF

#define BANOS_VERSION_MAKE(major, minor, patch) \
    (banos_version_t)( \
        (((major) & BANOS_VERSION_MAJOR_MASK) << BANOS_VERSION_MAJOR_SHIFT) | \
        (((minor) & BANOS_VERSION_MINOR_MASK) << BANOS_VERSION_MINOR_SHIFT) | \
        (((patch) & BANOS_VERSION_PATCH_MASK) << BANOS_VERSION_PATCH_SHIFT) \
    )

#define BANOS_VERSION_CURRENT BANOS_VERSION_MAKE(0, 0, 1)

#define BANOS_VERSION_GET_MAJOR(v) (((v) >> BANOS_VERSION_MAJOR_SHIFT) & BANOS_VERSION_MAJOR_MASK)
#define BANOS_VERSION_GET_MINOR(v) (((v) >> BANOS_VERSION_MINOR_SHIFT) & BANOS_VERSION_MINOR_MASK)
#define BANOS_VERSION_GET_PATCH(v) (((v) >> BANOS_VERSION_PATCH_SHIFT) & BANOS_VERSION_PATCH_MASK)
