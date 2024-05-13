#include "FSTHierarchy.hh"

#include "Debug.hh"
#include "Sta.hh"

namespace sta {

    FSTHierarchyTree::FSTHierarchyTree(StaState *sta):
        StaState(sta),
        has_root_(false)
    {
    }

    uint64_t FSTHierarchyTree::push(FSTScope scope){
        uint64_t next_id = nodes_.size();
        //creates a new node that will be added to the tree below the current one
        FSTNode new_node;
        if(parents_.empty() && !has_root_){ //new root node
            new_node.value = scope;
            new_node.parent = next_id; //root nodes are their own parents
            new_node.n_children = 0;
            has_root_ = true;
        } else if(parents_.empty()) {
            report_->error(7780, "Encountered FST hierarchy with multiple roots!");
        } else { //insert simple child node
            new_node.value = scope;
            new_node.parent = parents_.back(); //parent id
            new_node.n_children = 0;
        }
        nodes_.push_back(new_node);
        for(auto p : parents_){
            nodes_[p].n_children++;
        }
        parents_.push_back(next_id);
        return next_id;
    }

    void FSTHierarchyTree::pop() {
        if(!parents_.empty()){
            parents_.pop_back();
        } //if the stack is empty do nothing
    }

    FSTScope& FSTHierarchyTree::peek_current(){
        if(!nodes_.empty()){
            return nodes_.back().value;
        }
        //this should not happen as we provide an empty() method
        report_->error(7781, "Tried to add FST var but no scope is available!");
    }

    bool FSTHierarchyTree::empty(){
        return nodes_.empty();
    }

    std::pair<FSTHierarchyTree::node_it, FSTHierarchyTree::node_it> 
    FSTHierarchyTree::all_children(uint64_t id){
        if(id > nodes_.size() - 1){
            report_->error(7783, "Illegal accessor for FST hierachy: %" PRIu64, id);
        }
        return std::make_pair(nodes_.begin() + id + 1, nodes_.begin() + id + 1 + nodes_[id].n_children);
    }

    FSTScope& FSTHierarchyTree::get(uint64_t id){
        if(id > nodes_.size() - 1){
            report_->error(7785, "Direct scope return failed, illegal accessor for FST hierachy: %" PRIu64, id);
        }
        return nodes_[id].value;
    }

}