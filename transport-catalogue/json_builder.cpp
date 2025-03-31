#include "json_builder.h"

#include <stdexcept>
#include <utility>

using namespace std;

namespace json {

Builder& Builder::Value(Node value) {
    AddNode(move(value));
    return *this;
}

Builder::KeyContext Builder::Key(string key) {
    if (nodes_stack_.empty() || !GetCurrentNode()->IsDict()) {
        throw std::logic_error("Key without a Dict context"s);
    }
    
    if (last_key_) {
        throw std::logic_error("Key without a value"s);
    }

    last_key_ = std::move(key);
    
    return KeyContext(*this);
}

Builder::DictItemContext Builder::StartDict() {
    AddNode(Dict{});
    return DictItemContext(*this);
}

Builder& Builder::EndDict() {
    if (!GetCurrentNode()->IsDict()) {
        throw std::logic_error("It is not a dict to call EndDict"s);
    }
    nodes_stack_.pop_back();
    return *this;
}

Builder::ArrayItemContext Builder::StartArray() {
    AddNode(Array{});
    return ArrayItemContext(*this);
}

Builder& Builder::EndArray() {
    if (!GetCurrentNode()->IsArray()) {
        throw std::logic_error("It is not an array to call EndArray"s);
    }
    nodes_stack_.pop_back();
    return *this;
}

const Node& Builder::Build() const {
    if (root_.IsNull()/* || !nodes_stack_.empty()*/) {
        throw std::logic_error("Attempt to build JSON which is not completed"s);
    }
    return root_;
}

const Node* Builder::GetCurrentNode() const {
    if (nodes_stack_.empty()) {
        throw std::logic_error("Empty stack");
    }
    return nodes_stack_.back();
}

void Builder::AddNode(Node node) {
    if (root_.IsNull()/* && nodes_stack_.empty()*/) {
        root_ = move(node);
        if (root_.IsArray() || root_.IsDict()) {
            nodes_stack_.push_back(&root_);
        }
        return;
    }

    if (nodes_stack_.empty()) {
        throw std::logic_error("Adding node to completed document");
    }

    if (GetCurrentNode()->IsArray()) {
        Array& current_node = const_cast<Array&>(GetCurrentNode()->AsArray());
        current_node.emplace_back(move(node));
        Node* added = &current_node.back();
        if (added->IsArray() || added->IsDict()) {
            nodes_stack_.push_back(added);
        }
    }
    
    else if (GetCurrentNode()->IsDict()) {
        if (!last_key_) {
            throw std::logic_error("Value without a Key in Dict");
        }

        Dict& current_node = const_cast<Dict&>(GetCurrentNode()->AsDict());
        auto [it, inserted] = current_node.emplace(*last_key_, move(node));
        if (!inserted) {
            throw std::logic_error("Duplicate key");
        }

        Node* added = &it->second;
        last_key_.reset();
        if (added->IsArray() || added->IsDict()) {
            nodes_stack_.push_back(added);
        }
    }
    else {
        throw std::logic_error("Unavailable type of node"s);
    }
}

}