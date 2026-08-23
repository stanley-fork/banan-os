#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H 1

#include <sys/cdefs.h>

__BEGIN_DECLS

#include <stdint.h>

#define BANAN_FB_BPP 32

struct framebuffer_info_t
{
	uint32_t width;
	uint32_t height;
};

struct fb_fix_info
{
	uint32_t xpanstep;
	uint32_t ypanstep;
	uint32_t pitch;
	uint64_t mem_start;
	uint32_t mem_size;
};
struct fb_var_info
{
	uint32_t xres;
	uint32_t yres;
	uint32_t xres_virt;
	uint32_t yres_virt;
	uint32_t xoff;
	uint32_t yoff;
	uint32_t bpp;
};

__END_DECLS

#endif
