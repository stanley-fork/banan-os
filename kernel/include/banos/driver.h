#pragma once
// Copyright (c) 2026 Dcraftbg
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#include "version.h"
#include "revision.h"

#define BANOS_DRIVER_REVISION_CURRENT 0

typedef struct Banos_Driver Banos_Driver;
struct Banos_Driver {
    unsigned long driver_size;
    banos_version_t minimal_banos_version;
    const char* name;
    const char* license;
    banos_version_t version;

    // NOTE: checkout BANOS_DRIVER_INSTANCE_SIZE.
    // You may use this instance data for anything you wish to store.
    // If you need more than that just allocate it on the heap or
    // globally if you add the proper verification of having your driver run only
    // within a single instance
    int (*init)(Banos_Driver* drv);
    int (*uninit)(Banos_Driver* drv);
};
#define BANOS_DRIVER_API static __attribute__((section(".banos-driver"), used, aligned(8)))
