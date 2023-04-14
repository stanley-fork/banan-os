#pragma once

#include <BAN/NoCopyMove.h>
#include <BAN/Vector.h>

#include <stdint.h>

#define PAGE_SIZE 4096

namespace Kernel::Memory
{

	using paddr_t = uintptr_t;

	class PhysicalRange
	{
	public:
		static constexpr paddr_t invalid = ~paddr_t(0);

	public:
		PhysicalRange(paddr_t, size_t);
		paddr_t reserve_page();
		void release_page(paddr_t);

		paddr_t usable_start() const { return m_start + m_list_pages  * PAGE_SIZE; }
		paddr_t usable_end() const   { return m_start + m_total_pages * PAGE_SIZE; }
		uint64_t usable_pages() const { return m_reservable_pages; }

	private:
		struct node
		{
			node* next;
			node* prev;
		};
		
		paddr_t page_address(const node*) const;
		node* node_address(paddr_t) const;

	private:
		paddr_t m_start	{ 0 };
		size_t m_size	{ 0 };

		uint64_t m_total_pages		{ 0 };
		uint64_t m_reservable_pages	{ 0 };
		uint64_t m_list_pages		{ 0 };

		node* m_free_list { nullptr };
		node* m_used_list { nullptr };
	};
	
	class Heap
	{
		BAN_NON_COPYABLE(Heap);
		BAN_NON_MOVABLE(Heap);

	public:
		static void initialize();
		static Heap& get();

	private:
		Heap() = default;
		void initialize_impl();

	private:
		BAN::Vector<PhysicalRange> m_physical_ranges;
	};

}