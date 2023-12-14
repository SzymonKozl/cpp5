#ifndef __STACK_H__
#define __STACK_H__

#include <utility>
#include <map>
#include <vector>
#include <memory>
#include <list>
#include <stack>


namespace cxx {
    using std::shared_ptr;

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
                using iterator_category = std::forward_iterator_tag;
                using difference_type = size_t;
                using value_type = K;
                using pointer = K*;
                using reference = K&;

                using itnl_itr = std::map<K, uint64_t>::const_iterator;
                using itnl_itr_ptr = shared_ptr<itnl_itr>;


                public:
                    const_iterator(itnl_itr_ptr);
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
            bool can_be_shallow_copied = true;
            // 6 S
            struct data_struct;
            shared_ptr<data_struct> data;
    };

    template<typename K, typename V>
    void stack<K, V>::push(const K &, const V &) {

    }

    template<typename K, typename V>
    struct stack<K, V>::data_struct {
        // key id
        using kid_t             = uint64_t;
        using counter_t         = size_t;
        using sitem_t           = std::pair<kid_t, V>;
        using main_stack_t      = std::list<sitem_t>;
        using main_stack_cit_t  = const typename main_stack_t::iterator;
        using aux_stack_item_t  = std::list<main_stack_cit_t>;

        std::map<K, kid_t> keyMapping;
        std::map<kid_t, aux_stack_item_t> aux_lists;
        main_stack_t main_list;

        size_t usages;
        
        data_struct(): usages(1) {}

        data_struct(main_stack_t &main_list, std::map<K, aux_stack_item_t > & aux_lists, std::map<K, kid_t> keyMapping):
            usages(1),
            aux_lists(aux_lists),
            main_list(main_list),
            keyMapping(keyMapping)
        {}

        // TODO: albo zwracać np. optional<shared_ptr>?
        shared_ptr<data_struct> cpy_if_needed() {
            if (usages > 1) {
                shared_ptr tmp = std::make_shared<data_struct>(main_list, aux_lists);
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

        if (data.get()->main_list.size() == 0) throw std::invalid_argument("cannot pop from empty stack");

        shared_ptr<data_struct> tmp;
        int usages_cpy = data.get()->usages;
        // TODO: możliwe, że try catch niepotrzebny, bo wszystkie(?) operacje są noexcept
        try {
            tmp = data.get()->cpy_if_needed();
            K key = tmp.get()->main_list.front().first;
            tmp.get()->main_list.pop_front();
            tmp.get()->aux_lists.get(key).pop_front();
        } catch (...) { // TODO: upewnić się, że to nic nie kopiuje, i łapie przez referencje
            data.get()->usages = usages_cpy; // ugly, TODO: make not ugly
            throw;
        }
        data = tmp;
    }

    template<typename K, typename V>
    void stack<K, V>::pop(K const &key) {

        size_t sz;
        try {
            // TODO: może jakieś makra albo funkcje na dobieranie sie
            //  do tych internal list?
            sz = data.get()->aux_lists.get(key).size();
        } catch (std::exception &e) {
            throw e;
        }

        if (sz == 0) throw new std::invalid_argument("no element with given key");

        shared_ptr<data_struct> tmp;
        // todo: podobny blok do tego wyżej - może da się wyciągnąć coś wspólnego?
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

    template<typename K, typename V>
    stack<K, V>::const_iterator stack<K, V>::cbegin() {
        return const_iterator(data.keyMapping.cbegin());
    }

    template<typename K, typename V>
    stack<K, V>::const_iterator stack<K, V>::cend() {
        return const_iterator(data.keyMapping.cend());
    }

    template<typename K, typename V>
    stack<K, V>::const_iterator::const_iterator(itnl_itr_ptr iter): iter(iter) {}

    template<typename K, typename V>
    stack<K, V>::const_iterator& stack<K, V>::const_iterator::operator++() {
        auto cpy = std::make_shared<itnl_itr>(iter.get());
        cpy.get() ++;
        iter.swap(cpy);
        return &this;
    }

    template<typename K, typename V>
    stack<K, V>::const_iterator stack<K, V>::const_iterator::operator++(int) {
        auto cpy1 = std::make_shared<itnl_itr>(iter.get());
        auto cpy2 = std::make_shared<itnl_itr>(iter.get());
        cpy1.get() ++;
        const_iterator r(cpy2);
        iter.swap(cpy1);
        return r;
    }

    template<typename K, typename V>
    const K& stack<K, V>::const_iterator::operator*() const noexcept {
        return *iter.get();
    }

    template<typename K, typename V>
    const K* stack<K, V>::const_iterator::operator->() const noexcept {
        return iter.get().operator->();
    }

    template<typename K, typename V>
    bool stack<K, V>::const_iterator::operator==(const const_iterator& other) const noexcept{
        return (iter.get().operator->())==(other.iter.get().operator->());
    }

    template<typename K, typename V>
    bool stack<K, V>::const_iterator::operator!=(const const_iterator& other) const noexcept {
        return !this->operator==(other);
    }
}


#endif