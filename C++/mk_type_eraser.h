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


		static constexpr int const s_storage_size_bytes = 2 * sizeof(void*);
		static constexpr std::uint32_t const s_feel_dead = 0xFEE1DEAD; // 0xFEE1DEAD == Feel dead.


		union storate_u
		{
			unsigned char m_chars[s_storage_size_bytes];
			void* m_ptr;
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
		struct remove_ref_t
		{
			typedef t type;
		};

		template<typename t>
		struct remove_ref_t<t&>
		{
			typedef t type;
		};

		template<typename t>
		struct remove_ref_t<t&&>
		{
			typedef t type;
		};

		template<typename return_t, typename... param_ts>
		struct remove_ref_t<return_t(&)(param_ts...)>
		{
			typedef return_t(*type)(param_ts...);
		};

		template<typename t>
		struct remove_const_t
		{
			typedef t type;
		};

		template<typename t>
		struct remove_const_t<t const>
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

		template<typename t>
		struct last_step_transform_t
		{
			typedef t const& type;
		};
		template<typename t>
		struct last_step_transform_t<t&>
		{
			typedef t& type;
		};

		struct copy_construct_tag_t
		{
		};

		/* is_invocable_t */
		template<typename t>
		t&& declval_fn();

		template<typename /*t*/>
		struct dummy_t
		{
			typedef void type;
		};

		template<bool a, bool b>
		struct or_t
		{
			static constexpr bool const value = true;
		};
		template<>
		struct or_t<false, false>
		{
			static constexpr bool const value = false;
		};

		template<typename t>
		struct is_void_t
		{
			static constexpr bool const value = false;
		};
		template<>
		struct is_void_t<void>
		{
			static constexpr bool const value = true;
		};

		template<typename... /*ts*/>
		struct is_convertible_2_t
		{
			static constexpr bool const value = false;
		};
		template<typename from_t, typename to_t>
		struct is_convertible_2_t<typename dummy_t<decltype(declval_fn<void(*)(to_t)>()(declval_fn<from_t>()))>::type, from_t, to_t>
		{
			static constexpr bool const value = true;
		};
		template<typename from_t, typename to_t>
		struct is_convertible_t : is_convertible_2_t<void, from_t, to_t>
		{
		};

		template<typename t, typename return_t, typename... param_ts>
		struct has_good_return_type_t
		{
			static constexpr bool const value = or_t<is_void_t<return_t>::value, is_convertible_t<decltype(declval_fn<t>()(declval_fn<param_ts>()...)), return_t>::value>::value;
		};

		template<typename... /*ts*/>
		struct is_invocable_2_t
		{
			static constexpr bool const value = false;
		};
		template<typename t, typename return_t, typename... param_ts>
		struct is_invocable_2_t<typename dummy_t<decltype(declval_fn<t>()(declval_fn<param_ts>()...))>::type, t, return_t, param_ts...>
		{
			static constexpr bool const value = has_good_return_type_t<t, return_t, param_ts...>::value;
		};

		template<typename signature_t, typename t>
		struct is_invocable_t
		{
			static constexpr bool const value = false;
		};
		template<typename return_t, typename... param_ts, typename t>
		struct is_invocable_t<return_t(param_ts...), t> : is_invocable_2_t<void, t, return_t, param_ts...>
		{
		};
		/* is_invocable_t */

		template<typename return_t, typename... param_ts>
		class object_t
		{
		public:
			typedef void(*construct_move_t)(object_t&, object_t&);
			typedef void(*construct_copy_t)(object_t const&, object_t&);
			typedef void(*destroy_t)(object_t&);
			typedef return_t(*execute_t)(object_t&, typename last_step_transform_t<param_ts>::type&&...);
			struct funcs_t
			{
				construct_move_t m_construct_move;
				construct_copy_t m_construct_copy;
				destroy_t m_destroy;
				execute_t m_execute;
			};
		public:
			data_t m_data;
			funcs_t const* m_funcs;
		};
		
		template<typename return_t, typename... param_ts>
		class executor_base_t
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
			static void construct_copy_nonop(object_concrete_t const&, object_concrete_t& other)
			{
				other.m_funcs = &s_funcs_nonop;
			}
			static void destroy_nonop(object_concrete_t&)
			{
			}
		public:
			typedef typename object_concrete_t::funcs_t funcs_t;
			static constexpr funcs_t const s_funcs_nonop = {&construct_move_nonop, construct_copy_nonop, &destroy_nonop, nullptr};
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
			static void construct_copy_small(object_concrete_t const& self, object_concrete_t& other)
			{
				invocable_t const& self_invocable = *static_cast<invocable_t const*>(static_cast<void const*>(self.m_data.m_union.m_chars));
				void* const buff = other.m_data.m_union.m_chars;
				new(buff) invocable_t{self_invocable};
				other.m_funcs = &s_funcs_small;
			}
			static void destroy_small(object_concrete_t& self)
			{
				invocable_t& invocable = *static_cast<invocable_t*>(static_cast<void*>(self.m_data.m_union.m_chars));
				invocable.~invocable_t();
			}
			static return_t execute_small(object_concrete_t& self, typename last_step_transform_t<param_ts>::type&&... params)
			{
				invocable_t& invocable = *static_cast<invocable_t*>(static_cast<void*>(self.m_data.m_union.m_chars));
				return invocable(params...);
			}
		public:
			typedef typename object_concrete_t::funcs_t funcs_t;
			static constexpr funcs_t const s_funcs_small = {&construct_move_small, &construct_copy_small, &destroy_small, &execute_small};
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
				// TODO: Necessary cast?
				invocable_t* const new_invocable = new invocable_t{std::forward<invocable_concrete_t>(invocable)};
				static_assert(sizeof(new_invocable) == sizeof(self.m_data.m_union.m_ptr), "");
				std::memcpy(&self.m_data.m_union.m_ptr, &new_invocable, sizeof(new_invocable));
				//self.m_data.m_union.m_ptr = new_invocable;
				self.m_funcs = &s_funcs_large;
			}
		private:
			static void construct_move_large(object_concrete_t& self, object_concrete_t& other)
			{
				// TODO: Necessary cast?
				invocable_t* const self_invocable = static_cast<invocable_t*>(self.m_data.m_union.m_ptr);
				static_assert(sizeof(self_invocable) == sizeof(other.m_data.m_union.m_ptr), "");
				std::memcpy(&other.m_data.m_union.m_ptr, &self_invocable, sizeof(self_invocable));
				//other.m_data.m_union.m_ptr = self_invocable;
				other.m_funcs = &s_funcs_large;
				self.m_funcs = &executor_base_t<return_t, param_ts...>::s_funcs_nonop;
			}
			static void construct_copy_large(object_concrete_t const& self, object_concrete_t& other)
			{
				// TODO: Necessary cast?
				invocable_t const& self_invocable = *static_cast<invocable_t const*>(self.m_data.m_union.m_ptr);
				invocable_t* const new_invocable = new invocable_t{self_invocable};
				static_assert(sizeof(new_invocable) == sizeof(other.m_data.m_union.m_ptr), "");
				std::memcpy(&other.m_data.m_union.m_ptr, &new_invocable, sizeof(new_invocable));
				//other.m_data.m_union.m_ptr = new_invocable;
				other.m_funcs = &s_funcs_large;
			}
			static void destroy_large(object_concrete_t& self)
			{
				invocable_t* const invocable = static_cast<invocable_t*>(self.m_data.m_union.m_ptr);
				delete invocable;
			}
			static return_t execute_large(object_concrete_t& self, typename last_step_transform_t<param_ts>::type&&... params)
			{
				invocable_t& invocable = *static_cast<invocable_t*>(self.m_data.m_union.m_ptr);
				return invocable(std::forward<typename last_step_transform_t<param_ts>::type>(params)...);
			}
		public:
			typedef typename object_concrete_t::funcs_t funcs_t;
			static constexpr funcs_t const s_funcs_large = {&construct_move_large, &construct_copy_large, &destroy_large, &execute_large};
		};


		template<typename signature_t>
		class type_eraser_impl_t
		{
		};

		template<typename return_t, typename... param_ts>
		class type_eraser_impl_t<return_t(param_ts...)>
		{
		public:
			typedef type_eraser_impl_t<return_t(param_ts...)> self_t;
		public:
			type_eraser_impl_t() /* do not add initializer list*/
			{
				executor_base_t<return_t, param_ts...>::initialize(m_object);
			}
			template<typename invocable_t>
			type_eraser_impl_t(invocable_t&& invocable) :
				type_eraser_impl_t()
			{
				typedef typename remove_ref_t<remove_const_t<invocable_t>::type>::type invocable_nonref_t;
				typedef executor<is_small<invocable_nonref_t>::value, invocable_nonref_t, return_t, param_ts...> executor_t;
				executor_t::construct_forward(m_object, std::forward<invocable_t>(invocable));
			}
			type_eraser_impl_t(copy_construct_tag_t const&, self_t const& other)
			{
				self_t& self = *this;
				other.m_object.m_funcs->m_construct_copy(other.m_object, self.m_object);
			}
			self_t& operator=(self_t const& other)
			{
				self_t& self = *this;
				self.m_object.m_funcs->m_destroy(self.m_object);
				other.m_object.m_funcs->m_construct_copy(other.m_object, self.m_object);
				return *this;
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
			bool op_bool() const
			{
				return !!m_object.m_funcs->m_execute;
			}
			template<typename... param2_ts>
			return_t execute(param2_ts&&... params)
			{
				return m_object.m_funcs->m_execute(m_object, std::forward<param2_ts>(params)...);
			}
		private:
			object_t<return_t, param_ts...> m_object;
		};

	}


	template<typename signature_t>
	class type_eraser_t : private detail::type_eraser_impl_t<signature_t>
	{
	private:
		typedef type_eraser_t<signature_t> self_t;
		typedef detail::type_eraser_impl_t<signature_t> base_t;
	public:
		type_eraser_t() noexcept :
			base_t()
		{
		}
		type_eraser_t(std::nullptr_t) noexcept :
			type_eraser_t()
		{
		}
		template
		<
			typename invocable_t,
			typename = typename detail::enable_if<!detail::is_same<typename detail::remove_ref_t<typename detail::remove_const_t<invocable_t>::type>::type, self_t>::value>::type,
			typename = typename detail::enable_if<detail::is_invocable_t<signature_t, invocable_t>::value>::type
		>
		type_eraser_t(invocable_t&& invocable) :
			base_t(std::forward<invocable_t>(invocable))
		{
		}
		type_eraser_t(self_t const& other) :
			base_t(detail::copy_construct_tag_t{}, other)
		{
		}
		type_eraser_t(self_t&& other) noexcept :
			type_eraser_t()
		{
			this->swap(other);
		}
		self_t& operator=(self_t const& other)
		{
			static_cast<base_t*>(this)->operator=(other);
			return *this;
		}
		self_t& operator=(self_t&& other) noexcept
		{
			this->swap(other);
			return *this;
		}
		void swap(self_t& other) noexcept
		{
			base_t::swap(other);
		}
		~type_eraser_t()
		{
		}
		explicit operator bool() const
		{
			return this->op_bool();
		}
		template<typename... param_ts>
		decltype(auto) operator()(param_ts&&... params)
		{
			return this->execute(std::forward<param_ts>(params)...);
		}
		template<typename... param_ts>
		decltype(auto) operator()(param_ts&&... params) const
		{
			return const_cast<self_t*>(this)->operator()(std::forward<param_ts>(params)...);
		}
	};

	template<typename signature_t> void swap(type_eraser_t<signature_t>& a, type_eraser_t<signature_t>& b) noexcept { a.swap(b); }

	template<typename signature_t, typename invocable_t>
	type_eraser_t<signature_t> make_type_eraser(invocable_t&& invocable)
	{
		return type_eraser_t<signature_t>{std::forward<invocable_t>(invocable)};
	}


}
