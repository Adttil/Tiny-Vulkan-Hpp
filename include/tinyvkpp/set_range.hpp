#ifndef VKPP_SET_RANGE_HPP
#define VKPP_SET_RANGE_HPP

#include <ranges>

namespace vk
{
    namespace detail {
        struct set_range_fn;
    }
    
    struct detail::set_range_fn
    {
        template<class T, class TCount>
        constexpr auto operator()(T&& t, const TCount& count) const
        {
            if constexpr(std::ranges::contiguous_range<T>)
            {
                const_cast<TCount&>(count) = static_cast<TCount>(std::ranges::size(t));
                return std::ranges::data(t);
            }
            else if constexpr(std::is_lvalue_reference_v<T&&>)
            {
                const_cast<TCount&>(count) = static_cast<TCount>(1);
                return &t;
            }
            else
            {
                static_assert(false, "vk::set_range: T must be a contiguous range or an lvalue reference");
            }
        }
    };

    inline namespace functors
    {
        inline constexpr detail::set_range_fn set_range{};
    }
}

#endif