#include "FSTReader.hh"

#include "fstapi.h"

#include "Debug.hh"
#include "Sta.hh"
#include "FST.hh"

namespace sta {

    class FSTReader : public StaState {
    public:
        FSTReader(StaState *sta);
        void read(const char *filename);
    private:
        FST fst_;
        void *ctx_;
    };

    FSTReader::FSTReader(StaState *sta):
        StaState(sta),
        fst_(sta)
        {
        debug_->setLevel("read_fst_activities", 8);
    }

    void FSTReader::read(const char *filename) {
        ctx_ = fstReaderOpen(filename);
        if(ctx_){
            debugPrint(debug_, "read_fst_activities", 3, "Successfully opened fst context from file `%s`", filename);
            uint64_t start_time = fstReaderGetStartTime(ctx_);
            uint64_t end_time = fstReaderGetEndTime(ctx_);
            fst_.setStartTime(start_time);
            fst_.setEndTime(end_time);
            debugPrint(debug_, "read_fst_activities", 3, "Simulation from %" PRIu64 " to %" PRIu64, start_time, end_time);
            signed char time_scale_mag = fstReaderGetTimescale(ctx_);
            std::string time_unit;
            double time_unit_scale = 1.0;
            switch(time_scale_mag){
                case -15:   time_unit = "fs"; time_unit_scale = 1e-15; break;
                case -12:   time_unit = "ps"; time_unit_scale = 1e-12; break;
                case -9:    time_unit = "ns"; time_unit_scale = 1e-09; break;
                case -6:    time_unit = "us"; time_unit_scale = 1e-06; break;
                case -3:    time_unit = "ms"; time_unit_scale = 1e-03; break;
                case 0:     time_unit = "s";  time_unit_scale =   1.0; break;
                default: 
                    report_->error(7778, "Invalid time scale magnitude %d.", (int)time_scale_mag);
            }
            fst_.setTimeUnit(time_unit, time_unit_scale);
            debugPrint(debug_, "read_fst_activities", 3, "Time scale %le or `%s`", time_unit_scale, time_unit.c_str());
            std::string version { fstReaderGetVersionString(ctx_) };
            std::string date { fstReaderGetDateString(ctx_) };
            debugPrint(debug_, "read_fst_activities", 3, "Date: %s", date.c_str());
            debugPrint(debug_, "read_fst_activities", 3, "Version: %s", version.c_str());
            fst_.setDate(date);
            fst_.setVersionString(version);
            // int64_t start_cycle = (double)start_time * time_unit_scale;
            // int64_t end_cycle = (double)end_time * time_unit_scale;
            // int64_t total_cycles = (end_cycle - start_cycle) + 1;
            // debugPrint(debug_, "read_fst_activities", 3, "end cycle: %lf", end_time * time_unit_scale);
            // debugPrint(debug_, "read_fst_activities", 3, "Total cycles: %" PRId64, total_cycles);

            fstReaderClose(ctx_);
        } else {
            report_->warn(7777, "Failed to open fst context with file %s", filename);
        }
    }

    void readFSTFile(const char *filename, StaState *sta) {
        FSTReader reader { sta };
        reader.read(filename);
    }

} //namespace