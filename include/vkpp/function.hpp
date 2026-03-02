#ifndef VKPP_FUNCTION_HPP
#define VKPP_FUNCTION_HPP

#include <vulkan/vulkan_core.h>

#include <concepts>
#include <utility>
#include <stdexcept>
#include <print>

#include "soo_vector.hpp"
#include "result.hpp"
#include "tuple.hpp"
#include "function_trait.hpp"

namespace vk 
{
    template<auto F>
    struct enumerate_fn
    {
        using info = fn_info<F>;
        using element_type = std::remove_pointer_t<class info::template arg_type<info::arg_count - 1>>;

        template<class...Args> requires std::same_as<typename info::return_type, VkResult>
        Result<soo_vector<element_type>> operator()(Args&&...args) const
        {
            uint32_t n;
            VkResult result = F(std::forward<Args>(args)..., &n, nullptr);
            if(result != VK_SUCCESS)
            {
                return result;
            }
            soo_vector<element_type> elems(n);
            result = F(std::forward<Args>(args)..., &n, elems.data());
            if(result != VK_SUCCESS)
            {
                return result;
            }
            return elems;
        }

        template<class...Args> requires std::same_as<typename info::return_type, void>
        soo_vector<element_type> operator()(Args&&...args) const
        {
            uint32_t n;
            F(std::forward<Args>(args)..., &n, nullptr);
            soo_vector<element_type> elems(n);
            F(std::forward<Args>(args)..., &n, elems.data());
            return elems;
        }
    };

    inline namespace functors 
    {
        template<auto F>
        inline constexpr enumerate_fn<F> enumerate{};
    }

    template<auto F>
    struct get_fn
    {
        using info = fn_info<F>;
        using result_type = std::remove_pointer_t<class info::template arg_type<info::arg_count - 1>>;
        using code_type = info::return_type;

        template<class...Args> requires std::same_as<code_type, void>
        result_type operator()(Args&&...args) const
        {
            result_type t;
            F(std::forward<Args>(args)..., &t);
            return t;
        }

        template<class...Args> requires std::same_as<code_type, VkResult>
        Result<result_type> operator()(Args&&...args) const
        {
            result_type t;
            VkResult code = F(std::forward<Args>(args)..., &t);
            if(code != VK_SUCCESS)
            {
                return code;
            }
            return t;
        }
    };

    inline namespace functors 
    {
        template<auto F>
        inline constexpr get_fn<F> get{};
    }

    template<auto F>
    struct enhance_fn
    {
        template<class...Args>
        constexpr decltype(auto) operator()(Args&&...args) const
        {
            using info = fn_info<F>;
            if constexpr(not requires{ 
                requires not std::is_const_v<std::remove_pointer_t<
                    class info::template arg_type<info::arg_count - 1>
                >>; 
            })
            {
                return F((Args&&)args...);
            }
            else if constexpr(not requires{ 
                requires std::unsigned_integral<
                    class info::template arg_type<info::arg_count - 2>
                >; 
            })
            {
                return get<F>((Args&&)args...);
            }
            else
            {
                return enumerate<F>((Args&&)args...);
            }
            
        }
    };

    inline namespace functors 
    {
        template<auto F>
        inline constexpr enhance_fn<F> enhance{};
    }
}

#endif