#pragma once

#include <cassert>
#include <atomic>
#include <vector>

#include "macros.hpp"

namespace Common {
    template<typename T, typename Alloc = std::allocator<T>>
    class LFQueue final : private Alloc {
    private:
        using allocator_traits = std::allocator_traits<Alloc>;
        using size_type = typename allocator_traits::size_type;

        size_type capacity_ = 0;

        T* store_;
        std::atomic<size_type> next_read_index_ = {0};
        std::atomic<size_type> next_write_index_ = {0};

        static_assert(std::atomic<size_type>::is_always_lock_free);

    public:
        explicit LFQueue(size_type capacity, const Alloc& alloc) : Alloc{alloc}, capacity_(capacity), 
                store_(allocator_traits::allocate(*this, capacity)) {}

        ~LFQueue() {
            while (!empty()) {
                store_[next_read_index_ % capacity_].~T();
                ++next_read_index_;
            }
            allocator_traits::deallocate(*this, store_, capacity_);
        }

        bool push(const T& value) {
            if (full()) return false;

            new(&store_[next_write_index_ % capacity_]) T(value);   // placement new
            ++next_write_index_;
            return true;
        }

        bool pop(T& value) {
            if (empty()) return false;
            
            value = store_[next_read_index_ % capacity_];
            store_[next_read_index_ % capacity_].~T();
            ++next_read_index_;
            return true;
        }

        auto size() const noexcept {
            ASSERT(next_read_index_ <= next_write_index_, "Invalid LFQueue pointers in:" + std::to_string(pthread_self()));
            return next_write_index_ - next_read_index_;
        }

        auto capacity() const noexcept {
            return capacity_;
        }

        bool full() const noexcept {
            return size() == capacity_;
        }

        bool empty() const noexcept {
            return next_read_index_ == next_write_index_;
        }

        LFQueue() = delete;
        LFQueue(const LFQueue&) = delete;
        LFQueue(const LFQueue&&) = delete;
        LFQueue& operator=(const LFQueue&) = delete;
        LFQueue& operator=(const LFQueue&&) = delete;
    };
}