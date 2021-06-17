#pragma once

#include <string>
#include "containers/darray.h"
#include "containers/hash_table.h"

namespace json
{

using String = std::string;
using ArrayNode  = gn::darray<size_t>;
using ObjectNode = gn::hash_table<std::string, size_t, std::hash<std::string>>;

struct Resource
{
    enum struct Type
    {
        NONE,
        BOOLEAN,
        INTEGER,
        FLOAT,
        STRING
    };
    
    Type type;

    union
    {
        bool    _boolean;
        int64_t _integer;
        double  _float;
        String  _string;
    };

    Resource()
    :   type(Type::NONE) {}

    Resource(bool _boolean)
    :   type(Type::BOOLEAN), _boolean(_boolean) {}

    Resource(int64_t _integer)
    :   type(Type::INTEGER), _integer(_integer) {}

    Resource(double _float)
    :   type(Type::FLOAT), _float(_float) {}

    Resource(String&& _string)
    :   type(Type::STRING), _string(_string) {}

    ~Resource()
    {
        if (type == Type::STRING)
            _string.~basic_string();
    }
};

struct DependencyNode
{
    enum struct Type
    {
        DIRECT,
        ARRAY,
        OBJECT,
    };

    Type type;

    union
    {
        size_t     _index;      // Index into the resource tree
        ArrayNode  _array;      // This will contain indices into the dependecy tree
        ObjectNode _object;     // This will contain indices into the dependecy tree
    };

    DependencyNode(Type type)
    :   type(type)
    {
        // I know this is a bit hackey but it does the job
        switch (type)
        {
            case Type::ARRAY:
            _array.init();
            break;
            case Type::OBJECT:
            _object.init();
            break;
        }
    }

    ~DependencyNode()
    {
        switch (type)
        {
            case Type::ARRAY:
            {
                _array.~darray();
            } break;


            case Type::OBJECT:
            {
                _object.~hash_table();
            } break;
        }
    }
};

struct Value;
struct Document;

struct Document
{
    gn::darray<DependencyNode> dependencyTree;
    gn::darray<Resource>       resources;
    Value start() const;
};

struct Array
{
    const Document& _document;
    size_t _treeIndex;

    Array(const Document& document, size_t index)
    :   _document(document), _treeIndex(index)
    {
        auto& node = _document.dependencyTree[_treeIndex];
        ASSERT(node.type == DependencyNode::Type::ARRAY);
    }

    Value operator[](size_t index) const;

    size_t size() const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        return node._array.size();
    }

    struct iterator
    {
        const Array* _array;
        ArrayNode::iterator _arrayIt;

        iterator(const Array* _array, ArrayNode::iterator _arrayIt)
        :   _array(_array), _arrayIt(_arrayIt) {}

        iterator& operator++(int)
        {
            _arrayIt++;
            return *this;
        }
        
        iterator operator++()
        {
            iterator it = *this;
            _arrayIt++;
            return it;
        }

        Value operator*() const;

        bool operator==(const iterator& other) const
        {
            return _array == other._array &&
                   _arrayIt == other._arrayIt;
        }

        bool operator!=(const iterator& other) const
        {
            return _array != other._array ||
                   _arrayIt != other._arrayIt;
        }
    };

    iterator begin() const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        return iterator(this, node._array.begin());
    }

    iterator end() const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        return iterator(this, node._array.end());
    }
};

struct Object
{
    const Document& _document;
    size_t _treeIndex;

    Object(const Document& document, size_t index)
    :   _document(document), _treeIndex(index)
    {
        auto& node = _document.dependencyTree[_treeIndex];
        ASSERT(node.type == DependencyNode::Type::OBJECT);
    }

    // Returns null if key isn't found
    Value operator[](const String& key) const;
};

struct Value
{
    const Document& _document;
    size_t _treeIndex;

    Value(const Document& document, size_t index)
    :   _document(document), _treeIndex(index) {}

    const int64_t int64() const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        ASSERT(node.type == DependencyNode::Type::DIRECT);

        auto& resource = _document.resources[node._index];
        ASSERT(resource.type == Resource::Type::INTEGER);

        return resource._integer;
    }

    const double float64() const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        ASSERT(node.type == DependencyNode::Type::DIRECT);

        auto& resource = _document.resources[node._index];
        ASSERT(resource.type == Resource::Type::FLOAT);

        return resource._float;
    }

    const bool boolean() const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        ASSERT(node.type == DependencyNode::Type::DIRECT);

        auto& resource = _document.resources[node._index];
        ASSERT(resource.type == Resource::Type::BOOLEAN);

        return resource._boolean;
    }

    const String& string() const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        ASSERT(node.type == DependencyNode::Type::DIRECT);

        auto& resource = _document.resources[node._index];
        ASSERT(resource.type == Resource::Type::STRING);

        return resource._string;
    }

    Array array() const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        ASSERT(node.type == DependencyNode::Type::ARRAY);

        return Array(_document, _treeIndex);
    }

    Value operator[](size_t index) const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        ASSERT(node.type == DependencyNode::Type::ARRAY);

        return Value(_document, node._array[index]);
    }

    Object object() const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        ASSERT(node.type == DependencyNode::Type::OBJECT);

        return Object(_document, _treeIndex);
    }
    
    // Returns null if key isn't found
    Value operator[](const String& key) const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        ASSERT(node.type == DependencyNode::Type::OBJECT);

        auto val = node._object.find(key);

        // Return null value if key is not found
        if (val == node._object.end())
            return Value(_document, 0);

        return Value(_document, (*val).value);
    }

    bool IsNull() const
    {
        auto& node = _document.dependencyTree[_treeIndex];
        ASSERT(node.type == DependencyNode::Type::DIRECT);

        auto& resource = _document.resources[node._index];
        return resource.type == Resource::Type::NONE;
    }
};

} // namespace json
