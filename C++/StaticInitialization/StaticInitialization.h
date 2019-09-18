#pragma once


#pragma push_macro("new")
#ifdef new
#undef new
#endif


struct StaticInitializationBase;


struct StaticInitializationAll
{
public:
	static void Register(StaticInitializationBase* ptr);
	static void ConstructAll();
	static void DestructAll();
public:
	static StaticInitializationBase* g_head;
};


struct StaticInitializationBase
{
public:
	StaticInitializationBase(void(*pfnConstructor)(void*), void(*pfnDestructor)(void*), char* pStorage) :
		m_pfnConstructor(pfnConstructor),
		m_pfnDestructor(pfnDestructor),
		m_pStorage(pStorage),
		m_next(nullptr)
	{
	}
public:
	void(*const m_pfnConstructor)(void*);
	void(*const m_pfnDestructor)(void*);
	cha * const m_pStorage;
	StaticInitializationBase* m_next;
};


template <typename T>
struct StaticInitialization : public StaticInitializationBase
{
public:
	StaticInitialization() :
		StaticInitializationBase([](void * buff) { new (buff) T(); }, [](void * buff) { reinterpret_cast<T *>(buff)->~T(); }, m_storage),
		m_storage()
	{
		StaticInitializationAll::Register(this);
	}
public:
	alignas(alignof(T)) char m_storage[sizeof(T)];
};


#pragma pop_macro("new")
