#pragma once

#include <cstdint>
#include <vector>
#include <stack>
#include <utility>
#include <string>

#include "fstapi.h"

#include "Sta.hh"
#include "FST.hh"

namespace sta {

    class StaState;
    struct FSTScope;

    struct FSTNode {
        FSTScope value;
        uint64_t parent;
        uint64_t n_children;
    };

    class FSTHierarchyTree: public StaState {
    /*
        This tree is used to represent the scope hierarchy read from a FST file.    
    */
    public:
        using node_it = std::vector<FSTNode>::iterator;

        FSTHierarchyTree(StaState *sta);
        uint64_t push(FSTScope scope);
        void pop();
        FSTScope& peek_current();
        bool empty();
        std::pair<node_it, node_it> all_children(uint64_t id);
        FSTScope& get(uint64_t id);
    private:
        std::vector<FSTNode> nodes_;
        //stack-like data structure for current path
        std::vector<uint64_t> parents_;
        bool has_root_; //FST hierarchies can only have one root
    };


}
