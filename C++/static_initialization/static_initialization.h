#pragma once


#pragma push_macro("new")
#ifdef new
#undef new
#endif


struct static_initialization_base;


typedef void(*static_initialization_func_t)(void*);


struct static_initialization_all
{
public:
	static void register_obj(static_initialization_base* const obj);
	static void construct_all();
	static void destruct_all();
public:
	static static_initialization_base* g_head;
};


struct static_initialization_base
{
public:
	static_initialization_base(static_initialization_func_t const constructor, static_initialization_func_t const destructor, void* const storage) :
		m_constructor(constructor),
		m_destructor(destructor),
		m_storage(storage),
		m_next(nullptr)
	{
	}
public:
	static_initialization_func_t const m_constructor;
	static_initialization_func_t const m_destructor;
	void* const m_storage;
	static_initialization_base* m_next;
};


template <typename T>
struct static_initialization : public static_initialization_base
{
public:
	static_initialization() :
		static_initialization_base([](void* const storage){ new(storage)T(); }, [](void* const storage){ reinterpret_cast<T*>(storage)->~T(); }, m_aligned_buffer),
		m_aligned_buffer()
	{
		static_initialization_all::register_obj(this);
	}
public:
	alignas(alignof(T)) char m_aligned_buffer[sizeof(T)];
};


#pragma pop_macro("new")
