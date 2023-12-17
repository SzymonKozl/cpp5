#ifndef __STACK_H__
#define __STACK_H__

#include <cassert>
#include <utility>
#include <map>
#include <vector>
#include <memory>
#include <list>
#include <stack>
#include <stdexcept>
#include <iostream>

namespace cxx {
    using std::shared_ptr;
    constexpr bool DEBUG = true;

    template<typename K, typename V>
    class stack {
        public:
            // 1 P
            stack();
            ~stack() = default;
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
            // key id
            using key_id_t          = uint64_t;
            using main_stack_t      = std::list<std::pair<key_id_t, V>>;
            using main_stack_cit_t  = typename main_stack_t::iterator;
            using aux_stack_item_t  = std::list<main_stack_cit_t>;
            // setting this member to false indicates that some non-const 
            // references to stack data may exist so making shallow copy
            // would be unsafe
            bool can_be_shallow_copied = true;
            // 6 S
            struct data_struct;
            shared_ptr<data_struct> data;

            const std::pair<K const&, V const&> _front() const;
            const V & _front(K const &) const;
            std::pair<K const&, V&> _front();
            V & _front(K const &);
            shared_ptr<data_struct> fork_data_if_needed();
    };

    template<typename K, typename V>
    struct stack<K, V>::data_struct {

        std::map<K, key_id_t> keyMapping;
        std::map<key_id_t, typename std::map<K, key_id_t>::iterator> keyMappingRev; // reversed keyMapping
        std::map<key_id_t, aux_stack_item_t> aux_lists;
        main_stack_t main_list;

        key_id_t next_key_id;

        data_struct(): next_key_id(1) {}

        data_struct(main_stack_t &main_list, std::map<key_id_t, aux_stack_item_t> & aux_lists, std::map<K, key_id_t> &keyMapping, std::map<key_id_t, typename std::map<K, key_id_t>::iterator> &keyMappingRev, key_id_t next_key_id = 1) :
            next_key_id(next_key_id),
            aux_lists(aux_lists),
            main_list(main_list),
            keyMapping(keyMapping),
            keyMappingRev(keyMappingRev)
        {}

        // Automatically decrements the use_count!
        shared_ptr<data_struct> duplicate() {

            shared_ptr<data_struct> cpy = std::make_shared<data_struct>(main_list, aux_lists, keyMapping, keyMappingRev, next_key_id);
            return cpy;
        }
    };

    template<typename K, typename V>
    stack<K, V>::stack() : data(std::make_shared<data_struct>()) {}

    template<typename K, typename V>
    stack<K, V>::stack(const stack &other) {
        if (!other.can_be_shallow_copied) {
            data = other.data->duplicate();
        }
        else {
            data = other.data;
        }
    }

    template<typename K, typename V>
    stack<K, V>::stack(stack &&other) noexcept : can_be_shallow_copied(other.can_be_shallow_copied), data(other.data) {}

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
        if (!data->keyMapping.contains(key))
            return 0;
        key_id_t id = data->keyMapping[key];
        return data->aux_lists[id].size();
    }

    template<typename K, typename V>
    void stack<K, V>::clear() noexcept {
        data = std::make_shared<data_struct>();
    }

    template<typename K, typename V>
    void stack<K, V>::push(const K &key, const V &value) {
        shared_ptr<data_struct> copied_data = fork_data_if_needed();
        key_id_t id;
        typename std::map<K, key_id_t>::iterator keyMappingIter;
        typename std::map<key_id_t, typename std::map<K, key_id_t>::iterator>::iterator keyMappingRevIter;
        typename std::map<key_id_t, aux_stack_item_t>::iterator auxListIter;
        bool delFromKeyMapping = false;
        bool delFromKeyMappingRev = false;
        bool delFromAuxList = false;
        if (copied_data->keyMapping.contains(key)) id = copied_data->keyMapping[key];
        else {
            try {
                id = copied_data->next_key_id;
                keyMappingIter = copied_data->keyMapping.insert({key, id}).first;
                delFromKeyMapping = true;
                keyMappingRevIter = copied_data->keyMappingRev.insert({id, copied_data->keyMapping.find(key)}).first;
                delFromKeyMappingRev = true;
                auxListIter = copied_data->aux_lists.insert({id, aux_stack_item_t()}).first;
                delFromAuxList = true;
            }
            catch (...) {
                if (delFromKeyMapping) copied_data->keyMapping.erase(keyMappingIter);
                if (delFromKeyMappingRev) copied_data->keyMappingRev.erase(keyMappingRevIter);
                if (delFromAuxList) copied_data->aux_lists.erase(auxListIter);
                throw;
            }
        }
        bool remFromMainList = false;
        typename main_stack_t::iterator mainListIter;
        try {
            mainListIter = copied_data->main_list.insert(copied_data->main_list.end(), {id, value});
            remFromMainList = true;
            auto it = copied_data->main_list.end();
            copied_data->aux_lists[id].push_back(--it);
            if (delFromKeyMapping) ++copied_data->next_key_id;
        } catch (...) {
            if (remFromMainList) copied_data->main_list.erase(mainListIter);
            copied_data->keyMapping.erase(keyMappingIter);
            copied_data->keyMappingRev.erase(keyMappingRevIter);
            copied_data->aux_lists.erase(auxListIter);
            throw;
        }

        data = copied_data;
        can_be_shallow_copied = true;
    }

    template<typename K, typename V>
    void stack<K, V>::pop() {

        if (data->main_list.size() == 0) throw std::invalid_argument("cannot pop from empty stack");

        shared_ptr<data_struct> tmp = fork_data_if_needed();
        key_id_t key = tmp->main_list.back().first;
        const K& k = (*tmp->keyMappingRev[key]).first;

        tmp->main_list.pop_back();
        if (tmp->aux_lists[key].size() == 1) {
            tmp->keyMapping.erase(k); // unsafe - might throw sth on comapring between K type
            tmp->keyMappingRev.erase(key); // safe - compare on size_t is ok (?)
            tmp->aux_lists.erase(key); // as above
        }
        else tmp->aux_lists.at(key).pop_back(); // should not throw anything
        data = tmp;
        can_be_shallow_copied = true;
    }

    template<typename K, typename V>
    void stack<K, V>::pop(K const &key) {

        size_t sz;
        // TODO: może jakieś makra albo funkcje na dobieranie sie
        //  do tych internal list?

        if (!data->keyMapping.contains(key)) throw std::invalid_argument("no elem with given key");

        key_id_t id = data->keyMapping[key];

        shared_ptr<data_struct> tmp;
        tmp = fork_data_if_needed();
        auto iter = tmp->aux_lists.at(id).back();
        if (tmp->aux_lists.at(id).size() == 1) {
            tmp->keyMapping.erase(key);
            tmp->aux_lists.erase(id);
            tmp->keyMappingRev.erase(id);
        }
        else tmp->aux_lists.at(id).pop_back();
        tmp->main_list.erase(iter);
        data = tmp;
        can_be_shallow_copied = true;
    }

    template<typename K, typename V>
    std::pair<K const&, V&> stack<K, V>::front() {
        can_be_shallow_copied = false; // todo: rollback on fail
        data = fork_data_if_needed();

        return _front();
    }

    template<typename K, typename V>
    std::pair<K const&, V const&> stack<K, V>::front() const {
        return _front();
    }

    template<typename K, typename V>
    V& stack<K, V>::front(K const& key) {
        can_be_shallow_copied = false; // todo: rollback on fail
        fork_data_if_needed();

        return _front(key);
    }

    template<typename K, typename V>
    V const& stack<K, V>::front(K const& key) const {
        return _front(key);
    }

    template<typename K, typename V>
    const std::pair<K const&, V const&> stack<K, V>::_front() const {
        key_id_t key_id = data->main_list.back().first;
        V const& val = data->main_list.back().second;
        K const& key = data->keyMappingRev[key_id]->first;
        return {key, val};
    }

    template<typename K, typename V>
    std::pair<K const&, V&> stack<K, V>::_front() {
        key_id_t key_id = data->main_list.back().first;
        V& val = data->main_list.back().second;
        K const& key = data->keyMappingRev[key_id]->first;
        return {key, val};
    }

    template<typename K, typename V>
    V& stack<K, V>::_front(K const& key) {
        if (!data->keyMapping.contains(key)) {
            throw std::invalid_argument("no element with the given key");
        }

        key_id_t key_id = data->keyMapping[key];
        return data->aux_lists[key_id].back()->second;
    }


    template<typename K, typename V>
    const V& stack<K, V>::_front(K const& key) const {
        if (!data->keyMapping.contains(key)) {
            throw std::invalid_argument("no element with the given key");
        }

        key_id_t key_id = data->keyMapping[key];
        return data->aux_lists[key_id].back()->second;
    }

    /**
     * \brief Forks the data_struct if a reference to a
     *  data item was returned to an outside object, or
     *  if at least one other stack uses the same underlying
     *  data object.
     */
    template<typename K, typename V>
    shared_ptr<typename stack<K, V>::data_struct> stack<K, V>::fork_data_if_needed() {
        shared_ptr<data_struct> data_cpy = data;

        if (data.use_count() > 2) { // copy created above should be not counted in total count
            data_cpy = data->duplicate();
        }

        return data_cpy;
    }

    template<typename K, typename V>
    typename stack<K, V>::const_iterator stack<K, V>::cbegin() {
        return const_iterator(data->keyMapping.cbegin());
    }

    template<typename K, typename V>
    typename stack<K, V>::const_iterator stack<K, V>::cend() {
        return const_iterator(data->keyMapping.cend());
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
}


#endif