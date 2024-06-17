#include <LibImage/Image.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/framebuffer.h>
#include <sys/mman.h>
#include <unistd.h>

void render_to_framebuffer(BAN::UniqPtr<LibImage::Image> image, bool scale)
{
	int fd = open("/dev/fb0", O_RDWR);
	if (fd == -1)
	{
		perror("open");
		exit(1);
	}

	framebuffer_info_t fb_info;
	if (pread(fd, &fb_info, sizeof(fb_info), -1) == -1)
	{
		perror("pread");
		exit(1);
	}

	if (scale)
		image = MUST(image->resize(fb_info.width, fb_info.height));

	ASSERT(BANAN_FB_BPP == 24 || BANAN_FB_BPP == 32);

	size_t mmap_size = fb_info.height * fb_info.width * BANAN_FB_BPP / 8;

	void* mmap_addr = mmap(nullptr, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mmap_addr == MAP_FAILED)
	{
		perror("mmap");
		exit(1);
	}

	uint8_t* u8_fb = reinterpret_cast<uint8_t*>(mmap_addr);

	const auto& bitmap = image->bitmap();
	for (uint64_t y = 0; y < BAN::Math::min<uint64_t>(image->height(), fb_info.height); y++)
	{
		for (uint64_t x = 0; x < BAN::Math::min<uint64_t>(image->width(), fb_info.width); x++)
		{
			u8_fb[(y * fb_info.width + x) * BANAN_FB_BPP / 8 + 0] = bitmap[y * image->width() + x].b;
			u8_fb[(y * fb_info.width + x) * BANAN_FB_BPP / 8 + 1] = bitmap[y * image->width() + x].g;
			u8_fb[(y * fb_info.width + x) * BANAN_FB_BPP / 8 + 2] = bitmap[y * image->width() + x].r;
			if constexpr(BANAN_FB_BPP == 32)
				u8_fb[(y * fb_info.width + x) * BANAN_FB_BPP / 8 + 3] = bitmap[y * image->width() + x].a;
		}
	}

	if (msync(mmap_addr, mmap_size, MS_SYNC) == -1)
	{
		perror("msync");
		exit(1);
	}

	munmap(mmap_addr, mmap_size);
	close(fd);
}

int usage(char* arg0, int ret)
{
	FILE* out = (ret == 0) ? stdout : stderr;
	fprintf(out, "usage: %s [options]... IMAGE_PATH\n", arg0);
	fprintf(out, "options:\n");
	fprintf(out, "    -h, --help:   show this message and exit\n");
	fprintf(out, "    -s, --scale:  scale image to framebuffer size\n");
	return ret;
}

int main(int argc, char** argv)
{
	if (argc < 2)
		return usage(argv[0], 1);

	bool scale = false;
	for (int i = 1; i < argc - 1; i++)
	{
		if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--scale") == 0)
			scale = true;
		else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
			return usage(argv[0], 0);
		else
			return usage(argv[0], 1);
	}

	auto image_path = BAN::StringView(argv[argc - 1]);

	auto image_or_error = LibImage::Image::load_from_file(image_path);
	if (image_or_error.is_error())
	{
		fprintf(stderr, "Could not load image '%.*s': %s\n",
			(int)image_path.size(),
			image_path.data(),
			strerror(image_or_error.error().get_error_code())
		);
		return 1;
	}

	render_to_framebuffer(image_or_error.release_value(), scale);

	for (;;)
		sleep(1);

	return 0;
}
