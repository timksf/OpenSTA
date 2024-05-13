#include "FSTReader.hh"

#include <vector>
#include <stack>
#include <map>

#include "fstapi.h"

#include "Debug.hh"
#include "Sta.hh"
#include "FST.hh"
#include "FSTHierarchy.hh"

namespace sta {

    FSTReader::FSTReader(StaState *sta, const char *filename):
        StaState(sta),
        filename_(filename)
        {
        debug_->setLevel("read_fst_activities", 8);
        //open the context here and keep in memory until reader destroyed
        ctx_ = fstReaderOpen(filename);
    }

    FSTReader::~FSTReader(){
        //clean up fst reader context
        fstReaderClose(ctx_);
    }

    FST FSTReader::readHierarchy(const std::string &scope) {
        FST fst(this);
        if(ctx_){
            debugPrint(debug_, "read_fst_activities", 3, "Successfully opened fst context from file `%s`", filename_.c_str());
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
            uint64_t scope_id;
            bool found_scope = false;
            FSTHierarchyTree hier_tree_{ this };
            while((h = fstReaderIterateHier(ctx_))){
                switch(h->htyp){
                    case FST_HT_SCOPE: //scope begin
                    {   
                        FSTScope new_scope;
                        new_scope.name = h->u.scope.name;
                        new_scope.component = h->u.scope.component;
                        new_scope.type = h->u.scope.typ;
                        debugPrint(debug_, "read_fst_activities", 3, "Discovered scope with name: %s", new_scope.name.c_str());
                        auto id = hier_tree_.push(new_scope);
                        if((scope == "" || scope == new_scope.name) && !found_scope){ //remember scope id for later
                            scope_id = id;
                            found_scope = true;
                        }
                    }
                    case FST_HT_UPSCOPE:
                        hier_tree_.pop();
                        break;
                    case FST_HT_VAR:
                    {
                        FSTVar new_var;
                        new_var.type = h->u.var.typ;
                        new_var.name = h->u.var.name;
                        new_var.length = h->u.var.length;
                        new_var.handle = h->u.var.handle;
                        new_var.is_alias = h->u.var.is_alias;
                        debugPrint(debug_, "read_fst_activities", 3, "Var name: %s, Length: %d", new_var.name.c_str(), new_var.length);
                        if(hier_tree_.empty())
                            report_->error(7779, "No scope for var: %s! Aborting...", new_var.name.c_str());
                        FSTScope& current_scope = hier_tree_.peek_current();
                        current_scope.vars.push_back(new_var);
                        debugPrint(debug_, "read_fst_activities", 3, "Added var %s to scope %s", new_var.name.c_str(), current_scope.name.c_str());
                    }
                    break;
                    //TODO:
                    case FST_HT_ATTRBEGIN: break;
                    case FST_HT_ATTREND: break;
                    default : break;
                }
            }
            if(found_scope) {
                std::vector<FSTScope> all_scopes;
                auto [s, e] = hier_tree_.all_children(scope_id);
                std::transform(s, e, std::back_inserter(all_scopes), [](FSTNode &node){ return node.value; });
                debugPrint(debug_, "read_fst_activities", 3, "Scope %s has a total of %" PRIu64 " child scopes", scope.c_str(), all_scopes.size());
                int i = 0;
                for(auto &sc : all_scopes){
                    debugPrint(debug_, "read_fst_activities", 3, "Child scope %d: %s", ++i, sc.name.c_str());
                }
                all_scopes.insert(all_scopes.begin(), hier_tree_.get(scope_id)); //add top level scope
                std::vector<FSTVar> all_vars; //all relevant vars
                for(auto &sc : all_scopes){
                    //extend vars with those from all descendant scopes
                    all_vars.insert(all_vars.end(), sc.vars.begin(), sc.vars.end());
                }
                debugPrint(debug_, "read_fst_activities", 3, "Accumulated %" PRIu64 " variables", all_vars.size());
                fst.setVars(std::move(all_vars)); //transfer ownership to FST instance since it is no longer needed here
            } else {
                report_->error(7782, "Specified scope %s not found!", scope.c_str());
            }
        } else {
            report_->error(7777, "Failed to open fst context with file %s!", filename_.c_str());
        }
        return fst;
    }

    FSTValues FSTReader::readValuesForVar(FSTVar var) {
        if(!ctx_) report_->error(7777, "FST context for file %s not open!", filename_.c_str());
        FSTValues values;
        bool error = false;
        //let the API do the iteration
        struct fst_iter_data {
            Debug *debug;
            Report *report;
            FSTValues *v;
            FSTVar *var;
            bool *error;  
        };
        fst_iter_data usr_iter_data;
        usr_iter_data.debug = this->debug_;
        usr_iter_data.report = this->report_;
        usr_iter_data.v = &values;
        usr_iter_data.var = &var;
        usr_iter_data.error = &error;
        debugPrint(debug_, "read_fst_activities", 3, "Executing callback for %s", var.name.c_str());
        fstReaderSetFacProcessMask(ctx_, var.handle);
        fstReaderIterBlocks(ctx_,
        +[](void *usr, uint64_t time, [[maybe_unused]] fstHandle facidx, const unsigned char *value){
            struct fst_iter_data *usr_data = (struct fst_iter_data*) usr;
            // debugPrint(usr_data->debug, "read_fst_activities", 3, "var %s@%" PRIu64 "=%s", usr_data->var->name.c_str(), time, value);
            std::string value_s { reinterpret_cast<const char*>(value) };
            if(value_s.length() != usr_data->var->length){
                usr_data->report->warn(7789, "Variable length does not match value length: %" PRIu64 " vs %" PRIu32, value_s.length(), usr_data->var->length);
                *usr_data->error = true; //do not want to throw exceptions in extern "C"
            }
            usr_data->v->push_back(FSTValue { time, value_s });
        }, (void*)&usr_iter_data, nullptr);
        fstReaderClrFacProcessMask(ctx_, var.handle);
        if(error) 
            report_->error(7890, "Encountered error during read of value changes");
        return values;
    }

} //namespace