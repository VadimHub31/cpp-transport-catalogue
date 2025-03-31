#pragma once

#include <optional>

#include "json.h"

namespace json {

class Builder {
private:
    class Context;
    class KeyContext;
    class DictItemContext;
    class ArrayItemContext;

public:

    const Node& Build() const;

    Builder& Value(Node value);
    KeyContext Key(std::string key);
    DictItemContext StartDict();
    Builder& EndDict();
    ArrayItemContext StartArray();
    Builder& EndArray();
    
private:

    Node root_;
    std::vector<Node*> nodes_stack_;
    const Node* GetCurrentNode() const;
    void AddNode(Node node);

    std::optional<std::string> last_key_;

    class Context {
    public:
        Context(Builder& builder) : builder_(builder) {}
        Builder& Value(Node value) {
            return builder_.Value(move(value));
        }
        KeyContext Key(std::string key) {
            return builder_.Key(move(key));
        }
        DictItemContext StartDict() {
            return builder_.StartDict();
        }
        Builder& EndDict() {
            return builder_.EndDict();
        }
        ArrayItemContext StartArray() {
            return builder_.StartArray();
        }
        Builder& EndArray() {
            return builder_.EndArray();
        }

        const Node& Build() {
            return builder_.Build();
        }

    private:
        Builder& builder_;
    };

    class KeyContext : public Context {
    public:
        KeyContext(Builder& builder) : Context(builder) {}
        KeyContext Key(std::string key) = delete; 
        Builder& EndArray() = delete;
        Builder& EndDict() = delete;
        DictItemContext Value(Node value) {
            return Context::Value(move(value));
        }
        const Node& Build() = delete;
    };

    class ArrayItemContext : public Context {
    public:
        ArrayItemContext(Builder& builder) : Context(builder) {}
        Builder& EndDict() = delete;
        KeyContext Key(std::string key) = delete;
        ArrayItemContext Value(Node value) {
            return Context::Value(move(value));
        }
        ArrayItemContext StartArray() {
            return Context::StartArray();
        }
        const Node& Build() = delete;
    };

    class DictItemContext : public Context {
    public:
        DictItemContext(Builder& builder) : Context(builder) {}
        ArrayItemContext& StartArray() = delete;
        Builder& EndArray() = delete;
        DictItemContext StartDict() = delete;
        Builder& Value(Node value) = delete;
        const Node& Build() = delete;
    };

};

}