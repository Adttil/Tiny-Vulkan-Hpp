#ifndef VKPP_FUNCTION_TRAIT_HPP
#define VKPP_FUNCTION_TRAIT_HPP

#include <concepts>
#include <utility>
#include <stdexcept>

#include "tuple.hpp"

namespace vk 
{
    template<class T, T F>
    struct fn_info_impl;

    template<class R, class...Args, R(*F)(Args...)>
    struct fn_info_impl<R(*)(Args...), F>
    {
        using return_type = R;
        
        static constexpr size_t arg_count = sizeof...(Args);

        using arg_types = tuple<std::type_identity<Args>...>;

        using arg_pack_type = tuple<Args...>;

        template<size_t I>
        using arg_type = arg_pack_type::template type_at<I>;
    };

    template<auto F>
    struct fn_info : fn_info_impl<decltype(F), F>{};
}

#endif