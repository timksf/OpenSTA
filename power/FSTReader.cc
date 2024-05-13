#include "FSTReader.hh"

#include <vector>
#include <stack>
#include <map>

#include "fstapi.h"

#include "Debug.hh"
#include "Sta.hh"
#include "FST.hh"

namespace sta {

    struct FSTVar;

    struct FSTScope {
        uint8_t type;
        std::string name;
        std::string component;
    };

    struct FSTVar {
        uint8_t type;
        std::string name;
        uint32_t length;
        fstHandle handle;
        bool is_alias;
    };

    class FSTReader : public StaState {
    /*
        This FST reader flattens all scopes into the desired scope or the root scope if 
        no scope is provided. This means that it accumulates a list of FSTVar for all variables
        belonging to this scope and all children scopes.
    */
    public:
        FSTReader(StaState *sta, const char *scope);
        FST read(const char *filename);
    private:
        void *ctx_;
        std::string scope_;
        std::stack<FSTScope> scope_stack_;
        std::vector<FSTVar> vars;
    };

    FSTReader::FSTReader(StaState *sta, const char *scope):
        StaState(sta),
        scope_(scope)
        {
        debug_->setLevel("read_fst_activities", 8);
    }

    FST FSTReader::read(const char *filename) {
        FST fst(this);
        ctx_ = fstReaderOpen(filename);
        if(ctx_){
            debugPrint(debug_, "read_fst_activities", 3, "Successfully opened fst context from file `%s`", filename);
            uint64_t start_time = fstReaderGetStartTime(ctx_);
            uint64_t end_time = fstReaderGetEndTime(ctx_);
            fst.setStartTime(start_time);
            fst.setEndTime(end_time);
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
            fst.setTimeUnit(time_unit, time_unit_scale);
            debugPrint(debug_, "read_fst_activities", 3, "Time scale %le or `%s`", time_unit_scale, time_unit.c_str());
            std::string version { fstReaderGetVersionString(ctx_) };
            std::string date { fstReaderGetDateString(ctx_) };
            debugPrint(debug_, "read_fst_activities", 3, "Date: %s", date.c_str());
            debugPrint(debug_, "read_fst_activities", 3, "Version: %s", version.c_str());
            fst.setDate(date);
            fst.setVersionString(version);

            uint64_t var_count = fstReaderGetVarCount(ctx_);
            debugPrint(debug_, "read_fst_activities", 3, "FST file contains #variables: %" PRIu64, var_count);

            //read hierarchy for vars
            struct fstHier *h;
            bool record_vars = false;
            while((h = fstReaderIterateHier(ctx_))){
                switch(h->htyp){
                    case FST_HT_SCOPE: //scope begin
                    {   
                        FSTScope new_scope;
                        new_scope.name = h->u.scope.name;
                        new_scope.component = h->u.scope.component;
                        new_scope.type = h->u.scope.typ;
                        debugPrint(debug_, "read_fst_activities", 3, "Scope Name: %s", new_scope.name.c_str());
                        scope_stack_.push(new_scope);
                    }
                    case FST_HT_UPSCOPE: //scope end, ignore if not building with a stack
                        if(!scope_stack_.empty()) {
                            scope_stack_.pop();
                        } else {
                            report_->error(7779, "Trailing UPSCOPE encountered with empty stack!");
                        }
                        break;
                    case FST_HT_VAR:
                    {
                        FSTVar new_var;
                        new_var.type = h->u.var.typ;
                        new_var.name = h->u.var.name;
                        new_var.length = h->u.var.length;
                        new_var.handle = h->u.var.handle;
                        new_var.is_alias = h->u.var.is_alias;
                        debugPrint(debug_, "read_fst_activities", 3, "Var Name: %s, Length: %d", new_var.name.c_str(), new_var.length);
                        if(scope_stack_.empty())
                            report_->error(7779, "No scope for var: %s! Aborting...", new_var.name.c_str());
                        FSTScope& current_scope = scope_stack_.top();
                        current_scope.vars.push_back(new_var);
                    }
                    break;
                    //TODO:
                    case FST_HT_ATTRBEGIN: break;
                    case FST_HT_ATTREND: break;
                    default : break;
                }
            }

            fstReaderClose(ctx_);
        } else {
            report_->error(7777, "Failed to open fst context with file %s", filename);
        }
        return fst;
    }

    FST readFSTFile(const char *filename, const char* scope, StaState *sta) {
        FSTReader reader { sta, scope };
        return reader.read(filename);
    }

} //namespace