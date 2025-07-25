#include <kernel/BootInfo.h>
#include <kernel/Memory/Heap.h>
#include <kernel/Memory/PageTable.h>

extern uint8_t g_kernel_end[];

namespace Kernel
{

	static Heap* s_instance = nullptr;

	void Heap::initialize()
	{
		ASSERT(s_instance == nullptr);
		s_instance = new Heap;
		ASSERT(s_instance);
		s_instance->initialize_impl();
	}

	Heap& Heap::get()
	{
		ASSERT(s_instance);
		return *s_instance;
	}

	void Heap::initialize_impl()
	{
		if (g_boot_info.memory_map_entries.empty())
			panic("Bootloader did not provide a memory map");

		for (const auto& entry : g_boot_info.memory_map_entries)
		{
			const char* entry_type_string = nullptr;
			switch (entry.type)
			{
				case MemoryMapEntry::Type::Available:
					entry_type_string = "available";
					break;
				case MemoryMapEntry::Type::Reserved:
					entry_type_string = "reserved";
					break;
				case MemoryMapEntry::Type::ACPIReclaim:
					entry_type_string = "acpi reclaim";
					break;
				case MemoryMapEntry::Type::ACPINVS:
					entry_type_string = "acpi nvs";
					break;
				default:
					ASSERT_NOT_REACHED();
			}

			dprintln("{16H}, {16H}, {}",
				entry.address,
				entry.length,
				entry_type_string
			);

			if (entry.type != MemoryMapEntry::Type::Available)
				continue;

			// FIXME: only reserve kernel area and modules, not everything from 0 -> kernel end
			paddr_t start = entry.address;
			if (start < (vaddr_t)g_kernel_end - KERNEL_OFFSET + g_boot_info.kernel_paddr)
				start = (vaddr_t)g_kernel_end - KERNEL_OFFSET + g_boot_info.kernel_paddr;
			for (const auto& module : g_boot_info.modules)
				if (start < module.start + module.size)
					start = module.start + module.size;
			if (auto rem = start % PAGE_SIZE)
				start += PAGE_SIZE - rem;

			paddr_t end = entry.address + entry.length;
			if (auto rem = end % PAGE_SIZE)
				end -= rem;

			// Physical pages needs atleast 2 pages
			if (end > start + PAGE_SIZE)
				MUST(m_physical_ranges.emplace_back(start, end - start));
		}

		size_t total = 0;
		for (auto& range : m_physical_ranges)
		{
			size_t bytes = range.usable_memory();
			dprintln("RAM {8H}->{8H} ({}.{} MB)", range.start(), range.end(), bytes / (1 << 20), bytes % (1 << 20) * 1000 / (1 << 20));
			total += bytes;
		}
		dprintln("Total RAM {}.{} MB", total / (1 << 20), total % (1 << 20) * 1000 / (1 << 20));
	}

	paddr_t Heap::take_free_page()
	{
		SpinLockGuard _(m_lock);
		for (auto& range : m_physical_ranges)
			if (range.free_pages() >= 1)
				return range.reserve_page();
		return 0;
	}

	void Heap::release_page(paddr_t paddr)
	{
		SpinLockGuard _(m_lock);
		for (auto& range : m_physical_ranges)
			if (range.contains(paddr))
				return range.release_page(paddr);
		panic("tried to free invalid paddr {16H}", paddr);
	}

	paddr_t Heap::take_free_contiguous_pages(size_t pages)
	{
		SpinLockGuard _(m_lock);
		for (auto& range : m_physical_ranges)
			if (range.free_pages() >= pages)
				if (paddr_t paddr = range.reserve_contiguous_pages(pages))
					return paddr;
		return 0;
	}

	void Heap::release_contiguous_pages(paddr_t paddr, size_t pages)
	{
		SpinLockGuard _(m_lock);
		for (auto& range : m_physical_ranges)
			if (range.contains(paddr))
				return range.release_contiguous_pages(paddr, pages);
		ASSERT_NOT_REACHED();
	}

	size_t Heap::used_pages() const
	{
		SpinLockGuard _(m_lock);
		size_t result = 0;
		for (const auto& range : m_physical_ranges)
			result += range.used_pages();
		return result;
	}

	size_t Heap::free_pages() const
	{
		SpinLockGuard _(m_lock);
		size_t result = 0;
		for (const auto& range : m_physical_ranges)
			result += range.free_pages();
		return result;
	}

}
