#include "FSTHierarchy.hh"

#include "Debug.hh"
#include "Sta.hh"

namespace sta {

    template<class V>
    void FSTHierarchyTree<V>::push(V data){
        //creates a new node that will be added to the tree below the current one
        if(parents_.empty()){ //add new root node
            FSTNode new_root = { data, nodes_.size() };
        }
    }

}