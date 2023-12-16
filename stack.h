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
            stack();
            stack(stack const &);
            stack(stack &&) noexcept;
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
            size_t size() const noexcept;
            size_t count(K const &) const;
            void clear() noexcept;
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

            std::pair<K const&, V&> _front();
    };

    template<typename K, typename V>
    struct stack<K, V>::data_struct {
        // key id
        using key_id_t          = uint64_t;
        using main_stack_t      = std::list<std::pair<key_id_t, V>>;
        using main_stack_cit_t  = const typename main_stack_t::iterator;
        using aux_stack_item_t  = std::list<main_stack_cit_t>;

        std::map<K, key_id_t> keyMapping;
        std::map<key_id_t, aux_stack_item_t> aux_lists;
        main_stack_t main_list;

        size_t usages;
        key_id_t next_key_id;

        data_struct(): usages(1), next_key_id(1) {}

        data_struct(main_stack_t &main_list, std::map<K, aux_stack_item_t> & aux_lists, std::map<K, key_id_t> keyMapping) :
            usages(1),
            next_key_id(1),
            aux_lists(aux_lists),
            main_list(main_list),
            keyMapping(keyMapping)
        {}

        /**
         * \brief Makes a copy of this struct if it is needed.
         * \return If a copy was performed, returns a pointer to the copied struct.
         */
        shared_ptr<data_struct> cpy_if_needed() {
            if (usages > 1) {
                shared_ptr<data_struct> tmp = copy();
                return tmp;
            }
            else {
                // creating additional shared_ptr that is meant to replace existing
                return shared_ptr(this); // TODO: to chyba może dać segfaulta - bo będzie kilka shared pointerów zarządzających tą samą piamięcią
            }
        }

        shared_ptr<data_struct> copy() {
            return std::make_shared<data_struct>(main_list, aux_lists, keyMapping, next_key_id);
        }
    };

    template<typename K, typename V>
    stack<K, V>::stack() : data(std::make_shared<data_struct>()) {}

    template<typename K, typename V>
    stack<K, V>::stack(const stack &other) {
        if (other.can_be_shallow_copied) {
            data = other.data;
        } else {
            data = other.data->copy();
        }

        ++data->usages;
        can_be_shallow_copied = true;
    }

    template<typename K, typename V>
    stack<K, V>::stack(stack &&other) noexcept : can_be_shallow_copied(other.can_be_shallow_copied), data(std::move(other.data)) {}

    template<typename K, typename V>
    stack<K, V> &stack<K, V>::operator=(stack other) {
        std::swap(data, other.data);
        std::swap(can_be_shallow_copied, other.can_be_shallow_copied);

        return *this;
    }

    template<typename K, typename V>
    size_t stack<K, V>::size() const noexcept {
        return data->main_list.size();
    }

    template<typename K, typename V>
    size_t stack<K, V>::count(const K &key) const {
        if (!data->aux_lists.contains(key))
            return 0;
        return data->aux_lists[key].size();
    }

    template<typename K, typename V>
    void stack<K, V>::clear() noexcept {
        data = std::make_shared<data_struct>();
    }

    template<typename K, typename V>
    void stack<K, V>::push(const K &key, const V &value) {
        shared_ptr<data_struct> copied_data = data->cpy_if_needed(can_be_shallow_copied);
        typename data_struct::key_id_t id = data;

        copied_data->keyMapping.insert({key, id});
        try {
            copied_data->main_list.push_back({id, value});

            try {
                auto it = copied_data->main_list.cend();
                copied_data->aux_lists[id].push_back(--it);
            } catch (...) {
                if (copied_data->aux_lists.contains(id)) {
                    if (!copied_data->aux_lists[id].empty())
                        copied_data->aux_lists[id].pop_back();

                    if (copied_data->aux_lists[id].empty())
                        copied_data->aux_lists.erase(id);
                }

                throw;
            }

        } catch (...) {
            copied_data->keyMapping.erase(key);
            throw;
        }

        ++copied_data->next_key_id;
        data = copied_data;
    }

    template<typename K, typename V>
    void stack<K, V>::pop() {

        if (data.get()->main_list.size() == 0) throw std::invalid_argument("cannot pop from empty stack");

        shared_ptr<data_struct> tmp;
        int usages_cpy = data.get()->usages;
        tmp = data.get()->cpy_if_needed();
        K key = tmp.get()->main_list.front().first;
        tmp.get()->main_list.pop_front();
        tmp.get()->aux_lists.get(key).pop_front();
        if (data.get().usages > 1) data.get().usages --;
        data = tmp;
    }

    template<typename K, typename V>
    void stack<K, V>::pop(K const &key) {

        size_t sz;
        // TODO: może jakieś makra albo funkcje na dobieranie sie
        //  do tych internal list?
        sz = data.get()->aux_lists.get(key).size();

        if (sz == 0) throw new std::invalid_argument("no element with given key");

        shared_ptr<data_struct> tmp;
        tmp = data.get()->cpy_if_needed();
        auto iter = tmp.get()->aux_lists.get(key).front();
        tmp.get()->aux_lists.get(key).pop_front();
        tmp.get()->main_list.erase(iter);
        if (data.get().usages > 1) data.get().usages --;
        data = tmp;
    }

    template<typename K, typename V>
    std::pair<K const&, V&> stack<K, V>::front() {
        can_be_shallow_copied = false;
        data = data->cpy_if_needed(); // TODO

        return _front();
    }

    template<typename K, typename V>
    std::pair<K const&, V const&> stack<K, V>::front() const {
        return _front();
    }

    template<typename K, typename V>
    V& stack<K, V>::front(K const&) {
        can_be_shallow_copied = false;
        data = data->cpy_if_needed(); // TODO

        return _front().first;
    }

    template<typename K, typename V>
    V const& stack<K, V>::front(K const&) const {
        return _front().second;
    }

    template<typename K, typename V>
    std::pair<K const&, V&> stack<K, V>::_front() {
        typename data_struct::key_id_t key_id = data->main_list.front().first;
        V& val = data->main_list.front().second;
        K const& key = *data->aux_lists[key_id].front();
        return {key, val};
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