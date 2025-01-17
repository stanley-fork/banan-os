#pragma once

#include <BAN/Atomic.h>
#include <BAN/Errors.h>
#include <BAN/Move.h>
#include <BAN/NoCopyMove.h>
#include <stdint.h>

namespace BAN
{

	template<typename T>
	class RefCounted
	{
		BAN_NON_COPYABLE(RefCounted);
		BAN_NON_MOVABLE(RefCounted);

	public:
		uint32_t ref_count() const
		{
			return m_ref_count;
		}

		void ref() const
		{
			uint32_t old = m_ref_count.fetch_add(1, MemoryOrder::memory_order_relaxed);
			ASSERT(old > 0);
		}

		bool try_ref() const
		{
			uint32_t expected = m_ref_count.load(MemoryOrder::memory_order_relaxed);
			for (;;)
			{
				if (expected == 0)
					return false;
				if (m_ref_count.compare_exchange(expected, expected + 1, MemoryOrder::memory_order_acquire))
					return true;
			}
		}

		void unref() const
		{
			uint32_t old = m_ref_count.fetch_sub(1);
			ASSERT(old > 0);
			if (old == 1)
				delete static_cast<const T*>(this);
		}

	protected:
		RefCounted() = default;
		virtual ~RefCounted() { ASSERT(m_ref_count == 0); }

	private:
		mutable Atomic<uint32_t> m_ref_count = 1;
	};

	template<typename T>
	class RefPtr
	{
	public:
		RefPtr() = default;
		RefPtr(T* pointer)
		{
			m_pointer = pointer;
			if (m_pointer)
				m_pointer->ref();
		}
		~RefPtr() { clear(); }

		template<typename U>
		static RefPtr adopt(U* pointer)
		{
			RefPtr ptr;
			ptr.m_pointer = pointer;
			return ptr;
		}

		// NOTE: don't use is_constructible_v<T, Args...> as RefPtr<T> is allowed with friends
		template<typename... Args>
		static ErrorOr<RefPtr> create(Args&&... args) requires requires(Args&&... args) { T(forward<Args>(args)...); }
		{
			T* pointer = new T(forward<Args>(args)...);
			if (pointer == nullptr)
				return Error::from_errno(ENOMEM);
			return adopt(pointer);
		}

		RefPtr(const RefPtr& other) { *this = other; }
		RefPtr(RefPtr&& other) { *this = move(other); }
		template<typename U>
		RefPtr(const RefPtr<U>& other) { *this = other; }
		template<typename U>
		RefPtr(RefPtr<U>&& other) { *this = move(other); }

		RefPtr& operator=(const RefPtr& other)
		{
			clear();
			m_pointer = other.m_pointer;
			if (m_pointer)
				m_pointer->ref();
			return *this;
		}

		RefPtr& operator=(RefPtr&& other)
		{
			clear();
			m_pointer = other.m_pointer;
			other.m_pointer = nullptr;
			return *this;
		}

		template<typename U>
		RefPtr& operator=(const RefPtr<U>& other)
		{
			clear();
			m_pointer = other.m_pointer;
			if (m_pointer)
				m_pointer->ref();
			return *this;
		}

		template<typename U>
		RefPtr& operator=(RefPtr<U>&& other)
		{
			clear();
			m_pointer = other.m_pointer;
			other.m_pointer = nullptr;
			return *this;
		}

		T* ptr() { ASSERT(!empty()); return m_pointer; }
		const T* ptr() const { ASSERT(!empty()); return m_pointer; }

		T& operator*() { return *ptr(); }
		const T& operator*() const { return *ptr(); }

		T* operator->() { return ptr(); }
		const T* operator->() const { return ptr(); }

		bool operator==(RefPtr other) const { return m_pointer == other.m_pointer; }
		bool operator!=(RefPtr other) const { return m_pointer != other.m_pointer; }

		bool empty() const { return m_pointer == nullptr; }
		explicit operator bool() const { return m_pointer; }

		void clear()
		{
			if (m_pointer)
				m_pointer->unref();
			m_pointer = nullptr;
		}

	private:
		T* m_pointer = nullptr;

		template<typename U>
		friend class RefPtr;
	};

}
