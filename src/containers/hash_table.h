#pragma once

#include "misc/gn_assert.h"

#define HASH_TABLE_MAX_LOAD_FACTOR 0.8
#define HASH_TABLE_GROWTH_RATE     2.0

namespace gn {

using hash_t = size_t;

template <typename T>
struct hash
{
    hash_t operator()(T const& key) const;
};

template <typename key_t, typename value_t, typename hasher = hash<key_t>>
class hash_table
{
public:
    struct pair_t
    {
        key_t   key;
        value_t value;
    };

    enum class state_t
    {
        EMPTY,
        TOMBSTONE,
        ACTIVE
    };

    struct slot_t
    {
        state_t state;
        hash_t  hash;
        pair_t  pair;
    };

    struct iterator
    {
        const hash_table* table;
        size_t index;

        iterator(const hash_table* table, size_t index)
        :   table(table), index(index) {}

        iterator(const iterator& other)
        :   table(other.table), index(other.index) {}

        void advance()
        {
            if (index > table->_last)
                return;

            index++;

            while (table->_table[index].state != state_t::ACTIVE &&
                   index <= table->_last)
            { index++; }
        }

        iterator& operator++(int)
        {
            advance();
            return *this;
        }

        iterator operator++()
        {
            iterator it = *this;
            advance();
            return it;
        }

        const pair_t& operator*() const
        {
            return table->_table[index].pair;
        }

        pair_t& operator*()
        {
            return table->_table[index].pair;
        }
        
        bool operator==(const iterator& other) const
        {
            return table == other.table &&
                   index == other.index;
        }

        bool operator!=(const iterator& other) const
        {
            return table != other.table ||
                   index != other.index;
        }
    };

    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }

    iterator begin() const { return iterator(this, _first); }
    iterator end() const { return iterator(this, _last + 1); }

    void resize(size_t new_cap)
    {
        slot_t* prev_table = _table;
        size_t prev_cap = _capacity;

        _capacity = new_cap;
        _table = allocate_slots(_capacity);

        _first = new_cap;
        _last = 0;

        size_t move_count = 0;
        for (size_t idx = 0; move_count < _size && idx < prev_cap; idx++)
        {
            if (prev_table[idx].state != state_t::ACTIVE)
                continue;

            key_t& key = prev_table[idx].pair.key;
            hash_t h = prev_table[idx].hash;

            size_t probe_start = h % _capacity;
            size_t probe_end = (probe_start + _capacity - 1) % _capacity;

            for (size_t i = probe_start; i != probe_end; i = (i + 1) % _capacity)
            {
                if (_table[i].state == state_t::ACTIVE)
                    continue;

                _first = std::min(_first, i);
                _last = std::max(_last, i);

                // Use memcpy here
                memcpy(&_table[i], &prev_table[idx], sizeof(slot_t));
                move_count++;
                break;
            }
        }

        free(prev_table);
    }

    void clear()
    {
        for (size_t i = 0; _size > 0 && i < _capacity; i++)
        {
            if (_table[i].state == state_t::ACTIVE)
            {
                _table[i].pair.key.~key_t();
                _table[i].pair.value.~value_t();

                _table[i].state = state_t::TOMBSTONE;
                _size--;
            }
        }
    }

    template<typename... Args>
    value_t& emplace(const key_t& key, const Args&&... args)
    {
        if (load_factor() >= HASH_TABLE_MAX_LOAD_FACTOR)
            resize(_capacity * HASH_TABLE_GROWTH_RATE);
        
        hash_t h = hash(key);

        size_t probe_start = h % _capacity;
        size_t probe_end = (probe_start + _capacity - 1) % _capacity;

        for (size_t i = probe_start; i != probe_end; i = (i + 1) % _capacity)
        {
            if (_table[i].state == state_t::ACTIVE)
                break;

            _first = std::min(_first, i);
            _last = std::max(_last, i);

            // Fill slot if it's empty or a tombstone

            new(&_table[i].pair.key)   key_t(key);
            new(&_table[i].pair.value) value_t(std::forward<Args>(args)...);

            _table[i].hash  = h;
            _table[i].state = state_t::ACTIVE;
            _size++;

            return _table[i].pair.value;
        }

        ASSERT_NOT_VALID("hash_table::emplace() ran out of slots");
        return _table[_capacity].pair.value;
    }

    void erase(const key_t& key)
    {
        hash_t h = hash(key);

        size_t probe_start = h % _capacity;
        size_t probe_end = (probe_start + _capacity - 1) % _capacity;

        for (size_t i = probe_start; i != probe_end; i = (i + 1) % _capacity)
        {
            if (_table[i].state == state_t::EMPTY)
                break;
            
            if (_table[i].state == state_t::TOMBSTONE ||
                _table[i].hash != h)
                continue;
            
            _table[i].pair.key.~key_t();
            _table[i].pair.value.~value_t();

            size_t next_idx = (i + 1) % _capacity;
            _table[i].state = (_table[next_idx].state == state_t::EMPTY) ? state_t::EMPTY : state_t::TOMBSTONE;
            _size--;

            break;
        }
    }

    value_t& at(const key_t& key)
    {
        if (load_factor() >= HASH_TABLE_MAX_LOAD_FACTOR)
            resize(_capacity * HASH_TABLE_GROWTH_RATE);
        
        hash_t h = hash(key);

        size_t probe_start = h % _capacity;
        size_t probe_end = (probe_start + _capacity - 1) % _capacity;

        for (size_t i = probe_start; i != probe_end; i = (i + 1) % _capacity)
        {
            if (_table[i].state == state_t::TOMBSTONE)
                continue;

            if (_table[i].state == state_t::ACTIVE)
            {
                if (_table[i].hash != h)
                    continue;
                
                return _table[i].pair.value;
            }

            // Encountered an empty element

            _first = std::min(_first, i);
            _last = std::max(_last, i);

            new(&_table[i].pair.key)   key_t(key);
            new(&_table[i].pair.value) value_t();

            _table[i].hash  = h;
            _table[i].state = state_t::ACTIVE;
            _size++;

            return _table[i].pair.value;
        }

        ASSERT_NOT_VALID("hash_table::at() ran out of slots");
        return _table[_capacity].pair.value;
    }

    iterator find(const key_t& key) const
    {
        hash_t h = hash(key);

        size_t probe_start = h % _capacity;
        size_t probe_end = (probe_start + _capacity - 1) % _capacity;

        for (size_t i = probe_start; i != probe_end; i = (i + 1) % _capacity)
        {
            if (_table[i].state == state_t::TOMBSTONE)
                continue;

            if (_table[i].state == state_t::ACTIVE)
            {
                if (_table[i].hash != h)
                    continue;
                
                return iterator(this, i);
            }
        }

        return end();
    }

    const value_t& at(const key_t& key) const
    {
        hash_t h = hash(key);

        size_t probe_start = h % _capacity;
        size_t probe_end = (probe_start + _capacity - 1) % _capacity;

        for (size_t i = probe_start; i != probe_end; i = (i + 1) % _capacity)
        {
            if (_table[i].state == state_t::TOMBSTONE)
                continue;

            if (_table[i].state == state_t::ACTIVE)
            {
                if (_table[i].hash != h)
                    continue;
                
                return _table[i].pair.value;
            }
        }

        return _table[_last + 1].pair.value;
    }

    value_t& operator[](const key_t& key)
    {
        return at(key);
    }

    const value_t& operator[](const key_t& key) const
    {
        return at(key);
    }

    void init(size_t start_capacity = 8)
    {
        _last = _size = 0;
        _first = _capacity = start_capacity;
        _table = allocate_slots(_capacity);
    }

    // Constructors and Destructors

    hash_table(size_t start_capacity = 8)
    :   _size(0), _capacity(start_capacity),
        _first(start_capacity), _last(0)
    {
        _table = allocate_slots(_capacity);
    }

    ~hash_table()
    {
        clear();
        free(_table);
    }

private:

    double load_factor() const { return (double) _size / (double) _capacity; }

    static slot_t* allocate_slots(size_t count)
    {
        slot_t* slots = (slot_t*) malloc(count * sizeof(slot_t));
        for (size_t i = 0; i < count; i++)
            slots[i].state = state_t::EMPTY;
        return slots;
    }

private:
    slot_t* _table;
    size_t _size, _capacity;
    hasher hash;

    size_t _first, _last;
};

} // namespace gn

#include "common_hashes.h"