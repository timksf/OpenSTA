#pragma once

#include <string>
#include <cstdint>

#include "StaState.hh"

namespace sta {

    class FST : public StaState {
    public:
        FST(StaState *sta);
        void setDate(const std::string &date);
        void setTimeUnit(const std::string &time_unit, double time_unit_scale);
        void setVersionString(const std::string &version);
        void setStartTime(uint64_t t);
        void setEndTime(uint64_t t);
        void setStartCycle(int64_t t);
        void setEndCycle(int64_t t);
        void setTotalCycles(int64_t t);
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
    };

    
} // namespace name
