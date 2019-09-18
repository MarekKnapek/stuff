#include "StaticInitialization.h"


void StaticInitializationAll::Register(StaticInitializationBase* ptr)
{
	ptr->m_next = g_head;
	g_head = ptr;
}

void StaticInitializationAll::ConstructAll()
{
	StaticInitializationBase* curr = g_head;
	while(curr)
	{
		(*curr->m_pfnConstructor)(curr->m_pStorage);
		curr = curr->m_next;
	}
}

void StaticInitializationAll::DestructAll()
{
	StaticInitializationBase* curr = g_head;
	while(curr)
	{
		(*curr->m_pfnDestructor)(curr->m_pStorage);
		curr = curr->m_next;
	}
}


StaticInitializationBase* StaticInitializationAll::g_head;
