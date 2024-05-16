#pragma once

#include "FST.hh"

namespace sta {

    class StaState;

    class FSTReader : public StaState {
    public:
        FSTReader(StaState *sta, const char *filename);
        ~FSTReader();
        //the scope parameter acts as a filter
        FST readHierarchy(const std::string &scope);
        FSTValues readValuesForVar(FSTVar var);
        void readAllValues(FST &fst);
    private:
        std::string filename_;
        void *ctx_;
    };

} //namespace