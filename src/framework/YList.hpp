/*MIT License

Copyright (c) 2018 Florian GERARD

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Except as contained in this notice, the name of Florian GERARD shall not be used 
in advertising or otherwise to promote the sale, use or other dealings in this 
Software without prior written authorization from Florian GERARD

*/


#pragma once

#include <cstdint>
#include <functional>
#include <atomic>
#include <limits>


namespace framework {
    template<typename Item>
    class YNode {
        template<typename I, typename L>
        friend class YNodeIterator;

        template<typename I, typename L>
        friend class YNodeConstIterator;

        Item *previous_ = nullptr;
        Item *next_ = nullptr;

    public:
        constexpr YNode() = default;
    };

    template<typename Item, typename List = Item>
    class YNodeIterator {
        template<typename I, typename L>
        friend class YList;
    public:
        using self_type = YNodeIterator;
        using value_type = Item;
        using pointer = Item *;
        using reference = Item &;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        constexpr explicit YNodeIterator(pointer ptr = nullptr): item_(ptr) {}

        pointer item() {
            return item_;
        }

        self_type& operator++() {
            item_ = next();
            return *this;
        }

        self_type& operator--() {
            item_ = previous();
            return *this;
        }

        constexpr reference operator*() const {
            return *item_;
        }

        constexpr bool operator!=(const self_type &other) const {
            return item_ != other.item_;
        }

        constexpr bool operator==(const self_type &other) const {
            return item_ == other.item_;
        }

        constexpr pointer operator->() const{
            return item_;
        }

        [[nodiscard]] constexpr bool isValid() const {
            return item_ != nullptr;
        }

        [[nodiscard]] pointer next() const {
            return item_->List::next_;
        }

        pointer next(pointer ptr) {
            return item_->List::next_ = ptr;
        }

        auto next(const self_type it) {
            return next(it.item_);
        }

        [[nodiscard]] pointer previous() const {
            return item_->List::previous_;
        }

        pointer previous(pointer ptr){
            return item_->List::previous_ = ptr;
        }

        auto previous(const self_type it) {
            return previous(it.item_);
        }

    private:
        Item* item_;

        void reset() const {
            item_->List::previous_ = nullptr;
            item_->List::next_ = nullptr;
        }

        [[nodiscard]] bool is_free() const {
            return (item_->List::next_ == nullptr) && (item_->List::previous_ == nullptr);
        }

        [[nodiscard]] pointer ptr() const {
            return item_;
        }
    };

    template<typename Item, typename List = Item>
    class YNodeConstIterator {
        template<typename I, typename L>
        friend class YList;
    public:
        using self_type = YNodeConstIterator;
        using value_type = Item;
        using pointer = Item *;
        using reference = Item &;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        constexpr explicit YNodeConstIterator(pointer ptr = nullptr): item_(ptr) {}

        self_type& operator++() {
            item_ = next();
            return *this;
        }

        self_type& operator--() {
            item_ = previous();
            return *this;
        }

        constexpr reference operator*() const {
            return *item_;
        }

        constexpr bool operator!=(const self_type &other) const{
            return item_ != other.item_;
        }

        constexpr bool operator==(const self_type &other) const {
            return item_ == other.item_;
        }

        constexpr pointer operator->() const{
            return item_;
        }

        [[nodiscard]] constexpr bool isValid() const {
            return item_ != nullptr;
        }

        [[nodiscard]] pointer next() const {
            return item_->List::next_;
        }

        [[nodiscard]] pointer previous() const {
            return item_->List::previous_;
        }

        [[nodiscard]] bool is_free() const {
            return item_->List::next_ == nullptr && item_->List::previous_ == nullptr;
        }

        [[nodiscard]] pointer ptr() const{
            return item_;
        }

    private:
        Item* item_;

    };


    template<typename Item, typename List = Item>
    class YList {
        Item *first_ = nullptr;
        std::atomic<uint32_t> count_ = 0;

        using value_type = Item;
        using size_type = uint32_t;
        using difference_type = std::ptrdiff_t;
        using reference = Item &;
        using const_reference = const Item &;
        using pointer = Item *;
        using const_pointer = const Item *;
        using iterator = YNodeIterator<Item, List>;
        using const_iterator = YNodeConstIterator<Item, List>;
        //using reverse_iterator
        //using const_reverse_iterator

        /*Given comparator function,
        *if return value is >0 compared inferior to base
        *if return value is 0 compared and base are equal
        *if return value is <0 compared superior to base*/
        using Comparator = int8_t (*)(const Item *base, const Item *compared);

    public:

        constexpr YList() = default;

        //Element Access
        constexpr reference front() {
            return *first_;
        }

        constexpr const_reference front() const {
            return *first_;
        }

        //TODO
        constexpr reference back() {
            return *first_;
        }

        //TODO
        constexpr const_reference back() const {
            return *first_;
        }

        //Iterators

        iterator begin() {
            return iterator(first_);
        }

        const_iterator begin() const {
            return const_iterator(first_);
        }

        iterator end() {
            return iterator(nullptr);
        }

        const_iterator end() const {
            return const_iterator(nullptr);
        }


        //Capacity
        [[nodiscard]] constexpr bool empty() const {
            return count_ == 0;
        }

        [[nodiscard]] constexpr size_type size() const {
            return count_;
        }

        [[nodiscard]] constexpr size_type max_size() const {
            return std::numeric_limits<size_type>::max();
        }

        //Modifiers

        void clear() {
            first_ = nullptr;
            count_ = 0;
        }

        void insertAt(iterator pos, pointer item) {
            auto next = iterator(pos.next());
            auto newNode = iterator(item);

            newNode.next(next);
            newNode.previous(pos);

            pos.next(newNode);
            if (next.isValid()) {
                next.previous(newNode);
            }
            count_ = count_ + 1;
        }

        iterator erase(const iterator pos) {
            if (empty()) {
                return iterator(nullptr);
            }

            iterator result;
            if (pos == begin()) {
                first_ = pos.next();
                if (first_ != nullptr) {
                    iterator(first_).previous(nullptr);
                }
                result = iterator(first_);
            } else {
                if (pos.is_free()) {
                    return iterator(nullptr);
                }
                auto previous = iterator(pos.previous());
                auto next = iterator(pos.next());
                previous.next(next);
                if (next.ptr() != nullptr) {
                    next.previous(previous);
                }
                result = next;
            }
            pos.reset();
            count_ = count_ - 1;
            return result;
        }

        pointer erase(pointer item) {
            return erase(iterator(item)).ptr();
        }

        void push_front(pointer item) {
            auto it = iterator(item);
            if (!empty()) {
                it.next(first_);
                iterator(first_).previous(it.ptr());
            }
            first_ = it.ptr();
            count_ = count_ + 1;
        }

        void pop_front() {
            auto it = iterator(first_);
            first_ = it.next();
            it.reset();
            count_ = count_ - 1;
        }

        auto get_and_pop_front() {
            auto res = begin();
            pop_front();
            return res;
        }

        void push_back(pointer item) {
            if (empty()) {
                first_ = item;
                count_ = count_ + 1;
            } else {
                auto it = begin();
                while (it.next() != nullptr) {
                    ++it;
                }
                insertAt(it, item);
            }
        }

        void insertWhen(Item *node, const Comparator predicate) {
            auto it = iterator(node);
            if (first_ == nullptr || predicate(first_, it.item()) > 0) {
                push_front(node);
            } else {
                auto previous = iterator(first_);
                auto current = iterator(previous.next());
                while (current.isValid() && predicate(current.item(), it.item()) < 0) {
                    previous = current;
                    current = iterator(previous.next());
                }
                insertAt(previous, it.item());
            }
        }


        [[nodiscard]] bool contain(Item *node) const{
            if (first_ == nullptr) {
                return false;
            }
            if (node == first_) {
                return true;
            }
            auto it = iterator(first_);
            while (it.next() != nullptr) {
                ++it;
                if (node == it.item()) {
                    return true;
                }
            }
            return false;
        }

    };
}
