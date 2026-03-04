#ifndef VKPP_TUPLE_HPP
#define VKPP_TUPLE_HPP

#include <type_traits>
#include <utility>
#include <functional>

namespace vk 
{
    template<size_t I, class T>
    struct tuple_unit
    {
        T value;

        constexpr T& get(std::integral_constant<size_t, I> = {})& noexcept
        {
            return value;
        }

        constexpr const T& get(std::integral_constant<size_t, I> = {}) const& noexcept
        {
            return value;
        }

        constexpr T&& get(std::integral_constant<size_t, I> = {})&& noexcept
        {
            return (T&&)value;
        }

        constexpr const T&& get(std::integral_constant<size_t, I> = {}) const&& noexcept
        {
            return (const T&&)value;
        }

        constexpr T& get(std::type_identity<T> = {})& noexcept
        {
            return value;
        }

        constexpr const T& get(std::type_identity<T> = {}) const& noexcept
        {
            return value;
        }

        constexpr T&& get(std::type_identity<T> = {})&& noexcept
        {
            return std::move(value);
        }

        constexpr const T&& get(std::type_identity<T> = {}) const&& noexcept
        {
            return std::move(value);
        }

        static consteval std::type_identity<T> type_identity(std::integral_constant<size_t, I> = {}) noexcept
        {
            return {};
        }

        static consteval size_t index(std::type_identity<T> = {}) noexcept
        {
            return I;
        } 
    };

    template<class Seq, class...T>
    struct tuple_impl;

    template<size_t...I, class...T>
    struct tuple_impl<std::index_sequence<I...>, T...> : tuple_unit<I, T>...
    {
        using tuple_unit<I, T>::get...;
        using tuple_unit<I, T>::type_identity...;
        using tuple_unit<I, T>::index...;

        static consteval size_t size() noexcept
        {
            return sizeof...(T);
        }

        template<size_t J, class Self>
        constexpr auto&& get(this Self&& self) noexcept
        {
            return ((Self&&)self).get(std::integral_constant<size_t, J>{});
        }

        // template<size_t J>
        // constexpr auto&& get()& noexcept
        // {
        //     return (*this).get(std::integral_constant<size_t, J>{});
        // }

        // template<size_t J>
        // constexpr auto&& get() const& noexcept
        // {
        //     return (*this).get(std::integral_constant<size_t, J>{});
        // }

        // template<size_t J>
        // constexpr auto&& get()&& noexcept
        // {
        //     return std::move(*this).get(std::integral_constant<size_t, J>{});
        // }

        // template<size_t J>
        // constexpr auto&& get() const&& noexcept
        // {
        //     return std::move(*this).get(std::integral_constant<size_t, J>{});
        // }

        template<class U, class Self>
        constexpr auto&& get(this Self&& self) noexcept
        {
            return ((Self&&)self).get(std::type_identity<U>{});
        }

        // template<class U>
        // constexpr auto&& get()& noexcept
        // {
        //     return (*this).get(std::type_identity<U>{});
        // }

        // template<class U>
        // constexpr auto&& get() const& noexcept
        // {
        //     return (*this).get(std::type_identity<U>{});
        // }

        // template<class U>
        // constexpr auto&& get()&& noexcept
        // {
        //     return std::move(*this).get(std::type_identity<U>{});
        // }

        // template<class U>
        // constexpr auto&& get() const&& noexcept
        // {
        //     return std::move(*this).get(std::type_identity<U>{});
        // }

        template<size_t J>
        static consteval auto type_identity() noexcept
        {
            return type_identity(std::integral_constant<size_t, J>{});
        }

        template<size_t J>
        using type_at = decltype(tuple_impl::type_identity<J>())::type;

        template<class U>
        static consteval size_t index() noexcept
        {
            return index(std::type_identity<U>{});
        }
    };

    namespace tuple_ns 
    {
        template<class...T>
        struct tuple : tuple_impl<std::index_sequence_for<T...>, T...>{};

        template<class...T>
        tuple(T...) -> tuple<T...>;

        template<size_t I, class Self>
        constexpr auto&& get(Self&& self) noexcept
        {
            return ((Self&&)self).template get<I>();
        }

        template<class T, class Self>
        constexpr auto&& get(Self&& self) noexcept
        {
            return ((Self&&)self).template get<T>();
        }

        constexpr tuple<> tuple_cat() noexcept
        {
            return {};
        }

        template<class Tpl>
        constexpr auto tuple_cat(Tpl&& tpl)
        {
            return (Tpl&&)tpl;
        }

        template<class...T>
        constexpr tuple<std::remove_const_t<T>...> pass_as_tuple(T&&...t)
        {
            return { (T&&)t... };
        }

        template<class Tpl1, class Tpl2>
        constexpr auto tuple_cat(Tpl1&& tpl1, Tpl2&& tpl2)
        {
            return [&]<size_t...I1, size_t...I2>(std::index_sequence<I1...>, std::index_sequence<I2...>)
            {
                return pass_as_tuple( 
                    get<I1>((Tpl1&&)tpl1)..., get<I2>((Tpl2&&)tpl2)... 
                );
            }(std::make_index_sequence<Tpl1::size()>{}, std::make_index_sequence<Tpl2::size()>{});
        }

        template<class Tpl1, class Tpl2, class...Rest>
        constexpr auto tuple_cat(Tpl1&& tpl1, Tpl2&& tpl2, Rest&&...rest)
        {
            return tuple_cat(tuple_cat((Tpl1&&)tpl1, (Tpl2&&)tpl2), (Rest&&)rest...);
        }

        template<class F, class Tpl>
        constexpr decltype(auto) apply(F&& f, Tpl&& tpl)
        {
            return [&]<size_t...I>(std::index_sequence<I...>) -> decltype(auto)
            {
                return std::invoke((F&&)f, get<I>((Tpl&&)tpl)...);
            }(std::make_index_sequence<Tpl::size()>{});
        }
    }
    using tuple_ns::tuple;
    //using tuple_ns::get;
    using tuple_ns::tuple_cat;

    template<tuple Tpl>
    inline constexpr auto constant_tuple = []<size_t...I>(std::index_sequence<I...>)
    {
        return tuple<std::integral_constant<std::decay_t<decltype(get<I>(Tpl))>, get<I>(Tpl)>...>{};
    }(std::make_index_sequence<Tpl.size()>{});
    
    constexpr auto t = constant_tuple<tuple{1, (short)0}>;
}

namespace vk 
{

}



template<class...T>
struct std::tuple_size<vk::tuple<T...>> : std::integral_constant<size_t, sizeof...(T)>{};

template<size_t I, class...T>
struct std::tuple_element<I, vk::tuple<T...>> : std::type_identity<typename vk::tuple<T...>::template type_at<I>>{};

#endif