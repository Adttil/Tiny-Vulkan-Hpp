#ifndef VKPP_RESULT_HPP
#define VKPP_RESULT_HPP

#include <vulkan/vulkan_core.h>

#include <concepts>
#include <utility>

#include "soo_vector.hpp"

namespace vk 
{
    template<class T>
    class Result
    {
    public:
        template<class...Args> requires std::constructible_from<T, Args...>
        constexpr Result(Args&&...args) noexcept
        : value_{ (Args&&)args... }, err_code_{ VK_SUCCESS }
        {}

        constexpr Result(VkResult err_code) noexcept
        : err_code_{ err_code }
        {}

        constexpr bool has_value() const noexcept
        {
            return err_code_ == VK_SUCCESS;
        }

        constexpr operator bool()const noexcept
        {
            return has_value();
        }

        template<class Self>
        constexpr auto&& value(this Self&& self) noexcept
        {
            return std::forward_like<Self>(self.value_);
        }

        constexpr VkResult err_code()const noexcept
        {
            return err_code_;
        }

        template<class F, class Self>
        constexpr auto&& value_or_deal(this Self&& self, F&& f)
        {
            if(self.has_value())
            {
                return ((Self&&)self).value();
            }
            else if constexpr(std::convertible_to<decltype(f(self.err_code())), decltype(((Self&&)self).value())>)
            {
                return f(self.err_code);
            }
            else
            {
                static_assert(not noexcept(f(self.err_code())));
                f(self.err_code());// must throw
                std::unreachable();
            }
        }

        template<class E, class Self>
        constexpr auto&& value_or_throw(this Self&& self, E&& e)
        {
            if(self.has_value())
            {
                return ((Self&&)self).value();
            }
            
            throw e;
        }

    private:
        T value_;
        VkResult err_code_;
    };
}

#endif