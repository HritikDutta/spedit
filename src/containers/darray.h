#pragma once

#include "misc/gn_assert.h"

#define DARRAY_START_CAPACITY   2
#define DARRAY_GROWTH_RATE      1.5

// Functions used
void* realloc(void*, size_t);
void* memcpy(void*, const void*, size_t);
void  free(void*);

namespace gn {

template<typename T>
class darray
{
public:

    size_t size()     const { return _size; }
    size_t capacity() const { return _capacity; }

    const T* data() const { return buffer; };
          T* data()       { return buffer; };

    // Only capacity is increased
    void reserve(size_t capacity)
    {
        reallocate(capacity);
    }

    // The entire array is considered to be filled
    void resize(size_t capacity)
    {
        reallocate(capacity);
        _size = capacity;
    }

    T& push_back(const T& value)
    {
        if (_size >= _capacity)
            reallocate(_capacity * DARRAY_GROWTH_RATE);

        buffer[_size] = value;
        return buffer[_size++];
    }

    T& push_back(T&& value)
    {
        if (_size >= _capacity)
            reallocate(_capacity * DARRAY_GROWTH_RATE);

        buffer[_size] = std::move(value);
        return buffer[_size++];
    }

    template <typename... Args>
    T& emplace_back(Args&&... args)
    {
        if (_size >= _capacity)
            reallocate(_capacity * DARRAY_GROWTH_RATE);

        new(&buffer[_size]) T(std::forward<Args>(args)...);
        return buffer[_size++];
    }

    T& pop_back()
    {
        if (_size > 0)
        {
            _size--;
            buffer[_size].~T();
        }
    }

    T& insert(size_t index, const T& val)
    {
        if (_size >= _capacity)
            reallocate(_capacity * DARRAY_GROWTH_RATE);

        for (int i = _size; i > index; i--)
            buffer[i] = buffer[i - 1];

        buffer[index] = val;
        return buffer[index];
    }

    T& insert(size_t index, T&& val)
    {
        if (_size >= _capacity)
            reallocate(_capacity * DARRAY_GROWTH_RATE);

        for (size_t i = _size; i > index; i--)
            buffer[i] = buffer[i - 1];

        buffer[index] = std::move(val);
        return buffer[index];
    }

    void erase_at(size_t index)
    {
        ASSERT(index < _size);

        buffer[index].~T();
        _size--;

        for (size_t i = index; i < _size; i++)
            memcpy(buffer + i, buffer + i + 1, sizeof(T));
    }

    void erase_swap(size_t index)
    {
        ASSERT(index < _size);

        buffer[index].~T();
        _size--;

        buffer[index] = std::move(buffer[_size]);
    }

    void clear()
    {
        for (int i = 0; i < _size; i++)
            buffer[i].~T();
        
        _size = 0;
    }

    // Iterators and C++11 stuff

    const T* begin() const { return buffer; }
          T* begin()       { return buffer; }

    const T* end() const { return buffer + _size; }
          T* end()       { return buffer + _size; }

    // Constructors and Destructors

    darray(size_t capacity = DARRAY_START_CAPACITY)
    :   _size(0), _capacity(capacity),
        buffer(nullptr)
    {
        reallocate(capacity);
    }

    darray(const std::initializer_list<T>& values)
    :   _size(0), _capacity(values.size()),
        buffer(nullptr)
    {
        for (auto&& val : values)
            buffer[_size++] = std::move(values);
    }

    darray(const darray& other)
    :   _size(0), _capacity(0),
        buffer(nullptr)
    {
        reallocate(other._capacity);

        for (; _size < other._size; _size++)
	        buffer[_size] = other.buffer[_size];
    }

    darray(darray&& other)
    :   _size(other._size), _capacity(other._capacity),
        buffer(other.buffer)
    {
        other._size = other._capacity = 0;
        other.buffer = nullptr;
    }

    ~darray()
    {
        clear();
        free(buffer);
    }

    const T& operator[](size_t index) const
    {
        ASSERT(index < _size);
        return buffer[index];
    }

    T& operator[](size_t index)
    {
        ASSERT(index < _size);
        return buffer[index];
    }

    darray& operator=(const darray& other)
    {
        clear();
        reallocate(other._capacity);

        for (; _size < other._size; _size++)
	        buffer[_size] = other.buffer[_size];

        return *this;
    }

    darray& operator=(darray&& other)
    {
        buffer = other.buffer;
        _size  = other._size;
        _capacity = other._capacity;

        other._size = other._capacity = 0;
        other.buffer = nullptr;

        return *this;
    }

private:
    void reallocate(size_t new_cap)
    {
        ASSERT(_size <= new_cap);

        T* new_buffer = (T*) realloc(buffer, new_cap * sizeof(T));
        ASSERT(new_buffer);

        buffer = new_buffer;
        _capacity = new_cap;
    }

private:
    size_t _size = 0, _capacity = 0;
    T* buffer = nullptr;
};

} // namespace gn