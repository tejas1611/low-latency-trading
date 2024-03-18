/// Referenced https://github.com/CharlesFrasch/cppcon2023/blob/main/Fifo4a.hpp
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

        size_type mask_ = 0;

        T* store_;
        std::atomic<size_type> next_read_index_ = {0};
        std::atomic<size_type> next_write_index_ = {0};
        size_type next_read_index_cached_ = 0;
        size_type next_write_index_cached_ = 0;

        static_assert(std::atomic<size_type>::is_always_lock_free);

    public:
        explicit LFQueue(size_type capacity, const Alloc& alloc = Alloc()) : Alloc{alloc}, mask_(capacity - 1), 
                store_(allocator_traits::allocate(*this, capacity)) {}

        ~LFQueue() {
            while (!empty()) {
                element(next_read_index_)->~T();
                ++next_read_index_;
            }
            allocator_traits::deallocate(*this, store_, capacity());
        }

        bool push(const T& value) {
            auto next_write_index = next_write_index_.load(std::memory_order_relaxed);
            if (full(next_write_index, next_read_index_cached_)) {
                next_read_index_cached_ = next_read_index_.load(std::memory_order_acquire);
                
                if (full(next_write_index, next_read_index_cached_)) return false;
            }

            new(element(next_write_index)) T(value);   // placement new
            next_write_index_.store(next_write_index + 1, std::memory_order_release);
            return true;
        }

        bool pop(T& value) {
            auto next_read_index = next_read_index_.load(std::memory_order_relaxed);
            if (empty(next_write_index_cached_, next_read_index)) {
                next_write_index_cached_ = next_write_index_.load(std::memory_order_acquire);
                
                if (empty(next_write_index_cached_, next_read_index)) return false;
            }

            value = *element(next_read_index);
            element(next_read_index)->~T();
            next_read_index_.store(next_read_index + 1, std::memory_order_release);
            return true;
        }

        auto size() const noexcept {
            ASSERT(next_read_index_ <= next_write_index_, "Invalid LFQueue pointers in:" + std::to_string(pthread_self()));
            return next_write_index_ - next_read_index_;
        }

        auto capacity() const noexcept {
            return 1 + mask_;
        }

        bool full() const noexcept {
            return size() == capacity();
        }

        bool empty() const noexcept {
            return next_read_index_ == next_write_index_;
        }

        LFQueue() = delete;
        LFQueue(const LFQueue&) = delete;
        LFQueue(const LFQueue&&) = delete;
        LFQueue& operator=(const LFQueue&) = delete;
        LFQueue& operator=(const LFQueue&&) = delete;

    private:
        T* element(size_type location) noexcept {
            return &store_[mask_ & location];
        }

        bool empty(size_type write_index, size_type read_index) noexcept {
            return write_index == read_index;
        }

        bool full(size_type write_index, size_type read_index) noexcept {
            return (write_index - read_index) == capacity();
        }
    };
}