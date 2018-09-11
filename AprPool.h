#ifndef __APR_POOL_H__
#define __APR_POOL_H__

/* apr includes */
#include "apr.h"
#include "apr_pools.h"
#include "apr_strings.h"

#ifdef _MSC_VER
/* link libraries */
#pragma comment(lib, "libapr-1")
#pragma comment(lib, "libapriconv-1")
#pragma comment(lib, "libaprutil-1")
#endif

#include <iostream>
#include <iomanip>
#include <limits>
#include <string>

namespace internal
{
	static void apr_error(apr_status_t st, const char *fileLine)
	{
		char err_msg[256]{ 0 };
		apr_strerror(st, err_msg, sizeof(err_msg));

		printf("%s : %s (0x%X)\n", fileLine, err_msg, st);
	}

	template<typename T>
	static apr_status_t delete_object(void *t)
	{
		reinterpret_cast<T*>(t)->~T();
		operator delete(t);
		
		return APR_SUCCESS;
	}


	template<typename T>
	static apr_status_t generic_cleanup(void *t)
	{
		reinterpret_cast<T*>(t)->~T();

		return APR_SUCCESS;
	}

}

#define APR_ERR(status)	(internal::apr_error(status, APR_POOL__FILE_LINE__))
#define SUCCESS(st)		(st == APR_SUCCESS)
#define FAIL(st)		(st != APR_SUCCESS)

static inline apr_status_t INIT_APR()
{
	apr_status_t st = apr_initialize();

	if (FAIL(st))
		throw new std::exception("Failed to init APR");

	return st;
}

class Pool
{
public:

	apr_pool_t * m_pool = nullptr;

	// APR Allocator
	template<typename T>
	class AprAllocator
	{
	public:

		apr_pool_t * m_pool = nullptr;

		using value_type = T;

		using reference = T & ;
		using const_reference = const T&;
    
		using pointer = T * ;
		using const_pointer = const T *;
    
		using void_pointer = void*;
		using const_void_pointer = const void*;
    
		using size_type = size_t;

		AprAllocator() = default;
		~AprAllocator() = default;

		AprAllocator(apr_pool_t *pool) : m_pool(pool) { }
		AprAllocator(const Pool& pool) : m_pool(pool.ap()) { }

		template<typename U>
		AprAllocator(const AprAllocator<U> &other) : m_pool(other.m_pool) {}

		pointer allocate(size_type size)
		{
#ifdef _DEBUG
			auto *p = reinterpret_cast<pointer>(apr_palloc(m_pool, size * sizeof(T)));
			printf("%4u %18s 0x%8p (0x%8p)\n", size * sizeof(T), "bytes allocated at", p, m_pool);
			return reinterpret_cast<pointer>(apr_palloc(m_pool, size * sizeof(T)));
#elif
			return reinterpret_cast<pointer>(apr_palloc(m_pool, size * sizeof(T)));
#endif
		}

		void deallocate(pointer p, size_type n = 1)
		{
			apr_pool_clear(m_pool);
		}

		template <typename U>
		struct rebind
		{
			using other = AprAllocator<U>;
		};
	};

	template<typename Type = int>
	Pool::AprAllocator<Type> getAllocator() const
	{
		return Pool::AprAllocator<Type>(m_pool);
	}

	Pool()
	{
		apr_status_t st = apr_pool_create((apr_pool_t **)&m_pool, NULL);

		if (FAIL(st))
		{
			APR_ERR(st);
			return;
		}
	}

	Pool(apr_pool_t &other)
	{
		m_pool = &other;
	}

	Pool(const Pool &p)
	{
		m_pool = p.ap();
	}

	Pool &operator=(const Pool &p)
	{
		if (this != &p)
		{
			Pool tmp(p);
			std::swap(m_pool, tmp.m_pool);
		}

		return *this;
	}

	~Pool() noexcept
	{
		if (m_pool != nullptr) apr_pool_destroy(m_pool);
	}

	apr_pool_t* ap() const
	{
		return m_pool;
	}

	inline void *alloc(size_t n)
	{
		return apr_pcalloc(m_pool, static_cast<apr_size_t>(n));
	}

	typedef apr_status_t(*Callback)(void*);

	inline void attach(void *v, Callback cb)
	{
		apr_pool_cleanup_register(m_pool, v, cb, apr_pool_cleanup_null);
	}

	void detach(void *v, Callback cb)
	{
		apr_pool_cleanup_kill(m_pool, v, cb);
	}

	void clear()
	{
		apr_pool_clear(m_pool);
	}

	template<typename T>
	inline void attach(T *t)
	{
		attach((void*)t, (Callback)(internal::delete_object<T>));
	}

	template<typename T>
	void detach(T *t)
	{
		detach((void*)t, (Callback)(internal::delete_object<T>));
	}

	template<typename T>
	T *construct(size_t n = 1)
	{
		T *p = static_cast<T*>(alloc(n));
		attach(p, (Callback)internal::generic_cleanup<T>);
		return ::new(p) T();
	}

	template<typename T>
	T *construct(const T &t)
	{
		T *p = static_cast<T*>(alloc(sizeof(T)));
		attach(p, (Callback)internal::generic_cleanup<T>);
		return ::new(p) T(t);
	}

	template<typename T, typename... Args>
	T *construct(Args&&... args)
	{
		T *p = static_cast<T*>(alloc(sizeof(T)));
		attach(p, (Callback)internal::generic_cleanup<T>);
		return ::new(p) T(std::forward<Args>(args)...);
	}

	template<typename T>
	void destroy(T *t)
	{
		t->~T();
		detach(t);
	}

	char* operator()(const char *str)
	{
		return apr_pstrdup(m_pool, str);
	}
};

#endif
