#include "FST.hh"

#include "Debug.hh"
#include "Report.hh"

namespace sta {

    FST::FST(StaState *sta):
        StaState(sta),
        time_unit_scale_(1.0),
        time_unit_("s")
    {
    }

    void FST::setDate(const std::string &date){
        date_ = date;
    }

    void FST::setTimeUnit(const std::string &time_unit, double time_unit_scale){
        time_unit_ = time_unit;
        time_unit_scale_ = time_unit_scale;
    }

    void FST::setVersionString(const std::string &version){
        version_ = version;
    }

    void FST::setStartTime(uint64_t t){
        start_time_ = t;
    }

    void FST::setEndTime(uint64_t t){
        end_time_ = t;
    }

    void FST::setStartCycle(int64_t t){
        start_cycle_ = t;
    }

    void FST::setEndCycle(int64_t t){
        end_cycle_ = t;
    }

    void FST::setTotalCycles(int64_t t){
        total_cycles_ = t;
    }

}