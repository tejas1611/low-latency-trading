#pragma once
#include <atomic>
#include <vector>
#include <iostream>

#include "macros.hpp"

namespace Common {
    template<typename T>
    class LFQueue final {
    private:
        std::vector<T> store_;
        std::atomic<std::size_t> next_read_index_ = {0};
        std::atomic<std::size_t> next_write_index_ = {0};
        std::atomic<std::size_t> num_elements_ = {0};

    public:
        explicit LFQueue(std::size_t size) : store_(size, T()) {}

        T* getNextToWrite() noexcept {
            return &(store_[next_write_index_]);
        }

        void updateWriteIndex() noexcept {
            next_write_index_ = (next_write_index_ + 1) % store_.size();
            ++num_elements_;
        }

        T* getNextToRead() noexcept {
            return next_read_index_ == next_write_index_ ? nullptr : &(store_[next_read_index_]);
        }

        void updateReadIndex() noexcept {
            next_read_index_ = (next_read_index_ + 1) % store_.size();
            ASSERT(num_elements_ != 0, "Read an invalid element in:" + std::to_string(pthread_self()));
            --num_elements_;
        }

        std::size_t size() const noexcept {
            return num_elements_.load();
        }

        LFQueue() = delete;
        LFQueue(const LFQueue&) = delete;
        LFQueue(const LFQueue&&) = delete;
        LFQueue& operator=(const LFQueue&) = delete;
        LFQueue& operator=(const LFQueue&&) = delete;
    };
}