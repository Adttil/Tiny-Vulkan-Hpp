#ifndef VKPP_SOO_VECTOR_HPP
#define VKPP_SOO_VECTOR_HPP

#include <functional>
#include <memory>
#include <print>

namespace vk 
{
    template<class T, size_t N_ = (64uz - sizeof(size_t)) / sizeof(T), class TAlloc = std::allocator<T>>
    class soo_vector
    {
    public:
        constexpr soo_vector() noexcept 
        : size_{ 0 }
        {}

        constexpr explicit soo_vector(size_t size)
        : size_{ size }
        {
            if(size > N_)
            {
                std::construct_at((std::vector<T, TAlloc>*)storage);
                ((std::vector<T, TAlloc>*)storage)->resize(size);
            }
        }

        constexpr soo_vector(const soo_vector& other)
        : size_{ other.size_ }
        {
            if(size_ <= N_)
            {
                std::memcpy(&storage, &other.storage, sizeof(T) * N_);
            }
            else
            {
                std::construct_at((std::vector<T, TAlloc>*)storage, *(const std::vector<T, TAlloc>*)other.storage);
            }
        }

        constexpr soo_vector(soo_vector&& other) noexcept
        : size_{ other.size_ }
        {
            if(size_ <= N_)
            {
                std::memcpy(&storage, &other.storage, sizeof(T) * N_);
            }
            else
            {
                std::construct_at((std::vector<T, TAlloc>*)storage, std::move(*(std::vector<T, TAlloc>*)other.storage));
            }
        }

        constexpr T* data()
        {
            if(size_ <= N_)
            {
                return reinterpret_cast<T*>(storage);
            }
            return &*((std::vector<T, TAlloc>*)storage)->begin();
        }

        constexpr const T* data() const
        {
            if(size_ <= N_)
            {
                return reinterpret_cast<const T*>(storage);
            }
            return &*((const std::vector<T, TAlloc>*)storage)->begin();
        }

        constexpr T* begin()
        {
            return data();
        }

        constexpr const T* begin() const
        {
            return data();
        }

        constexpr T* end()
        {
            return begin() + size_;
        }

        constexpr const T* end() const
        {
            return begin() + size_;
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

        ~soo_vector() noexcept
        {
            if(size_ > N_)
            {
                std::destroy_at((std::vector<T, TAlloc>*)storage);
            }
        }

    private:
        size_t size_ = 0;
        alignas(std::max(alignof(T), alignof(std::vector<T, TAlloc>))) 
        unsigned char storage[std::max(sizeof(T) * N_, sizeof(std::vector<T, TAlloc>))];
    };
}

#endif