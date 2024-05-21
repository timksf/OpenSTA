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
        void setVarActivity(FSTVar var, std::string &var_name);
        void setVarActivity(const std::string &var_name, const FSTValues &values, int bit);
        void findVarActivity(const FSTValues& vals, int value_bit, double &transition_count, double &activity, double &duty);
        void checkClockPeriod(const Pin *pin, double transition_count);
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
        // debug_->setLevel("read_fst_activities", 8);
    };

    void ReadFSTActivities::readActivities() {
        debugPrint(debug_, "read_fst_activities", 3, "fst file: %s, scope: %s", filename_.c_str(), scope_.c_str());
        //the fst_ object will contain some general information and FSTVar's for all relevant variables (inside of scope_)
        fst_ = reader_.readHierarchy(scope_.c_str());
        // reader_.readAllValues(fst_);
        clk_period_ = INF; //TODO ??
        for (Clock *clk : *sta_->sdc()->clocks())
            clk_period_ = std::min(static_cast<double>(clk->period()), clk_period_);

        debugPrint(debug_, "read_fst_activities", 3, "Clock period: %le", clk_period_);

        if(fst_.endTime() > 0){
            setActivities();
        } else {
            report_->warn(7784, "VCD max time is zero.");
        }
        report_->reportLine("Annotated %lu pin activities.", annotated_pins_.size());
    }

    void ReadFSTActivities::setActivities() {
        auto [var_begin, var_end] = fst_.vars();
        report_->reportLine("Total variables to set activities for: %" PRIu64, std::distance(var_begin, var_end));
        const int CHUNKSIZE = 32;//std::distance(var_begin, var_end);
        int trailing = std::distance(var_begin, var_end) % CHUNKSIZE;
        auto process_var = [this](const FSTVar &var){
            if(var.type == FST_VT_VCD_WIRE || var.type == FST_VT_VCD_REG){
                //strip variable name of root scope prefix for locating it in sdc network
                std::string var_name = var.name;
                if(scope_.length() > 0){
                    if(var_name.substr(0, scope_.length()) == scope_){
                        var_name = var_name.substr(scope_.length() + 1); //+1 removes trailing `/`
                    }
                }
                setVarActivity(var, var_name);
            }
        };
        //process in chunks
        auto it = var_begin;
        for(; it != (var_end - trailing); it+=CHUNKSIZE){
            //load all values relevant to this chunk
            reader_.readValuesForChunk(fst_, std::make_pair(it, it+CHUNKSIZE));
            for(auto it1 = it; it1 != it+CHUNKSIZE; it1++) { 
                process_var(*it1);
            }
            fst_.clearValues(); //after a chunk has been processed, delete all corresponding entries
        }
        fst_.clearValues();
        //process trailing elements one by one
        for(;it != var_end; ++it){
            reader_.readValuesForVar(fst_, *it);
            process_var(*it);
            //no clearing since only a few remaining vars are processed
        }

    }

    void ReadFSTActivities::setVarActivity(FSTVar var, std::string &var_name){
        FSTValues &vals = fst_.valuesForVar(var);
        if(var.length == 1){ //simple case, single signal
            std::string sta_name = netVerilogToSta(var_name.c_str());
            // debugPrint(debug_, "read_fst_activities", 3, "var name `%s` to sta name `%s`", var_name.c_str(), sta_name.c_str());
            setVarActivity(sta_name, vals, 0);
        } else { //probably a bus signal
            //remove space in front of range annotation in wide signal
            auto delim_pos = var_name.find(' ');
            var_name.erase(delim_pos, 1);
            bool is_bus, is_range, subscript_wild;
            std::string bus_name;
            int from, to;
            parseBusName(var_name.c_str(), '[', ']', '\\',
                        is_bus, is_range, bus_name, from, to, subscript_wild);
            if (is_bus) {
                std::string sta_bus_name = netVerilogToSta(bus_name.c_str());
                debugPrint(debug_, "read_fst_activities", 10, "Parsed bus %s [%s] (from %d to %d)", 
                    bus_name.c_str(), sta_bus_name.c_str(), from, to);
                if(to < from) std::swap(to, from);
                for(int i = from, value_bit = 0; i <= to; i++) {
                    std::string pin_name = sta_bus_name + '[' + std::to_string(i) + ']';
                    // debugPrint(debug_, "read_fst_activities", 3, "bus values %s", pin_name.c_str());
                    setVarActivity(pin_name, vals, value_bit++);
                }
            } else
                report_->warn(7791, "problem parsing bus %s.", var_name.c_str());
        }
    }

    void ReadFSTActivities::setVarActivity(const std::string &pin_name, const FSTValues &values, int bit) {
        const Pin *pin = sdc_network_->findPin(pin_name.c_str());
        if(pin) {
            double transition_count, activity, duty;
            debugPrint(debug_, "read_fst_activities", 3, "%s values", pin_name.c_str());
            findVarActivity(values, bit, transition_count, activity, duty);
            debugPrint(debug_, "read_fst_activities", 3, "%s transitions %.1f activity %.2f duty %.2f", 
                pin_name.c_str(), transition_count, activity, duty);
            if(sdc_->isLeafPinClock(pin))
                checkClockPeriod(pin, transition_count);
            power_->setUserActivity(pin, activity, duty, PwrActivityOrigin::fst);
            annotated_pins_.insert(pin);
        } else
            debugPrint(debug_, "read_fst_activities", 9, "could not find pin %s in sdc network", pin_name.c_str());
    }

    void ReadFSTActivities::findVarActivity(const FSTValues& vals, int value_bit, double &transition_count, double &activity, double &duty) {
        transition_count = 0.;
        char prev_value = vals[0].value[value_bit];
        uint64_t prev_time = vals[0].time;
        uint64_t high_time = 0;
        for(auto &v : vals){
            uint64_t time = v.time;
            char value = v.value[value_bit];
            debugPrint(debug_, "read_fst_activities", 3, " %" PRId64 " %c (from %s)", time, value, v.value.c_str());
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
        auto end_time = fst_.endTime();
        if(prev_value == '1')
            high_time += end_time - prev_time;
        duty = static_cast<double>(high_time) / end_time;
        activity = transition_count / (end_time * fst_.timeScale() / clk_period_);//TODO;
    }

    void ReadFSTActivities::checkClockPeriod(const Pin *pin, double transition_count) {
        uint64_t end_time = fst_.endTime();
        debugPrint(debug_, "read_fst_activities", 10, "time scale %le", fst_.timeScale());
        double sim_period = end_time * fst_.timeScale() / (transition_count / 2.0);
        ClockSet *clks = sdc_->findLeafPinClocks(pin);
        if (clks) {
            for (Clock *clk : *clks) {
                double clk_period = clk->period();
                debugPrint(debug_, "read_fst_activities", 3, "sim_period of %s calculated as %le", clk->name(), sim_period);
                if (abs((clk_period - sim_period) / clk_period) > .1)
                    // Warn if sim clock period differs from SDC by 10%.
                    report_->warn(7793, "clock `%s` vcd period `%s` differs from SDC clock period `%s`",
                                clk->name(),
                                delayAsString(sim_period, this),
                                delayAsString(clk_period, this));
                }
        }
    }

}