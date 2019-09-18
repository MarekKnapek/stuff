#include "static_initialization.h"


#pragma push_macro("new")
#ifdef new
#undef new
#endif


void static_initialization_all::register_obj(static_initialization_base* const obj)
{
	obj->m_next = g_head;
	g_head = obj;
}

void static_initialization_all::construct_all()
{
	static_initialization_base const* curr = g_head;
	while(curr)
	{
		(*curr->m_constructor)(curr->m_storage);
		curr = curr->m_next;
	}
}

void static_initialization_all::destruct_all()
{
	static_initialization_base const* curr = g_head;
	while(curr)
	{
		(*curr->m_destructor)(curr->m_storage);
		curr = curr->m_next;
	}
}


static_initialization_base* static_initialization_all::g_head;


#pragma pop_macro("new")
