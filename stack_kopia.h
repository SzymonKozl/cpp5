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
            // setting this member to false indicates that some non-const 
            // references to stack data may exist so making shallow copy
            // would be unsafe
            bool can_be_shallow_copied = true;
            // 6 S
            struct data_struct;
            shared_ptr<data_struct> data;

            const std::pair<K const&, V const&> _front() const;
            std::pair<K const&, V&> _front();
            template<typename R> R _front(K const &);
            shared_ptr<data_struct> fork_data_if_needed(bool auto_use_count_mgmt = true);
    };

    template<typename K, typename V>
    struct stack<K, V>::data_struct {
        // key id
        using key_id_t          = uint64_t;
        using main_stack_t      = std::list<std::pair<key_id_t, V>>;
        using main_stack_cit_t  = typename main_stack_t::iterator;
        using aux_stack_item_t  = std::list<main_stack_cit_t>;

        std::map<K, key_id_t> keyMapping;
        std::map<key_id_t, typename std::map<K, key_id_t>::iterator> keyMappingRev; // reversed keyMapping
        std::map<key_id_t, aux_stack_item_t> aux_lists;
        main_stack_t main_list;

        size_t use_count;
        key_id_t next_key_id;

        data_struct(): use_count(1), next_key_id(1) {}

        data_struct(main_stack_t &main_list, std::map<key_id_t, aux_stack_item_t> & aux_lists, std::map<K, key_id_t> keyMapping, std::map<key_id_t, typename std::map<K, key_id_t>::iterator> keyMappingRev, key_id_t next_key_id = 1) :
            use_count(1),
            next_key_id(next_key_id),
            aux_lists(aux_lists),
            main_list(main_list),
            keyMapping(keyMapping),
            keyMappingRev(keyMappingRev)
        {}

        // Automatically decrements the use_count!
        shared_ptr<data_struct> duplicate(bool decrement = true) {
            if constexpr (DEBUG) {
                if (decrement) {
                    assert(use_count >= 1);
                }
            }

            shared_ptr<data_struct> cpy = std::make_shared<data_struct>(main_list, aux_lists, keyMapping, keyMappingRev, next_key_id);
            if (decrement) {
                use_count--;
            }
            return cpy;
        }
    };

    template<typename K, typename V>
    stack<K, V>::stack() : data(std::make_shared<data_struct>()) {}

    template<typename K, typename V>
    stack<K, V>::stack(const stack &other) {
        data = other.data;
        ++data->use_count;
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
        shared_ptr<data_struct> copied_data = fork_data_if_needed();
        typename data_struct::key_id_t id;
        if (copied_data->keyMapping.contains(key)) id = copied_data->keyMapping[key];
        else {
            id = copied_data->next_key_id;
            copied_data->keyMapping.insert({key, id});
        }
        copied_data->keyMappingRev.insert({id, copied_data->keyMapping.find(key)});
        try {
            copied_data->main_list.push_back({id, value});

            try {
                auto it = copied_data->main_list.end();
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
            copied_data->keyMapping.erase(key); // po chuj? bo fork mógł zwrócić pointer na oryginalnego data_structa
            throw;
        }

        ++copied_data->next_key_id;
        data = copied_data;
    }

    template<typename K, typename V>
    void stack<K, V>::pop() {

        if (data->main_list.size() == 0) throw std::invalid_argument("cannot pop from empty stack");

        shared_ptr<data_struct> tmp;
        tmp = fork_data_if_needed(false);
        key_t key = tmp->main_list.back().first;
        tmp->main_list.pop_back();
        tmp->aux_lists.at(key).pop_back();
        if (data->use_count > 1) data->use_count--;
        data = tmp;
    }

    template<typename K, typename V>
    void stack<K, V>::pop(K const &key) {

        size_t sz;
        // TODO: może jakieś makra albo funkcje na dobieranie sie
        //  do tych internal list?
        sz = data->aux_lists.at(key).size();

        if (sz == 0) throw new std::invalid_argument("no element with given key");

        shared_ptr<data_struct> tmp;
        tmp = fork_data_if_needed(false);
        auto iter = tmp->aux_lists.at(key).back();
        tmp->aux_lists.at(key).pop_back();
        tmp->main_list.erase(iter);
        if (data->use_count > 1) data->use_count--;
        data = tmp;
    }

    template<typename K, typename V>
    std::pair<K const&, V&> stack<K, V>::front() {
        can_be_shallow_copied = false;
        data = fork_data_if_needed();

        return _front();
    }

    template<typename K, typename V>
    std::pair<K const&, V const&> stack<K, V>::front() const {
        return _front();
    }

    template<typename K, typename V>
    V& stack<K, V>::front(K const& key) {
        can_be_shallow_copied = false;
        fork_data_if_needed();

        return _front(key).second;
    }

    template<typename K, typename V>
    V const& stack<K, V>::front(K const& key) const {
        return _front(key);
    }

    template<typename K, typename V>
    const std::pair<K const&, V const&> stack<K, V>::_front() const {
        typename data_struct::key_id_t key_id = data->main_list.back().first;
        V& val = data->main_list.back().second;
        K const& key = data->keyMappingRev[key_id]->first;
        return {key, val};
    }

    template<typename K, typename V>
    std::pair<K const&, V&> stack<K, V>::_front() {
        typename data_struct::key_id_t key_id = data->main_list.back().first;
        V& val = data->main_list.back().second;
        K const& key = data->keyMappingRev[key_id]->first;
        return {key, val};
    }

    template<typename K, typename V, typename R>
    R stack<K, V>::_front(K const& key) {
        if (!data->keyMapping.contains(key)) {
            throw std::invalid_argument("no element with the given key");
        }

        typename data_struct::key_id_t key_id = data->keyMapping[key];
        return data->aux_lists[key_id].back()->second;
    }


    template<typename K, typename V>
    V const& stack<K, V>::_front(K const& key) const {
        if (!data->keyMapping.contains(key)) {
            throw std::invalid_argument("no element with the given key");
        }

        typename data_struct::key_id_t key_id = data->keyMapping[key];
        return data->aux_lists[key_id].back()->second;
    }

    /**
     * \brief Forks the data_struct if a reference to a
     *  data item was returned to an outside object, or
     *  if at least one other stack uses the same underlying
     *  data object.
     */
    template<typename K, typename V>
    shared_ptr<typename stack<K, V>::data_struct> stack<K, V>::fork_data_if_needed(bool auto_use_count_mgmt) {
        shared_ptr<data_struct> data_cpy = data;
        if (data->use_count == 1) {
            can_be_shallow_copied = true;
        }

        if (!can_be_shallow_copied || data->use_count > 1) {
            data_cpy = data->duplicate(auto_use_count_mgmt);
            can_be_shallow_copied = true;
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