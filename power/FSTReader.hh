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

        void readValuesForVar(FST &fst, FSTVar var);
        void readAllValues(FST &fst);
        void readValuesForChunk(FST &fst, FST::var_range value_range);
    private:
        struct fst_iter_data {
            Debug *debug;
            Report *report;
            FSTValues *vals;
            FSTVar *var;
            bool *error;
            FST *fst;
            uint64_t *ctr;
        };

        std::string filename_;
        void *ctx_;
    };

} //namespace