#ifndef __STACK_H__
#define __STACK_H__

#include <utility>
#include <map>
#include <vector>
#include <memory>
#include <list>
#include <stack>

using std::shared_ptr;


namespace cxx {
    template<typename K, typename V> class stack {
        public:
            // 1 P
            stack() noexcept;
            stack(stack const &) noexcept;
            stack(stack &&);
            stack& operator=(stack);
            // 2 P
            void push(K const &, V const &);
            // 3 S
            void pop();
            void pop(K const &);
            // 4 P
            std::pair<K const &, V &> front();
            std::pair<K const &, V const &> front() const;
            V & front(K const &);
            V const & front(K const &) const;
            // 5 P
            size_t size() const;
            size_t count(K const &) const;
            void clear();
            // S
            class const_iterator {
                public const_iterator& operator ++();
                public const_iterator operator ++(int);
                public const K& operator *() const noexcept;
                public const K* operator ->() const noexcept;
                public const bool operator ==() const noexcept;
                public const bool operator !=() const noexcept;
            };
            const_iterator cbegin();
            const_iterator cend();
        private:
            // setting this member to false indicates that some non-const 
            // references to stack data may exist so making shallow copy
            // would be unsafe
            bool can_be_shallow_copied = true;
            // 6 S
            struct data_struct;
            shared_ptr<data_struct> data;
    };

    template<typename K, typename V>
    struct stack<K, V>::data_struct {
        using stck = std::list<std::pair<K, V>>;

        std::map<K, std::stack<const typename stck::iterator>> aux_lists;
        stck main_list;
        int usages;
        
        data_struct(): usages(1) {}

        data_struct(stck &main_list, std::map<K, std::stack<const typename stck::iterator>> & aux_lists):
            usages(1),
            aux_lists(aux_lists),
            main_list(main_list)
        {}

        shared_ptr<data_struct> cpy_if_needed() {
            if (usages > 1) {
                shared_ptr tmp = std::make_shared<data_struct>(main_list, aux_lists)
                usages --;
                return tmp;
            }
            else {
                // creating additional shared_ptr that is meant to replace existing
                return shared_ptr(this);
            }
        }

    };

    template<typename K, typename V>
    void stack<K, V>::pop() {

        if (data.get()->main_list.size() == 0) throw new std::invalid_argument("cannot pop from empty stack");

        shared_ptr<data_struct> tmp;
        int usages_cpy = data.get()->usages;
        try {
            tmp = data.get()->cpy_if_needed();
            K key = tmp.get()->main_list.front().first;
            tmp.get()->main_list.pop_front();
            tmp.get()->aux_lists.get(key).pop_front();
        } catch (const std::exception& e) {
            data.get()->usages = usages_cpy; // ugly, TODO: make not ugly
            throw e;
        }
        data = tmp;
    }

    template<typename K, typename V>
    void stack<K, V>::pop(K const &key) {

        size_t sz;
        try {
            sz = data.get()->aux_lists.get(key).size();
        } catch (std::exception &e) {
            throw e;
        }

        if (sz == 0) throw new std::invalid_argument("no element with given key");

        shared_ptr<data_struct> tmp;
        int usages_cpy = data.get()->usages;
        try {
            tmp = data.get()->cpy_if_needed();
            auto iter = tmp.get()->aux_lists.get(key).front();
            tmp.get()->aux_lists.get(key).pop_front();
            tmp.get()->main_list.erase(iter);
        } catch (const std::exception& e) {
            data.get()->usages = usages_cpy; // again ugly, TODO: make not ugly
            throw e;
        }
        data = tmp;
    }
}



#endif