#ifndef __STACK_H__
#define __STACK_H__

#include <cassert>
#include <sys/types.h>
#include <utility>
#include <map>
#include <vector>
#include <memory>
#include <list>
#include <stack>
#include <stdexcept>
#include <iostream>
#include <iterator>

namespace cxx {
    using std::shared_ptr;

    template<typename K, typename V>
    class stack {
        public:
            // 1 P
            stack();
            ~stack() = default;
            stack(stack const &other);
            stack(stack &&other) noexcept;
            stack& operator=(stack other);
            // 2 P
            void push(K const &key, V const &val);
            // 3 S
            void pop();
            void pop(K const &key);
            // 4 P
            std::pair<K const &, V &> front();
            std::pair<K const &, V const &> front() const;
            V & front(K const &key);
            V const & front(K const &key) const;
            // 5 P
            size_t size() const noexcept;
            size_t count(K const &key) const;
            void clear() noexcept;
            
            // S
            class const_iterator {
                using itnl_itr = typename std::map<K, uint64_t>::const_iterator; // TODO: key_id_t
                using itnl_itr_ptr = shared_ptr<itnl_itr>;

            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type = std::ptrdiff_t;
                using value_type = K;
                using pointer = K*;
                using reference = K&;

                const_iterator(itnl_itr);
                const_iterator();
                const_iterator(const_iterator const&);
                const_iterator(const_iterator&&);
                const_iterator& operator=(const const_iterator&);
                const_iterator& operator ++();
                const_iterator operator ++(int);
                const K& operator *() const noexcept;
                const K* operator ->() const noexcept;
                bool operator ==(const const_iterator & other) const noexcept;
                bool operator !=(const const_iterator & other) const noexcept;
            private:
                itnl_itr_ptr iter;

            };
        const_iterator cbegin();
        const_iterator cend();
            
        private:
            using key_id_t      = size_t;
            using key_it_t      = typename std::map<K, key_id_t>::iterator;
            using stack_t       = std::list<std::pair<key_it_t, V>>;
            using stack_it_t    = typename std::list<std::pair<key_it_t, V>>::iterator;
            using key_stack_t   = std::list<typename stack_t::iterator>;

            // setting this member to false indicates that some non-const
            // references to stack data may exist so making shallow copy
            // would be unsafe
            bool can_share_data = true;
            // 6 S
            struct data_struct;
            shared_ptr<data_struct> data;

    };

    template<typename K, typename V>
    struct stack<K, V>::data_struct {

        std::map<K, key_id_t> keys;
        stack_t stack;
        std::map<key_id_t, key_stack_t> key_stacks;
        size_t next_key_id;

        data_struct(): next_key_id(1) {}

        data_struct(std::map<K, key_id_t>& keys, stack_t& stack, size_t next_key_id)
            : keys(keys),
              stack(),
              next_key_id(next_key_id) {

            // reconstructing stack and key_stacks
            for (auto it = stack.begin(); it != stack.end(); it++) {
                key_it_t new_key_it = this->keys.find(it->first->first);
                this->stack.push_back({new_key_it, it->second});
            }

            for (auto it = this->stack.begin(); it != this->stack.end(); it++) {
                this->key_stacks[it->first->second].push_back(it);
            }
        }

        shared_ptr<data_struct> duplicate() {
            return std::make_shared<data_struct>(keys, stack, next_key_id);
        }
    };

    template<typename K, typename V>
    stack<K, V>::stack() : data(std::make_shared<data_struct>()) {}

    template<typename K, typename V>
    stack<K, V>::stack(const stack &other) {
        if (!other.can_share_data) {
            data = other.data->duplicate();
        } else {
            data = other.data;
        }
    }

    template<typename K, typename V>
    stack<K, V>::stack(stack &&other) noexcept : can_share_data(other.can_share_data), data(std::move(other.data)) {}

    template<typename K, typename V>
    stack<K, V> &stack<K, V>::operator=(stack other) {
        std::swap(data, other.data);
        std::swap(can_share_data, other.can_share_data);
        return *this;
    }

    template<typename K, typename V>
    void stack<K, V>::push(const K &key, const V &value) {
        shared_ptr<data_struct> copied_data = modifiable_data();
        key_id_t id;
        key_it_t keys_iter;
        typename std::map<key_id_t, key_stack_t>::iterator aux_list_iter;
        bool del_from_keys = false;
        bool del_from_aux_list = false;

        if (copied_data->keys.contains(key)) {
            id = copied_data->keys[key];
        } else {
            try {
                id = copied_data->next_key_id;
                keys_iter = copied_data->keys.insert({key, id}).first;
                del_from_keys = true;
                aux_list_iter = copied_data->key_stacks.insert({id, key_stack_t()}).first;
                del_from_aux_list = true;
            }
            catch (...) {
                if (del_from_keys)
                    copied_data->keys.erase(keys_iter);
                if (del_from_aux_list)
                    copied_data->key_stacks.erase(aux_list_iter);
                throw;
            }
        }

        bool remove_from_main_list = false;
        stack_it_t main_list_iter;
        try {
            main_list_iter = copied_data->stack.insert(copied_data->stack.begin(), {keys_iter, value});
            remove_from_main_list = true;
            auto it = copied_data->stack.begin();
            copied_data->key_stacks[id].push_front(it);
            if (del_from_keys)
                ++copied_data->next_key_id;
        } catch (...) {
            if (remove_from_main_list)
                copied_data->stack.erase(main_list_iter);
            copied_data->keys.erase(keys_iter);
            copied_data->key_stacks.erase(aux_list_iter);

            throw;
        }

        data = copied_data;
        can_share_data = true;
    }

    template<typename K, typename V>
    void stack<K, V>::pop() {
        if (data->stack.empty())
            throw std::invalid_argument("cannot pop from an empty stack");

        shared_ptr<data_struct> working_data = modifiable_data();

        auto key_it = working_data->stack.front().first;
        remove_element(working_data, working_data->stack.begin(), key_it);

        data = working_data;
        can_share_data = true;
    }

    template<typename K, typename V>
    void stack<K, V>::pop(const K &key) {
        if (!data->keys.contains(key))
            throw std::invalid_argument("no elem with the given key");

        shared_ptr<data_struct> working_data = modifiable_data();

        key_it_t key_it = working_data->keys.find(key);
        key_id_t key_id = key_it->second;
        stack_it_t stack_it = working_data->key_stacks[key_id].front();
        remove_element(working_data, stack_it, key_it);

        data = working_data;
        can_share_data = true;
    }

    template<typename K, typename V>
    std::pair<K const&, V&> stack<K, V>::front() {
        shared_ptr<data_struct> working_data = modifiable_data();

        auto& [key_iterator, value] = working_data->stack.front();
        K const& key = key_iterator->first;

        data = working_data;
        can_share_data = false;
        return {key, value};
    }

    template<typename K, typename V>
    std::pair<const K &, const V &> stack<K, V>::front() const {
        auto const& [key_iterator, value] = data->stack.front();
        K const& key = key_iterator->first;
        return {key, value};
    }

    template<typename K, typename V>
    V &stack<K, V>::front(const K &key) {
        shared_ptr<data_struct> working_data = modifiable_data();

        if (!data->keys.contains(key)) {
            throw std::invalid_argument("no element with the given key");
        }

        key_id_t key_id = data->keys[key];
        V& val = data->key_stacks[key_id].front()->second;

        data = working_data;
        can_share_data = false;
        return val;
    }

    template<typename K, typename V>
    V const &stack<K, V>::front(const K &key) const {
        if (!data->keys.contains(key)) {
            throw std::invalid_argument("no element with the given key");
        }

        key_id_t key_id = data->keys[key];
        return data->key_stacks[key_id].front()->second;
    }

    template<typename K, typename V>
    size_t stack<K, V>::size() const noexcept {
        return data->stack.size();
    }

    template<typename K, typename V>
    size_t stack<K, V>::count(const K &key) const {
        if (!data->keys.contains(key))
            return 0;
        key_id_t id = data->keys[key];
        return data->key_stacks[id].size();
    }

    template<typename K, typename V>
    void stack<K, V>::clear() noexcept {
        data = std::make_shared<data_struct>();
    }

    template<typename K, typename V>
    shared_ptr<typename stack<K, V>::data_struct> stack<K, V>::modifiable_data() {
        if (data.use_count() > 1)
            return data->duplicate();
        return data;
    }

    // Works for removing only the first element with a given key.
    template<typename K, typename V>
    void stack<K, V>::remove_element(shared_ptr<data_struct>& working_data, stack::stack_it_t stack_it, stack::key_it_t key_it) {
        key_id_t key_id = key_it->second;
        if (working_data->key_stacks[key_id].size() == 1) {
            working_data->keys.erase(key_it); // safe?
            working_data->key_stacks.erase(key_id); // safe?
        } else {
            working_data->key_stacks[key_id].pop_front();
        }
        working_data->stack.erase(stack_it);
    }

    template<typename K, typename V>
    typename stack<K, V>::const_iterator stack<K, V>::cbegin() {
        return const_iterator(data->keys.cbegin());
    }

    template<typename K, typename V>
    typename stack<K, V>::const_iterator stack<K, V>::cend() {
        return const_iterator(data->keys.cend());
    }

    template<typename K, typename V>
    stack<K, V>::const_iterator::const_iterator(itnl_itr iter): iter(std::make_shared<itnl_itr>(iter)) {}

    template<typename K, typename V>
    typename stack<K, V>::const_iterator& stack<K, V>::const_iterator::operator++() {
        auto cpy = std::make_shared<itnl_itr>(*iter.get());
        (*cpy.get()) ++;
        iter.swap(cpy);
        return *this;
    }

    template<typename K, typename V>
    typename stack<K, V>::const_iterator stack<K, V>::const_iterator::operator++(int) {
        auto cpy1 = std::make_shared<itnl_itr>(*iter.get());
        auto cpy2 = std::make_shared<itnl_itr>(*iter.get());
        cpy1.get() ++;
        const_iterator r(cpy2);
        iter.swap(cpy1);
        return r;
    }

    template<typename K, typename V>
    const K& stack<K, V>::const_iterator::operator*() const noexcept {
        return (**iter.get()).first;
    }

    template<typename K, typename V>
    const K* stack<K, V>::const_iterator::operator->() const noexcept {
        return iter.get().operator->();
    }

    template<typename K, typename V>
    bool stack<K, V>::const_iterator::operator==(const const_iterator& other) const noexcept{
        return (iter.get()->operator->())==(other.iter.get()->operator->());
    }

    template<typename K, typename V>
    bool stack<K, V>::const_iterator::operator!=(const const_iterator& other) const noexcept {
        return !this->operator==(other);
    }

    template<typename K, typename V>
    typename stack<K, V>::const_iterator& stack<K, V>::const_iterator::operator=(const const_iterator& other) {
        iter = other.iter;
    }

    template<typename K, typename V>
    stack<K, V>::const_iterator::const_iterator() {
        iter = itnl_itr_ptr();
    }

    template<typename K, typename V>
    stack<K, V>::const_iterator::const_iterator(const_iterator const& other) {
        iter = itnl_itr_ptr(other.iter);
    }

    template<typename K, typename V>
    stack<K, V>::const_iterator::const_iterator(const_iterator && other) {
        iter = itnl_itr_ptr(std::move(other.iter));
    }
}


#endif