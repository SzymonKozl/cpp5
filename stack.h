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

                using itnl_itr = typename std::map<K, uint64_t>::const_iterator;
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
            // setting this member to false indicates that some non-const 
            // references to stack data may exist so making shallow copy
            // would be unsafe
            bool can_share_data = true;
            // 6 S
            struct data_struct;
            shared_ptr<data_struct> data;

            shared_ptr<data_struct> modifiable_data();
    };

    template<typename K, typename V>
    struct stack<K, V>::data_struct {
        using key_id_t      = size_t;
        using key_it_t      = typename std::map<K, key_id_t>::iterator;
        using stack_t       = std::list<std::pair<key_it_t, V>>;
        using key_stack_t   = std::list<typename stack_t::iterator>;

        std::map<K, key_id_t> keys;
        stack_t stack;
        std::map<key_id_t, key_stack_t> key_stacks;

        data_struct(std::map<key_id_t, K>& keys, stack_t& stack, std::map<key_id_t, key_stack_t>& key_stacks)
            : keys(keys),
              stack(stack),
              key_stacks(key_stacks) {
        }

        shared_ptr<data_struct> duplicate() {
            return std::make_shared<data_struct>(keys, stack, key_stacks);
        }
    };

    template<typename K, typename V>
    std::pair<K const&, V&> stack<K, V>::front() {
        shared_ptr<data_struct> working_data = modifiable_data();

        auto& [key_iterator, value] = working_data->stack.front();
        K const& key = key_iterator->first;

        data = working_data;
        can_share_data = false;
        return key, value; // TODO: czy to przejdzie?
    }

    template<typename K, typename V>
    shared_ptr<typename stack<K, V>::data_struct> stack<K, V>::modifiable_data() {
        if (data.use_count() > 1)
            return data->duplicate();
        return data;
    }
}


#endif