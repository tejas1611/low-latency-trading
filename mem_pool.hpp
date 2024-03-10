#pragma once
#include <vector>
#include <cstdint>
#include <string>

#include "macros.hpp"

namespace Common {
    template <typename T>
    class MemPool final {
    private:
        class ObjectBlock {
        public:
            T object_;
            bool is_free = true;
        };

        // Can use array instead of vector for small size pools
        std::vector<ObjectBlock> store_;
        std::size_t next_free_index_ = 0;

        void updateNextFreeIndex() noexcept {
            const auto initial_free_index = next_free_index_;
            while (!store_[next_free_index_].is_free) {
                ++next_free_index_;
                if (next_free_index_ == store_.size()) [[unlikely]] {
                    next_free_index_ = 0;
                }
                if (next_free_index_ == initial_free_index) [[unlikely]] {
                    ASSERT(initial_free_index != next_free_index_, "Memory Pool out of space.");
                }
            }
        }

    public:
        explicit MemPool (std::size_t size) : store_(size, { T(), true }) {
            ASSERT(reinterpret_cast<const ObjectBlock *>(&(store_[0].object_)) == &(store_[0]), "T object should be first member of ObjectBlock.");
        }

        template<typename... Args>
        T* allocate(Args... args) noexcept {
            auto obj_block = &(store_[next_free_index_]);
            ASSERT(obj_block->is_free, "Expected free ObjectBlock at index:" + std::to_string(next_free_index_));

            T* ret = &(obj_block->object_);
            ret = new(ret) T(args...);  // placement new (doesn't allocate memory)
            obj_block->is_free = false;
            updateNextFreeIndex();
            return ret;
        }

        auto deallocate(const T* elem) noexcept {
            const auto elem_index = reinterpret_cast<const ObjectBlock*>(elem) - &(store_[0]);
            ASSERT(elem_index >= 0 && static_cast<std::size_t>(elem_index) < store_.size(), "Element being deallocated does not belong to this Memory pool.");
            ASSERT(!store_[elem_index].is_free, "Expected in-use ObjectBlock at index:" + std::to_string(elem_index));
            store_[elem_index].is_free = true;
        }

        MemPool() = delete;
        MemPool(const MemPool&) = delete;
        MemPool(const MemPool&&) = delete;
        MemPool& operator=(const MemPool&) = delete;
        MemPool& operator=(const MemPool&&) = delete;
    };
}