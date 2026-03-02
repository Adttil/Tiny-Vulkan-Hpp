#ifndef TINY_VULKAN_HPP_ROLLBACK
#define TINY_VULKAN_HPP_ROLLBACK

#include <functional>

#include <vulkan/vulkan_core.h>

#include "tuple.hpp"

namespace vk 
{
    //template 
    namespace detail 
    {
        struct guard_fn;
    }
    
    struct detail::guard_fn
    {
        constexpr bool operator()(VkResult result) const
        {
            return result == VK_SUCCESS;
        }

        template<class T>
        constexpr bool operator()(T&& t) const
        {
            return (bool)(T&&)t;
        }
    };

    inline namespace functors 
    {
        inline constexpr detail::guard_fn guard{};
    }
}

namespace vk::transaction_ns
{
    template<class S, class R>
    struct transaction_t
    {
        S s;
        R r;

        template<class...Args>
        constexpr decltype(auto) submit(Args&&...args) const 
        noexcept(noexcept(std::invoke(s, (Args&&)args...)))
        {
            return std::invoke(s, (Args&&)args...);
        }

        template<class...Args>
        constexpr void rollback(Args&&...args) const noexcept
        {
            std::invoke(r, (Args&&)args...);
        }
    };

    template<class L, class R>
    struct transaction_pipeline
    {
        L l;
        R r;

        template<class...Args>
        constexpr decltype(auto) submit(Args&&...args) const noexcept
        requires (noexcept(l.submit(args...)))
        {
            if(decltype(auto) rl = l.submit(args...); not guard(rl))
            {
                return rl;
            }
            decltype(auto) rr = r.submit(args...);
            if(not guard(rr))
            {
                l.rollback((Args&&)args...);
            }   
            return rr;
        }

        template<class...Args>
        constexpr void submit(Args&&...args) const
        requires (not noexcept(l.submit(args...)))
        {
            l.submit(args...);
            try
            {
                r.submit(args...);
            }
            catch(std::exception& e)
            {
                l.rollback((Args&&)args...);
                throw e;
            }
        }

        template<class...Args>
        constexpr void rollback(Args&&...args) const
        {
            r.rollback(args...);
            l.rollback((Args&&)args...);
        }
    };

    template<class L, class R>
    constexpr transaction_pipeline<L, R> operator|(L l, R r)
    {
        return { l, r };
    }
}

namespace vk 
{
    namespace detail 
    {
        struct transaction_fn;
        //struct rollback_fn;
    }

    struct detail::transaction_fn
    {
        template<class S, class R>
        constexpr transaction_ns::transaction_t<S, R> operator()(S s, R r) const
        {
            return { s, r };
        }

        static constexpr auto do_nothing = [](auto&&...){};
        using do_thing_t = std::remove_const_t<decltype(do_nothing)>;

        template<class S>
        constexpr transaction_ns::transaction_t<S, do_thing_t> operator()(S s) const
        {
            return { s, {} };
        }
    };

    inline namespace functors 
    {
        inline constexpr detail::transaction_fn transaction{};
    }
}

#endif