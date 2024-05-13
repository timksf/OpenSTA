#include "ReadFSTActivities.hh"

#include <string>

#include "Debug.hh"
#include "Sta.hh"
#include "FSTReader.hh"

namespace sta {

    class ReadFSTActivities : public StaState {
    public:
        ReadFSTActivities(const char *filename, const char *scope, Sta *sta);
        void readActivities();
    private:
        Sta *sta_;
        std::string filename_;
        std::string scope_;
    };

    //actual tcl function
    void readFSTActivities(const char *filename, const char *scope, Sta *sta) {
        ReadFSTActivities fst_act { filename, scope, sta };
        fst_act.readActivities();
    }

    ReadFSTActivities::ReadFSTActivities(const char *filename, const char *scope, Sta *sta): 
        StaState(sta),
        sta_(sta),
        filename_(filename),
        scope_(scope)
    {
        debug_->setLevel("read_fst_activities", 8);
    };

    void ReadFSTActivities::readActivities() {
        debugPrint(debug_, "read_fst_activities", 3, "fst file: %s, scope: %s", filename_.c_str(), scope_.c_str());
        FST fst = readFSTFile(filename_.c_str(), sta_);
    }
}