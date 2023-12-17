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

namespace cxx {
    using std::shared_ptr;
    constexpr bool DEBUG = true;

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
                using iterator_category = std::forward_iterator_tag;
                using difference_type = size_t;
                using value_type = K;
                using pointer = K*;
                using reference = K&;

                using itnl_itr = typename std::map<K, uint64_t>::const_iterator; // TODO: key_id_t
                using itnl_itr_ptr = shared_ptr<itnl_itr>;


                public:
                    const_iterator(itnl_itr);
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
            using stack_it_t    = std::list<std::pair<key_it_t, V>>::iterator;
            using key_stack_t   = std::list<typename stack_t::iterator>;

        // setting this member to false indicates that some non-const
            // references to stack data may exist so making shallow copy
            // would be unsafe
            bool can_share_data = true;
            // 6 S
            struct data_struct;
            shared_ptr<data_struct> data;

            shared_ptr<data_struct> modifiable_data();
            void remove_element(shared_ptr<data_struct>& working_data, stack_it_t stack_it, key_it_t key_it);
    };

    template<typename K, typename V>
    struct stack<K, V>::data_struct {

        std::map<K, key_id_t> keys;
        stack_t stack;
        std::map<key_id_t, key_stack_t> key_stacks;

        data_struct(std::map<K, key_id_t>& keys, stack_t& stack, std::map<key_id_t, key_stack_t>& key_stacks)
            : keys(keys),
              stack(stack),
              key_stacks(key_stacks) {
        }

        shared_ptr<data_struct> duplicate() {
            return std::make_shared<data_struct>(keys, stack, key_stacks);
        }
    };

    // Works for removing only the first element with a given key.
    template<typename K, typename V>
    void stack<K, V>::remove_element(shared_ptr<data_struct>& working_data, stack::stack_it_t stack_it, stack::key_it_t key_it) {
        key_id_t key_id = *key_it;
        if (working_data->key_stacks[key_id].size() == 1) {
            working_data->keys.erase(key_it); // safe?
            working_data->key_stacks.erase(key_id); // safe?
        } else {
            working_data->key_stacks[key_id].pop_front();
        }
        working_data->stack.erase(stack_it);
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
        auto key_it = working_data->keys[key].begin();
        auto stack_it = working_data->key_stacks[*key_it].front();

        remove_element(working_data, stack_it, key_it);

        data = working_data;
        can_share_data = true;
    }

    template<typename K, typename V>
    std::pair<K const&, V&> stack<K, V>::front() {
        shared_ptr<data_struct> working_data = modifiable_data();

        auto const& [key_iterator, value] = working_data->stack.front();
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
    shared_ptr<typename stack<K, V>::data_struct> stack<K, V>::modifiable_data() {
        if (data.use_count() > 1)
            return data->duplicate();
        return data;
    }
}


#endif