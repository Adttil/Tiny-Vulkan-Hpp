#ifndef VKPP_GENERATE_UNTIL_EMPTY_HPP
#define VKPP_GENERATE_UNTIL_EMPTY_HPP

#include <concepts>
#include <utility>
#include <ranges>
#include <optional>

namespace vk
{
    template<class F>
    class generate_until_null_view : std::ranges::view_interface<generate_until_null_view<F>>
    {
    public:
        constexpr generate_until_null_view(F fn)
        : fn_{ (F&&)fn }
        {}

        struct sentinel{};

        class iterator
        {
        public:
            using iterator_concept = std::input_iterator_tag;
            using value_type = decltype(std::declval<const F&>()().value());
            using reference_type = decltype(std::declval<const F&>()().value());
            using difference_type = ptrdiff_t;
            
            constexpr iterator(const generate_until_null_view& parent)
            : parent_{ &parent }
            {
                value_ = parent.fn_();
            }

            constexpr decltype(auto) operator*() const
            {
                return value_.value();
            }

            constexpr iterator& operator++()
            {
                value_ = parent_->fn_();
                return *this;
            }

            constexpr iterator operator++(int)
            {
                auto copy = *this;
                ++*this;
                return copy;
            }

            constexpr bool operator==(sentinel) const
            {
                return not value_.has_value();
            }

        private:
            const generate_until_null_view* parent_;
            decltype(std::declval<const F&>()()) value_;
        };

        constexpr iterator begin() const
        {
            return iterator{ *this };
        }

        constexpr sentinel end() const
        {
            return {};
        }

    private:
        F fn_;
    };

    template<class F>
    generate_until_null_view(F) -> generate_until_null_view<F>;

    namespace detail 
    {
        struct generate_until_null_fn
        {
            template<class F>
            constexpr generate_until_null_view<F> operator()(F fn) const
            {
                return { std::move(fn) };
            }
        };
    }

    inline namespace functors 
    {
        inline constexpr detail::generate_until_null_fn generate_until_null{};
    }
}

#endif