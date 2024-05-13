#pragma once

#include <vector>
#include <stack>


namespace sta {

class StaState;

template<class V>
struct FSTNode {
    V value;
    uint64_t parent;
};

template<class V>
class FSTHierarchyTree {
/*
    This tree is used to represent the scope hierarchy read from a FST file.    
*/
public:
    FSTHierarchyTree();
    void push(V data);
    void pop();
private:
    std::vector<FSTNode<V>> nodes_;
    std::stack<uint64_t> parents_;
};


}
