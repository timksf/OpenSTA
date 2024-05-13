#include "ReadFSTActivities.hh"

#include <string>
#include <utility>

#include "Debug.hh"
#include "Sdc.hh"
#include "Sta.hh"
#include "Network.hh"
#include "Power.hh"
#include "VerilogNamespace.hh"
#include "ParseBus.hh"
#include "FSTReader.hh"

namespace sta {

    typedef Set<const Pin*> ConstPinSet; //reuse from ReadVcdActivities

    class ReadFSTActivities : public StaState {
    public:
        ReadFSTActivities(const char *filename, const char *scope, Sta *sta);
        void readActivities();
        void setActivities();
        void setVarActivity(FSTVar var);
        void setVarActivity(const std::string &var_name, const FSTValues &values, int bit);
        void findVarActivity(const FSTValues& vals, int value_bit, double &transition_count, double &activity, double &duty);
    private:
        Sta *sta_;
        FST fst_;
        FSTReader reader_;
        std::string filename_;
        std::string scope_;
        double clk_period_;
        Power *power_;
        ConstPinSet annotated_pins_;
    };

    //actual tcl function
    void readFSTActivities(const char *filename, const char *scope, Sta *sta) {
        ReadFSTActivities fst_act { filename, scope, sta };
        fst_act.readActivities();
    }

    ReadFSTActivities::ReadFSTActivities(const char *filename, const char *scope, Sta *sta): 
        StaState(sta),
        sta_(sta),
        fst_(sta),
        reader_(sta, filename),
        filename_(filename),
        scope_(scope),
        clk_period_(0.0),
        power_(sta->power())
    {
        debug_->setLevel("read_fst_activities", 8);
    };

    void ReadFSTActivities::readActivities() {
        debugPrint(debug_, "read_fst_activities", 3, "fst file: %s, scope: %s", filename_.c_str(), scope_.c_str());
        //the fst_ object will contain some general information and FSTVar's for all relevant variables (inside of scope_)
        fst_ = reader_.readHierarchy(scope_.c_str());

        clk_period_ = INF; //TODO ??
        for (Clock *clk : *sta_->sdc()->clocks())
            clk_period_ = std::min(static_cast<double>(clk->period()), clk_period_);

        debugPrint(debug_, "read_fst_activities", 3, "Clock period: %lf", clk_period_);

        if(fst_.endTime() > 0){
            setActivities();
        } else {
            report_->warn(7784, "VCD max time is zero.");
        }

    }

    void ReadFSTActivities::setActivities() {
        auto [var_begin, var_end] = fst_.vars();
        for(auto it = var_begin; it != var_end; ++it){
            FSTVar &var = *it;
            if(var.type == FST_VT_VCD_WIRE || var.type == FST_VT_VCD_REG){
                setVarActivity(var);
            }
        }
    }

    void ReadFSTActivities::setVarActivity(FSTVar var){
        FSTValues vals = reader_.readValuesForVar(var);
        if(var.length == 1){ //simple case, single signal
            std::string sta_name = netVerilogToSta(var.name.c_str());
            debugPrint(debug_, "read_fst_activities", 3, "var name `%s` to sta name `%s`", var.name.c_str(), sta_name.c_str());
        } else { //probably a bus signal
            //in the FST format bus signals have the brackets separated from the signal with a space
            auto delim_pos = var.name.find(' ');
            var.name.erase(delim_pos, 1);
            bool is_bus, is_range, subscript_wild;
            std::string bus_name;
            int from, to;
            parseBusName(var.name.c_str(), '[', ']', '\\',
                        is_bus, is_range, bus_name, from, to, subscript_wild);
            if (is_bus) {
                std::string sta_bus_name = netVerilogToSta(bus_name.c_str());
                debugPrint(debug_, "read_fst_activities", 3, "Parsed bus %s [%s] (from %d to %d)", bus_name.c_str(), sta_bus_name.c_str(), from, to);
                if(to < from) std::swap(to, from);
                for(int i = to, value_bit = 0; i <= from; i++) {
                    std::string pin_name = sta_bus_name + '[' + std::to_string(i) + ']';
                    setVarActivity(pin_name, vals, value_bit++);
                }
            } else
                report_->warn(7791, "problem parsing bus %s.", var.name.c_str());
        }
    }

    void ReadFSTActivities::setVarActivity(const std::string &pin_name, const FSTValues &values, int bit) {
        const Pin *pin = sdc_network_->findPin(pin_name.c_str());
        if(pin) {
            double transition_count, activity, duty;
            findVarActivity(values, bit, transition_count, activity, duty);
        } else
            report_->warn(7792, "could not find pin %s in sdc network", pin_name.c_str());
    }

    void ReadFSTActivities::findVarActivity(const FSTValues& vals, int value_bit, double &transition_count, double &activity, double &duty) {
        transition_count = 0.;
        char prev_value = vals[0].value[value_bit];
        uint64_t prev_time = vals[0].time;
        uint64_t high_time = 0;
        for(auto &v : vals){
            uint64_t time = v.time;
            char value = v.value[value_bit];
            debugPrint(debug_, "read_vcd_activities", 3, " %" PRId64 " %c", time, value);
            if(prev_value == '1')
                high_time += time - prev_time;
            if(value != prev_value) //transition
                transition_count += (value == 'X'
                           || value == 'Z'
                           || prev_value == 'X'
                           || prev_value == 'Z')
                ? .5
                : 1.0;
            prev_time = time;
            prev_value = value;
        }
        auto endTime = fst_.endTime();
        if(prev_value == '1')
            high_time += fst_.endTime() - prev_time;
        duty = static_cast<double>(high_time) / endTime;
        // activity = transition_count / (endTime * fst_.time)//TODO;
    }

}