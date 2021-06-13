#pragma once

#include "math/types.h"
#include "misc/gn_assert.h"

#define HASHTABLE_START_CAPACITY   32
#define HASHTABLE_GROWTH_RATE      2.0
#define HASHTABLE_LOAD_FACTOR      0.7

#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a > b) ? a : b)

// Functions used
void* malloc(size_t _Size);
void* calloc(size_t _Count, size_t _Size);
void* memcpy(void *_Dst, const void *_Src, size_t _Size);

namespace gn
{

template <typename T>
struct hash
{
    size_t operator()(const T& key);
};

template <typename key_type, typename value_type, typename hasher = hash<key_type>>
class hash_table
{
public:
    struct pair
    {
        key_type   key;
        value_type value;

        pair() {};

        pair(const key_type& key, const value_type& value)
        :   key(key), value(value) {}

        pair(key_type&& key, value_type&& value)
        :   key(std::move(key)), value(std::move(value)) {}

        pair(const pair& other)
        :   key(other.key), value(other.value) {}

        pair(pair&& other)
        :   key(std::move(other.key)), value(std::move(other.value)) {}
    };

    struct iterator
    {
        const hash_table& table;
        pair* ptr;

        iterator() {};

        iterator(const hash_table& table, pair* ptr)
        :   table(table), ptr(ptr) {};

        iterator(const iterator& other)
        :   table(other.table), ptr(other.ptr) {}

        void advance()
        {
            while (ptr < table.last + 1)
            {
                ptr++;

                if (table.is_active(table.active_pairs, (size_t)(ptr - table.first)))
                    break;
            }

            ptr = min(ptr, table.last + 1);
        }

        void retreat()
        {
            while (ptr > table.first)
            {
                ptr--;

                if (table.is_active(table.active_pairs, (size_t)(ptr - table.first)))
                    break;
            }

            ptr = max(ptr, table.first);
        }

        iterator& operator++()
        {
            advance();
            return *this;
        }

        iterator operator++(int)
        {
            iterator it = *this;
            advance();
            return it;
        }

        iterator& operator--()
        {
            retreat();
            return *this;
        }

        iterator operator--(int)
        {
            iterator it = *this;
            retreat();
            return it;
        }

        const pair& operator*() const
        {
            return *ptr;
        }

        pair& operator*()
        {
            return *ptr;
        }

        const pair* operator->() const
        {
            return ptr;
        }

        pair* operator->()
        {
            return ptr;
        }

        bool operator==(const iterator& other) const
        {
            return ptr == other.ptr;
        }

        bool operator!=(const iterator& other) const
        {
            return ptr != other.ptr;
        }
    };

public:

    size_t filled() const { return active_count; }
    size_t capacity() const { return cap; }

    bool is_empty() const { return active_count == 0; }

    void resize(size_t new_cap)
    {
        if (new_cap == cap)
            return;

        pair* new_pairs = (pair*) malloc(new_cap * sizeof(pair));
        pair* new_first = new_pairs + new_cap;
        pair* new_last  = new_pairs - 1;

        size_t new_active_pair_buckets = ((new_cap / 64) + 1);
        u64* new_active_pairs = (u64*) calloc(new_active_pair_buckets, sizeof(u64));

        // rehash the table
        for (size_t i = 0; i < cap; i += 64)
        {
            if (active_pairs[i / 64] == 0)
                continue;

            size_t limit = min(i + 64, cap);
            for (size_t j = i; j < limit; j++)
            {
                if (is_active(active_pairs, j))
                {
                    // move key and value to new_pairs
                    size_t start_idx = hash(pairs[j].key) % new_cap;
                    size_t last_idx  = (start_idx == 0) ? new_cap - 1: start_idx - 1;

                    for (size_t k = start_idx; k != last_idx; k = (k + 1) % new_cap)
                    {
                        if (is_active(new_active_pairs, k))
                            continue;

                        new_first = min(new_pairs + k, new_first);
                        new_last  = max(new_pairs + k, new_last);

                        set_active(new_active_pairs, k, true);

                        memcpy(new_pairs + k, pairs + j, sizeof(pair));
                        break;
                    }
                }
            }
        }

        // delete the previous array
        free(pairs);
        free(active_pairs);

        // update details
        pairs = new_pairs;
        first = new_first;
        last  = new_last;

        active_pairs = new_active_pairs;
        active_pair_buckets = new_active_pair_buckets;

        cap = new_cap;
    }

    void clear()
    {
        for (size_t i = 0; i < cap; i++)
        {
            if (active_pairs[i / 64] == 0)
            {
                i += 64;
                continue;
            }

            if (is_active(active_pairs, i))
            {
                set_active(active_pairs, i, false);
                pairs[i].~pair();
            }
        }
        
        active_count = 0;
    }

    void put(const key_type& key, const value_type& value)
    {
        if (((double) active_count / (double) cap) >= HASHTABLE_LOAD_FACTOR)
            resize(cap * HASHTABLE_GROWTH_RATE);

        size_t start_idx = hash(key) % cap;
        size_t last_idx  = (start_idx == 0) ? cap - 1: start_idx - 1;

        for (size_t i = start_idx; i != last_idx; i = (i + 1) % cap)
        {
            if (is_active(active_pairs, i))
            {
                if (pairs[i].key != key)
                    continue;
            }
            else
            {
                first = min(pairs + i, first);
                last  = max(pairs + i, last);

                set_active(active_pairs, i, true);

                // Construct key
                new(&pairs[i].key)   key_type(key);
                new(&pairs[i].value) value_type();
                active_count++;
            }

            pairs[i].value = value;
            return;
        }

        ASSERT_NOT_VALID("hash_table::put() shouldn't reach this point");
    }

    void put(const key_type& key, value_type&& value)
    {
        if (((double) active_count / (double) cap) >= HASHTABLE_LOAD_FACTOR)
            resize(cap * HASHTABLE_GROWTH_RATE);

        size_t start_idx = hash(key) % cap;
        size_t last_idx  = (start_idx == 0) ? cap - 1: start_idx - 1;

        for (size_t i = start_idx; i != last_idx; i = (i + 1) % cap)
        {
            if (is_active(active_pairs, i))
            {
                if (pairs[i].key != key)
                    continue;
            }
            else
            {
                first = min(pairs + i, first);
                last  = max(pairs + i, last);

                set_active(active_pairs, i, true);

                new(&pairs[i].key)   key_type(key);
                new(&pairs[i].value) value_type();
                active_count++;
            }

            pairs[i].value = std::move(value);
            return;
        }

        ASSERT_NOT_VALID("hash_table::put() shouldn't reach this point");
        return pairs[cap].value;
    }

    iterator find(const key_type& key)
    {
        size_t start_idx = hash(key) % cap;
        size_t last_idx  = (start_idx == 0) ? cap - 1: start_idx - 1;
        for (size_t i = start_idx; i != last_idx; i = (i + 1) % cap)
        {
            if (active_pairs[i / 64] == 0)
                break;
            
            if (!is_active(active_pairs, i))
                break;

            if (pairs[i].key == key)
                return iterator(*this, pairs + i);
        }

        return end();
    }

    const value_type& get(const key_type& key) const
    {
        size_t start_idx = hash(key) % cap;
        size_t last_idx  = (start_idx == 0) ? cap - 1: start_idx - 1;
        for (size_t i = start_idx; i != last_idx; i = (i + 1) % cap)
        {
            if (active_pairs[i / 64] == 0)
                break;
            
            if (!is_active(active_pairs, i))
                break;

            if (pairs[i].key == key)
                return pairs[i].value;
        }

        ASSERT_NOT_VALID("hash_table::get() can only be used for keys that have already been inserted");
        return pairs[cap].value;
    }

    value_type& get(const key_type& key)
    {
        size_t start_idx = hash(key) % cap;
        size_t last_idx  = (start_idx == 0) ? cap - 1: start_idx - 1;
        for (size_t i = start_idx; i != last_idx; i = (i + 1) % cap)
        {
            if (active_pairs[i / 64] == 0)
                break;
            
            if (!is_active(active_pairs, i))
                break;

            if (pairs[i].key == key)
                return pairs[i].value;
        }

        ASSERT_NOT_VALID("hash_table::get() can only be used for keys that have already been inserted");
        return pairs[cap].value;
    }

    const value_type& at(const key_type& key) const
    {
        if (((double) active_count / (double) cap) >= HASHTABLE_LOAD_FACTOR)
            resize(cap * HASHTABLE_GROWTH_RATE);

        size_t start_idx = hash(key) % cap;
        size_t last_idx  = (start_idx == 0) ? cap - 1: start_idx - 1;

        for (size_t i = start_idx; i != last_idx; i = (i + 1) % cap)
        {
            if (active_pairs[i / 64] == 0 ||
                !is_active(active_pairs, i))
            {
                first = min(pairs + i, first);
                last  = max(pairs + i, last);

                set_active(active_pairs, i, true);

                new(&pairs[i].key)   key_type(key);
                new(&pairs[i].value) value_type();
                active_count++;

                return pairs[i].value;
            }

            if (pairs[i].key == key)
                return pairs[i].value;
        }

        ASSERT_NOT_VALID("hash_table::at() shouldn't reach this point");
        return pairs[cap].value;
    }

    value_type& at(const key_type& key)
    {
        if (((double) active_count / (double) cap) >= HASHTABLE_LOAD_FACTOR)
            resize(cap * HASHTABLE_GROWTH_RATE);

        size_t start_idx = hash(key) % cap;
        size_t last_idx  = (start_idx == 0) ? cap - 1: start_idx - 1;

        size_t i;

        for (i = start_idx; i != last_idx; i = (i + 1) % cap)
        {
            if (active_pairs[i / 64] == 0 ||
                !is_active(active_pairs, i))
            {
                first = min(pairs + i, first);
                last  = max(pairs + i, last);

                set_active(active_pairs, i, true);

                new(&pairs[i].key)   key_type(key);
                new(&pairs[i].value) value_type();
                active_count++;

                return pairs[i].value;
            }

            if (pairs[i].key == key)
                return pairs[i].value;
        }

        ASSERT_NOT_VALID("hash_table::at() shouldn't reach this point");
        return pairs[cap].value;
    }

    // Iterators and C++11 stuff

    iterator begin() const
    {
        iterator it(*this, first);
        return it;
    }

    iterator end() const
    {
        iterator it(*this, last + 1);
        return it;
    }

    // Constructors and `

    hash_table()
    :   active_count(0), cap(HASHTABLE_START_CAPACITY),
        active_pair_buckets((HASHTABLE_START_CAPACITY / 64) + 1)
    {
        pairs = (pair*) malloc(HASHTABLE_START_CAPACITY * sizeof(pair));
        active_pairs = (u64*) calloc(active_pair_buckets, sizeof(u64));

        first = pairs + cap;
        last  = pairs - 1;
    }

    hash_table(const hash_table& other)
    :   active_count(other.active_count), cap(other.cap),
        active_pair_buckets(other.active_pair_buckets),
        first(other.first), last(other.last)
    {
        pairs = (pair*) malloc(HASHTABLE_START_CAPACITY * sizeof(pair));
        active_pairs = (u64*) malloc(active_pair_buckets, sizeof(u64));
        memset(active_pairs, other.active_pairs, active_pair_buckets * sizeof(u64));

        for (size_t i = 0; i < cap; i++)
        {
            if (other.active_pairs[i / 64] == 0)
            {
                i += 64;
                continue;
            }

            if (is_active(other.active_pairs, i))
            {
                new(&pairs[i].key)   key_type(others[i].key);
                new(&pairs[i].value) value_type(others[i].value);
            }
        }
    }

    hash_table(hash_table&& other)
    :   active_count(other.active_count), cap(other.cap),
        active_pair_buckets(other.active_pair_buckets),
        pairs(other.pairs), active_pairs(other.active_pairs),
        first(other.first), last(other.last)
    {
        other.active_pairs = nullptr;
        other.pairs = other.first = other.last = nullptr;

        other.cap = other.active_pair_buckets = other.active_count = 0;
    }

    ~hash_table()
    {
        clear();

        free(active_pairs);
        free(pairs);
    }

    // Operators

    const value_type& operator[](const key_type& key) const
    {
        return at(key);
    }

    value_type& operator[](const key_type& key)
    {
        return at(key);
    }

private:

    bool is_active(u64* bits, size_t index) const
    {
        size_t big_idx   = index / 64;
        size_t small_idx = index % 64;
        bool res = bits[big_idx] & (1Ui64 << small_idx);
        return res;
    }

    void set_active(u64* bits, size_t index, bool active)
    {
        size_t big_idx   = index / 64;
        size_t small_idx = index % 64;

        if (active)
            bits[big_idx] |= (1Ui64 << small_idx);
        else
            bits[big_idx] &= ~(1Ui64 << small_idx);
    }

    // A bitfield to keep track of occupied pairs
    u64* active_pairs { nullptr };
    size_t active_pair_buckets { 0 };

    pair* pairs;
    size_t active_count, cap;

    pair* first;
    pair* last;

    hasher hash;

    friend struct iterator;
};

#undef min
#undef max

} // namespace gn

#include "common_hashes.h"