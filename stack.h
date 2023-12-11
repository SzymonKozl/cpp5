#ifndef __STACK_H__
#define __STACK_H__

#include <utility>
#include <map>
#include <vector>
#include <memory>


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
            // 5 P
            size_t size() const;
            size_t count(K const &) const;
            void clear();
        private:
            bool is_shallow_copy;
            // 6 S
            template<typename K, typename V>
            struct data_struct;
            std::shared_ptr<data_struct> data;
    };

    template<typename K, typename V>
    struct cxx::stack<K, V>::data_struct {
        // konstruktory & destruktory
        // list<pair<K, v>> l;
        // map<K, stack<iterator>> auxiliary stacks
        // copy()

    };
}



#endif