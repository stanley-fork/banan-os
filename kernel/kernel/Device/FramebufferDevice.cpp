#include <kernel/BootInfo.h>
#include <kernel/Device/DeviceNumbers.h>
#include <kernel/Device/FramebufferDevice.h>
#include <kernel/FS/DevFS/FileSystem.h>
#include <kernel/Memory/Heap.h>
#include <kernel/Terminal/FramebufferTerminal.h>

#include <sys/framebuffer.h>
#include <sys/mman.h>
#include <sys/sysmacros.h>

namespace Kernel
{

	static uint32_t get_framebuffer_device_index()
	{
		static uint32_t index = 0;
		return index++;
	}

	BAN::ErrorOr<BAN::RefPtr<FramebufferDevice>> FramebufferDevice::create_from_boot_framebuffer()
	{
		ASSERT(g_boot_info.framebuffer.type == FramebufferInfo::Type::RGB);
		if (g_boot_info.framebuffer.bpp != 24 && g_boot_info.framebuffer.bpp != 32)
			return BAN::Error::from_errno(ENOTSUP);
		auto* device_ptr = new FramebufferDevice(
			0660, 0, 900,
			makedev(DeviceNumber::Framebuffer, get_framebuffer_device_index()),
			g_boot_info.framebuffer.address,
			g_boot_info.framebuffer.width,
			g_boot_info.framebuffer.height,
			g_boot_info.framebuffer.pitch,
			g_boot_info.framebuffer.bpp
		);
		if (device_ptr == nullptr)
			return BAN::Error::from_errno(ENOMEM);
		auto device = BAN::RefPtr<FramebufferDevice>::adopt(device_ptr);
		TRY(device->initialize());
		DevFileSystem::get().add_device(device);
		return device;
	}

	FramebufferDevice::FramebufferDevice(mode_t mode, uid_t uid, gid_t gid, dev_t rdev, paddr_t paddr, uint32_t width, uint32_t height, uint32_t pitch, uint8_t bpp)
		: CharacterDevice(mode, uid, gid)
		, m_name(MUST(BAN::String::formatted("fb{}", minor(rdev))))
		, m_rdev(rdev)
		, m_video_memory_paddr(paddr)
		, m_width(width)
		, m_height(height)
		, m_pitch(pitch)
		, m_bpp(bpp)
	{ }

	FramebufferDevice::~FramebufferDevice()
	{
		if (m_video_memory_vaddr == 0)
			return;
		size_t video_memory_pages = range_page_count(m_video_memory_paddr, m_height * m_pitch);
		PageTable::kernel().unmap_range(m_video_memory_vaddr, video_memory_pages * PAGE_SIZE);
	}

	BAN::ErrorOr<void> FramebufferDevice::initialize()
	{
		size_t video_memory_pages = range_page_count(m_video_memory_paddr, m_height * m_pitch);
		m_video_memory_vaddr = PageTable::kernel().reserve_free_contiguous_pages(video_memory_pages, KERNEL_OFFSET);
		if (m_video_memory_vaddr == 0)
			return BAN::Error::from_errno(ENOMEM);
		PageTable::kernel().map_range_at(
			m_video_memory_paddr & PAGE_ADDR_MASK,
			m_video_memory_vaddr,
			video_memory_pages * PAGE_SIZE,
			PageTable::Flags::ReadWrite | PageTable::Flags::Present,
			PageTable::WriteCombining
		);

		m_video_buffer = TRY(VirtualRange::create_to_vaddr_range(
			PageTable::kernel(),
			KERNEL_OFFSET, UINTPTR_MAX,
			BAN::Math::div_round_up<size_t>(m_width * m_height * (BANAN_FB_BPP / 8), PAGE_SIZE) * PAGE_SIZE,
			PageTable::Flags::ReadWrite | PageTable::Flags::Present,
			true, false
		));

		return {};
	}

	BAN::ErrorOr<size_t> FramebufferDevice::read_impl(off_t offset, BAN::ByteSpan buffer)
	{
		// Reading from negative offset will fill buffer with framebuffer info
		if (offset < 0)
		{
			if (buffer.size() < sizeof(framebuffer_info_t))
				return BAN::Error::from_errno(ENOBUFS);

			auto& fb_info = buffer.as<framebuffer_info_t>();
			fb_info.width = m_width;
			fb_info.height = m_height;

			return sizeof(framebuffer_info_t);
		}

		if ((size_t)offset >= m_width * m_height * (BANAN_FB_BPP / 8))
			return 0;

		size_t bytes_to_copy = BAN::Math::min<size_t>(m_width * m_height * (BANAN_FB_BPP / 8) - offset, buffer.size());
		memcpy(buffer.data(), reinterpret_cast<void*>(m_video_buffer->vaddr() + offset), bytes_to_copy);

		return bytes_to_copy;
	}

	BAN::ErrorOr<size_t> FramebufferDevice::write_impl(off_t offset, BAN::ConstByteSpan buffer)
	{
		if (offset < 0)
			return BAN::Error::from_errno(EINVAL);
		if ((size_t)offset >= m_width * m_height * (BANAN_FB_BPP / 8))
			return 0;

		size_t bytes_to_copy = BAN::Math::min<size_t>(m_width * m_height * (BANAN_FB_BPP / 8) - offset, buffer.size());
		memcpy(reinterpret_cast<void*>(m_video_buffer->vaddr() + offset), buffer.data(), bytes_to_copy);

		uint32_t first_pixel = offset / (BANAN_FB_BPP / 8);
		uint32_t pixel_count = BAN::Math::div_round_up<uint32_t>(bytes_to_copy + (offset % (BANAN_FB_BPP / 8)), (BANAN_FB_BPP / 8));
		sync_pixels_linear(first_pixel, pixel_count);

		return bytes_to_copy;
	}

	uint32_t FramebufferDevice::get_pixel(uint32_t x, uint32_t y) const
	{
		ASSERT(x < m_width && y < m_height);
		static_assert(BANAN_FB_BPP == 32);
		return reinterpret_cast<uint32_t*>(m_video_buffer->vaddr())[y * m_width + x];
	}

	void FramebufferDevice::set_pixel(uint32_t x, uint32_t y, uint32_t rgb)
	{
		if (x >= m_width || y >= m_height)
			return;
		static_assert(BANAN_FB_BPP == 32);
		reinterpret_cast<uint32_t*>(m_video_buffer->vaddr())[y * m_width + x] = rgb;
	}

	void FramebufferDevice::fill(uint32_t rgb)
	{
		static_assert(BANAN_FB_BPP == 32);
		auto* video_buffer_u32 = reinterpret_cast<uint32_t*>(m_video_buffer->vaddr());
		for (uint32_t i = 0; i < m_width * m_height; i++)
			video_buffer_u32[i] = rgb;
	}

	void FramebufferDevice::scroll(int32_t rows, uint32_t rgb)
	{
		if (rows == 0)
			return;
		rows = BAN::Math::clamp<int32_t>(rows, -m_height, m_height);
		const uint32_t abs_rows = BAN::Math::abs(rows);

		auto* video_buffer_u8 = reinterpret_cast<uint8_t*>(m_video_buffer->vaddr());
		uint8_t* src = (rows < 0) ? video_buffer_u8 : &video_buffer_u8[(abs_rows * m_width) * (BANAN_FB_BPP / 8)];
		uint8_t* dst = (rows > 0) ? video_buffer_u8 : &video_buffer_u8[(abs_rows * m_width) * (BANAN_FB_BPP / 8)];
		memmove(dst, src, (m_height - abs_rows) * m_width * (BANAN_FB_BPP / 8));

		uint32_t start_y = (rows < 0) ? 0 : m_height - abs_rows;
		uint32_t stop_y  = (rows < 0) ? abs_rows : m_height;
		for (uint32_t y = start_y; y < stop_y; y++)
			for (uint32_t x = 0; x < m_width; x++)
				set_pixel(x, y, rgb);
	}

	void FramebufferDevice::sync_pixels_full()
	{
		return sync_pixels_linear(0, m_width * m_height);
	}

	void FramebufferDevice::sync_pixels_linear(uint32_t first_pixel, uint32_t pixel_count)
	{
		if (first_pixel >= m_width * m_height)
			return;
		if (first_pixel + pixel_count > m_width * m_height)
			pixel_count = m_width * m_height - first_pixel;

		auto* video_memory_u8 = reinterpret_cast<uint8_t*>(m_video_memory_vaddr);
		auto* video_buffer_u8 = reinterpret_cast<uint8_t*>(m_video_buffer->vaddr());

		if (m_bpp == BANAN_FB_BPP)
		{
			const uint32_t buffer_pitch = m_width * (BANAN_FB_BPP / 8);

			uint32_t row = first_pixel / m_width;
			if (auto rem = first_pixel % m_width)
			{
				const uint32_t to_copy = BAN::Math::min(pixel_count, m_width - rem);
				memcpy(
					&video_memory_u8[row * m_pitch      + rem * (BANAN_FB_BPP / 8)],
					&video_buffer_u8[row * buffer_pitch + rem * (BANAN_FB_BPP / 8)],
					to_copy * (BANAN_FB_BPP / 8)
				);
				pixel_count -= to_copy;
				row++;
			}

			while (pixel_count)
			{
				const uint32_t to_copy = BAN::Math::min(pixel_count, m_width);
				memcpy(
					&video_memory_u8[row * m_pitch],
					&video_buffer_u8[row * buffer_pitch],
					to_copy * (BANAN_FB_BPP / 8)
				);
				pixel_count -= to_copy;
				row++;
			}

			return;
		}

		for (uint32_t i = 0; i < pixel_count; i++)
		{
			uint32_t row = (first_pixel + i) / m_width;
			uint32_t idx = (first_pixel + i) % m_width;

			video_memory_u8[(row * m_pitch) + (idx * m_bpp / 8) + 0] = video_buffer_u8[(first_pixel + i) * (BANAN_FB_BPP / 8) + 0];
			video_memory_u8[(row * m_pitch) + (idx * m_bpp / 8) + 1] = video_buffer_u8[(first_pixel + i) * (BANAN_FB_BPP / 8) + 1];
			video_memory_u8[(row * m_pitch) + (idx * m_bpp / 8) + 2] = video_buffer_u8[(first_pixel + i) * (BANAN_FB_BPP / 8) + 2];
		}
	}

	void FramebufferDevice::sync_pixels_rectangle(uint32_t top_right_x, uint32_t top_right_y, uint32_t width, uint32_t height)
	{
		if (top_right_x >= m_width || top_right_y >= m_height)
			return;
		if (top_right_x + width > m_width)
			width = m_width - top_right_x;
		if (top_right_y + height > m_height)
			height = m_height - top_right_y;

		for (uint32_t row = top_right_y; row < top_right_y + height; row++)
			sync_pixels_linear(row * m_width + top_right_x, width);
	}

	class FramebufferMemoryRegion : public MemoryRegion
	{
	public:
		static BAN::ErrorOr<BAN::UniqPtr<FramebufferMemoryRegion>> create(PageTable& page_table, size_t size, AddressRange address_range, MemoryRegion::Type region_type, PageTable::flags_t page_flags, BAN::RefPtr<FramebufferDevice> framebuffer)
		{
			auto* region_ptr = new FramebufferMemoryRegion(page_table, size, region_type, page_flags, framebuffer);
			if (region_ptr == nullptr)
				return BAN::Error::from_errno(ENOMEM);
			auto region = BAN::UniqPtr<FramebufferMemoryRegion>::adopt(region_ptr);

			TRY(region->initialize(address_range));

			return region;
		}

		~FramebufferMemoryRegion()
		{
			m_framebuffer->sync_pixels_full();
		}

		virtual BAN::ErrorOr<void> msync(vaddr_t vaddr, size_t size, int flags) override
		{
			if (flags != MS_SYNC)
				return BAN::Error::from_errno(ENOTSUP);
			if (vaddr % (BANAN_FB_BPP / 8))
				return BAN::Error::from_errno(EINVAL);

			if (auto rem = size % (BANAN_FB_BPP / 8))
				size += (BANAN_FB_BPP / 8) - rem;

			const vaddr_t start = BAN::Math::max(vaddr, m_vaddr);
			const size_t  end   = BAN::Math::min(vaddr + size, m_vaddr + m_size);
			if (start < end)
			{
				do_msync(
					(start - m_vaddr) / (BANAN_FB_BPP / 8),
					(end   - start)   / (BANAN_FB_BPP / 8)
				);
			}

			return {};
		}

		virtual BAN::ErrorOr<BAN::UniqPtr<MemoryRegion>> clone(PageTable& new_page_table) override
		{
			auto* region_ptr = new FramebufferMemoryRegion(new_page_table, m_size, m_type, m_flags, m_framebuffer);
			if (region_ptr == nullptr)
				return BAN::Error::from_errno(ENOMEM);
			auto region = BAN::UniqPtr<FramebufferMemoryRegion>::adopt(region_ptr);

			TRY(region->initialize({ m_vaddr, m_vaddr + BAN::Math::div_round_up<uintptr_t>(m_size, PAGE_SIZE) * PAGE_SIZE }));

			return BAN::UniqPtr<MemoryRegion>(BAN::move(region));
		}

	protected:
		// Returns error if no memory was available
		// Returns true if page was succesfully allocated
		// Returns false if page was already allocated
		virtual BAN::ErrorOr<bool> allocate_page_containing_impl(vaddr_t vaddr, bool wants_write) override
		{
			(void)wants_write;

			vaddr &= PAGE_ADDR_MASK;
			if (m_page_table.physical_address_of(vaddr))
				return false;

			paddr_t paddr = PageTable::kernel().physical_address_of(m_framebuffer->m_video_buffer->vaddr() + (vaddr - m_vaddr));
			m_page_table.map_page_at(paddr, vaddr, m_flags);

			return true;
		}

	private:
		FramebufferMemoryRegion(PageTable& page_table, size_t size, MemoryRegion::Type region_type, PageTable::flags_t page_flags, BAN::RefPtr<FramebufferDevice> framebuffer)
			: MemoryRegion(page_table, size, region_type, page_flags)
			, m_framebuffer(framebuffer)
		{ }

		void do_msync(uint32_t first_pixel, uint32_t pixel_count)
		{
			if (!Processor::get_should_print_cpu_load())
				return m_framebuffer->sync_pixels_linear(first_pixel, pixel_count);

			const uint32_t fb_width = m_framebuffer->width();

			// If we are here (in FramebufferMemoryRegion), our terminal driver is FramebufferTerminalDriver
			ASSERT(g_terminal_driver->has_font());
			const auto& font = g_terminal_driver->font();

			const uint32_t x = first_pixel % fb_width;
			const uint32_t y = first_pixel / fb_width;

			const uint32_t load_w = 16                 * font.width();
			const uint32_t load_h = Processor::count() * font.height();

			if (y >= load_h || x + pixel_count <= fb_width - load_w)
				return m_framebuffer->sync_pixels_linear(first_pixel, pixel_count);

			if (x >= fb_width - load_w && x + pixel_count <= fb_width)
				return;

			if (x < fb_width - load_w)
				m_framebuffer->sync_pixels_linear(first_pixel, fb_width - load_w - x);

			if (x + pixel_count > fb_width)
			{
				const uint32_t past_last_pixel = first_pixel + pixel_count;

				first_pixel = (y + 1) * fb_width;
				pixel_count = past_last_pixel - first_pixel;

				const uint32_t cpu_load_end = load_h * fb_width;

				while (pixel_count && first_pixel < cpu_load_end)
				{
					m_framebuffer->sync_pixels_linear(first_pixel, BAN::Math::min(pixel_count, fb_width - load_w));

					const uint32_t advance = BAN::Math::min(pixel_count, fb_width);
					pixel_count -= advance;
					first_pixel += advance;
				}

				if (pixel_count)
					m_framebuffer->sync_pixels_linear(first_pixel, pixel_count);
			}
		}

	private:
		BAN::RefPtr<FramebufferDevice> m_framebuffer;
	};

	BAN::ErrorOr<BAN::UniqPtr<MemoryRegion>> FramebufferDevice::mmap_region(PageTable& page_table, off_t offset, size_t len, AddressRange address_range, MemoryRegion::Type region_type, PageTable::flags_t page_flags)
	{
		if (offset != 0)
			return BAN::Error::from_errno(EINVAL);
		if (len > m_video_buffer->size())
			return BAN::Error::from_errno(EINVAL);
		if (region_type != MemoryRegion::Type::SHARED)
			return BAN::Error::from_errno(EINVAL);

		auto region = TRY(FramebufferMemoryRegion::create(page_table, len, address_range, region_type, page_flags, this));
		return BAN::UniqPtr<MemoryRegion>(BAN::move(region));
	}

}
