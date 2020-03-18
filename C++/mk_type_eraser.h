#pragma once


#include <utility> // forward, move
#include <new> // placement new
#include <cstddef> // nullptr_t
#include <cassert> // assert
#include <cstdint> // uint32_t
#include <cstring> // memset


namespace mk
{


	namespace detail
	{


		static constexpr int const s_storage_size_ptrs = 2;
		static constexpr std::uint32_t const s_feel_dead = 0xFEE1DEAD; // 0xFEE1DEAD == Feel dead.


		union storate_u
		{
			unsigned char m_chars[sizeof(void*) * s_storage_size_ptrs];
			void* m_ptrs[s_storage_size_ptrs];
		};

		struct data_t
		{
			storate_u m_union;
		};

		template<bool b>
		struct bool_t
		{
		};

		template<typename t>
		struct is_small
		{
			static constexpr bool const value = sizeof(t) <= sizeof(data_t) && alignof(t) <= alignof(data_t);
		};

		template<typename t>
		struct remove_ref
		{
			typedef t type;
		};

		template<typename t>
		struct remove_ref<t&>
		{
			typedef t type;
		};

		template<typename t>
		struct remove_ref<t&&>
		{
			typedef t type;
		};

		template<typename return_t, typename... param_ts>
		struct remove_ref<return_t(&)(param_ts...)>
		{
			typedef return_t(*type)(param_ts...);
		};

		template<typename t>
		struct remove_const
		{
			typedef t type;
		};

		template<typename t>
		struct remove_const<t const>
		{
			typedef t type;
		};

		template<typename t1, typename t2>
		struct is_same
		{
			static constexpr bool const value = false;
		};

		template<typename t>
		struct is_same<t, t>
		{
			static constexpr bool const value = true;
		};

		template<bool b, typename t = void>
		struct enable_if
		{
		};

		template<typename t>
		struct enable_if<true, t>
		{
			typedef t type;
		};


		template<typename return_t, typename... param_ts>
		class object_t
		{
		public:
			typedef void(*construct_move_t)(object_t&, object_t&);
			typedef void(*destroy_t)(object_t&);
			typedef return_t(*execute_t)(object_t&, param_ts&&...);
			struct funcs_t
			{
				construct_move_t m_construct_move;
				destroy_t m_destroy;
				execute_t m_execute;
			};
		public:
			data_t m_data;
			funcs_t const* m_funcs;
		};
		
		template<typename return_t, typename... param_ts>
		class executor_base
		{
		public:
			typedef object_t<return_t, param_ts...> object_concrete_t;
		public:
			static void initialize(object_concrete_t& self)
			{
				for(int i = 0; i != sizeof(self.m_data); ++i){ assert((self.m_data.m_union.m_chars[i] = reinterpret_cast<unsigned char const*>(&s_feel_dead)[i % sizeof(s_feel_dead)], true)); }
				self.m_funcs = &s_funcs_nonop;
			}
		private:
			static void construct_move_nonop(object_concrete_t&, object_concrete_t& other)
			{
				other.m_funcs = &s_funcs_nonop;
			}
			static void destroy_nonop(object_concrete_t&)
			{
			}
		public:
			typedef typename object_concrete_t::funcs_t funcs_t;
			static constexpr funcs_t const s_funcs_nonop = {&construct_move_nonop, &destroy_nonop, nullptr};
		};

		template<bool is_small, typename invocable_t, typename return_t, typename... param_ts>
		class executor
		{
		};

		template<typename invocable_t, typename return_t, typename... param_ts>
		class executor<true, invocable_t, return_t, param_ts...>
		{
		public:
			typedef object_t<return_t, param_ts...> object_concrete_t;
		public:
			template<typename invocable_concrete_t>
			static void construct_forward(object_concrete_t& self, invocable_concrete_t&& invocable)
			{
				void* const buff = self.m_data.m_union.m_chars;
				new(buff) invocable_t{std::forward<invocable_concrete_t>(invocable)};
				self.m_funcs = &s_funcs_small;
			}
		private:
			static void construct_move_small(object_concrete_t& self, object_concrete_t& other)
			{
				invocable_t& self_invocable = *static_cast<invocable_t*>(static_cast<void*>(self.m_data.m_union.m_chars));
				void* const buff = other.m_data.m_union.m_chars;
				new(buff) invocable_t{std::move(self_invocable)};
				other.m_funcs = &s_funcs_small;
				self_invocable.~invocable_t();
			}
			static void destroy_small(object_concrete_t& self)
			{
				invocable_t& invocable = *static_cast<invocable_t*>(static_cast<void*>(self.m_data.m_union.m_chars));
				invocable.~invocable_t();
			}
			static return_t execute_small(object_concrete_t& self, param_ts&&... params)
			{
				invocable_t& invocable = *static_cast<invocable_t*>(static_cast<void*>(self.m_data.m_union.m_chars));
				return invocable(std::forward<param_ts>(params)...);
			}
		public:
			typedef typename object_concrete_t::funcs_t funcs_t;
			static constexpr funcs_t const s_funcs_small = {&construct_move_small, &destroy_small, &execute_small};
		};

		template<typename invocable_t, typename return_t, typename... param_ts>
		class executor<false, invocable_t, return_t, param_ts...>
		{
		public:
			typedef object_t<return_t, param_ts...> object_concrete_t;
		public:
			template<typename invocable_concrete_t>
			static void construct_forward(object_concrete_t& self, invocable_concrete_t&& invocable)
			{
				invocable_t* const new_invocable = new invocable_t{std::forward<invocable_concrete_t>(invocable)};
				self.m_data.m_union.m_ptrs[0] = new_invocable;
				self.m_funcs = &s_funcs_large;
			}
		private:
			static void construct_move_large(object_concrete_t& self, object_concrete_t& other)
			{
				invocable_t* const self_invocable = static_cast<invocable_t*>(self.m_data.m_union.m_ptrs[0]);
				other.m_data.m_union.m_ptrs[0] = self_invocable;
				other.m_funcs = &s_funcs_large;
				self.m_funcs = &executor_base<return_t, param_ts...>::s_funcs_nonop;
			}
			static void destroy_large(object_concrete_t& self)
			{
				invocable_t* const invocable = static_cast<invocable_t*>(self.m_data.m_union.m_ptrs[0]);
				delete invocable;
			}
			static return_t execute_large(object_concrete_t& self, param_ts&&... params)
			{
				invocable_t& invocable = *static_cast<invocable_t*>(self.m_data.m_union.m_ptrs[0]);
				return invocable(std::forward<param_ts>(params)...);
			}
		public:
			typedef typename object_concrete_t::funcs_t funcs_t;
			static constexpr funcs_t const s_funcs_large = {&construct_move_large, &destroy_large, &execute_large};
		};


		template<typename signature_t>
		class type_eraser_impl_t
		{
		};

		template<typename return_t, typename... param_ts>
		class type_eraser_impl_t<return_t(param_ts...)>
		{
		public:
			type_eraser_impl_t() /* do not add initializer list*/
			{
				executor_base<return_t, param_ts...>::initialize(m_object);
			}
			template<typename invocable_t>
			type_eraser_impl_t(invocable_t&& invocable) :
				type_eraser_impl_t()
			{
				typedef typename remove_ref<invocable_t>::type invocable_nonref_t;
				typedef executor<is_small<invocable_nonref_t>::value, invocable_nonref_t, return_t, param_ts...> executor_t;
				executor_t::construct_forward(m_object, std::forward<invocable_t>(invocable));
			}
			void swap(type_eraser_impl_t<return_t(param_ts...)>& other)
			{
				type_eraser_impl_t<return_t(param_ts...)> tmp;
				type_eraser_impl_t<return_t(param_ts...)>& self = *this;
				self.m_object.m_funcs->m_construct_move(self.m_object, tmp.m_object); // Move self to tmp.
				other.m_object.m_funcs->m_construct_move(other.m_object, self.m_object); // Move other to self.
				tmp.m_object.m_funcs->m_construct_move(tmp.m_object, other.m_object); // Move tmp to other.
			}
			~type_eraser_impl_t()
			{
				m_object.m_funcs->m_destroy(m_object);
			}
			return_t execute(param_ts&&... params)
			{
				return m_object.m_funcs->m_execute(m_object, std::forward<param_ts>(params)...);
			}
		private:
			object_t<return_t, param_ts...> m_object;
		};

	}


	template<typename signature_t>
	class type_eraser_t : private detail::type_eraser_impl_t<signature_t>
	{
	public:
		type_eraser_t() noexcept :
			type_eraser_impl_t()
		{
		}
		type_eraser_t(std::nullptr_t const&) noexcept :
			type_eraser_impl_t(),
		{
		}
		template<typename invocable_t, typename = typename detail::enable_if<!detail::is_same<typename detail::remove_ref<typename detail::remove_const<invocable_t>::type>::type, type_eraser_t<signature_t>>::value>::type>
		type_eraser_t(invocable_t&& invocable) :
			type_eraser_impl_t(std::forward<invocable_t>(invocable))
		{
		}
		type_eraser_t(type_eraser_t<signature_t> const&) = delete;
		type_eraser_t(type_eraser_t<signature_t>&& other) noexcept :
			type_eraser_t()
		{
			this->swap(other);
		}
		type_eraser_t<signature_t>& operator=(type_eraser_t<signature_t> const&) = delete;
		type_eraser_t<signature_t>& operator=(type_eraser_t<signature_t>&& other) noexcept
		{
			this->swap(other);
			return *this;
		}
		void swap(type_eraser_t<signature_t>& other) noexcept
		{
			type_eraser_impl_t<signature_t>::swap(other);
		}
		~type_eraser_t()
		{
		}
		template<typename... param_ts>
		auto operator()(param_ts&&... params)
		{
			return execute(std::forward<param_ts>(params)...);
		}
	};

	template<typename signature_t> void swap(type_eraser_t<signature_t>& a, type_eraser_t<signature_t>& b) noexcept { a.swap(b); }

	template<typename signature_t, typename invocable_t>
	type_eraser_t<signature_t> make_type_eraser(invocable_t&& invocable)
	{
		return type_eraser_t<signature_t>{std::forward<invocable_t>(invocable)};
	}


}
