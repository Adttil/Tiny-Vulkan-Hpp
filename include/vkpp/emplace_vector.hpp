#ifndef VKPP_EMPLACE_VECTOR_HPP
#define VKPP_EMPLACE_VECTOR_HPP

#include <functional>

namespace vk 
{
    template<class T, size_t Capacity_>
    class emplace_vector
    {
    public:

        constexpr T* begin()
        {
            return reinterpret_cast<T*>(storage);
        }

        constexpr const T* begin() const
        {
            return reinterpret_cast<const T*>(storage);
        }

        constexpr T* end()
        {
            return begin() + Capacity_;
        }

        constexpr const T* end() const
        {
            return begin() + Capacity_;
        }
        
        constexpr T& operator[](size_t offset)
        {
            return begin()[offset];
        }

        constexpr const T& operator[](size_t offset) const
        {
            return begin()[offset];
        }

        constexpr size_t size() const
        {
            return size_;
        }

        constexpr bool empty() const
        {
            return not size(); 
        }

        constexpr void push_back(const T& t)
        {
            std::construct_at(begin() + size_, t);
            ++size_;
        }

        template<class Self>
        constexpr auto& back(this Self& self)
        {
            return self[self.size() - 1];
        }

        constexpr void pop_back()
        {
            --size_;
            std::destroy_at(begin() + size_);
        }

    private:
        size_t size_ = 0;
        alignas(alignof(T)) unsigned char storage[sizeof(T) * Capacity_];
    };
}

#endif