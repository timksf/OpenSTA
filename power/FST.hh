#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <utility>

#include "fstapi.h"

#include "StaState.hh"

namespace sta {

    struct FSTVar;

    struct FSTScope {
        uint8_t type;
        std::string name;
        std::string component;
        std::vector<FSTVar> vars;
    };

    struct FSTVar {
        uint8_t type;
        std::string name;
        uint32_t length;
        fstHandle handle;
        bool is_alias;
    };

    struct FSTValue {
        uint64_t time;
        std::string value; //for bus values, there are multiple char's
    };

    using FSTValues = std::vector<FSTValue>;
    class FST : public StaState {
    public:
        using vvars_t = std::vector<FSTVar>;
        using iterator = vvars_t::iterator;
        using var_range = std::pair<iterator, iterator>;

        FST(StaState *sta);
        void setDate(const std::string &date);
        void setTimeUnit(const std::string &time_unit, double time_unit_scale);
        void setVersionString(const std::string &version);
        void setStartTime(uint64_t t);
        void setEndTime(uint64_t t);
        void setStartCycle(int64_t t);
        void setEndCycle(int64_t t);
        void setTotalCycles(int64_t t);
        void setVars(vvars_t &&vars);

        inline uint64_t startTime() { return start_time_; }
        inline uint64_t endTime() { return end_time_; }

        inline var_range vars() {
            return std::make_pair(vars_.begin(), vars_.end());
        };
    private:
        std::string date_;
        std::string version_;

        double time_unit_scale_;
        std::string time_unit_;
        //FST does not support separate time unit and time unit scale AFAIK
        static constexpr double time_scale = 1.0;

        uint64_t start_time_;
        uint64_t end_time_;

        int64_t start_cycle_;
        int64_t end_cycle_;
        int64_t total_cycles_;

        vvars_t vars_;
    };

    
} // namespace name
