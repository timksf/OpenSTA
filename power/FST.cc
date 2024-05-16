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

    void FST::setTimeScale(double time_scale) {
        time_scale_ = time_scale;
    }

    void FST::setStartTime(uint64_t t){
        start_time_ = t;
    }

    void FST::setEndTime(uint64_t t){
        end_time_ = t;
    }

    void FST::setVars(std::vector<FSTVar> &&vars){
        vars_ = vars;
    }

    void FST::insertValue(fstHandle var_id, const FSTValue &value) {
        var_values_map_[var_id].push_back(value);
    }

}