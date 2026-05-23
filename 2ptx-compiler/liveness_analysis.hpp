#ifndef LIVENESS_ANALYSIS_HPP
#define LIVENESS_ANALYSIS_HPP

#include "tac_nodes.hpp"

#include <set>
#include <vector>
#include <memory>
#include <set>
#include <cassert>
#include <unordered_map>

struct LivenessInfo {
    std::set<int> live_before; 
    std::set<int> live_after; 
    std::set<std::string> live_var_before; 
    std::set<std::string> live_var_after; 
};

struct LiveRange {
    int start; 
    int end; 
};

struct VarLiveRange {
    int start; 
    int end; 
};

class LivenessAnalysis {
public: 
    std::unordered_map<int, LiveRange> temp_to_live_range; // use this when register allocating

    std::unordered_map<std::string, LiveRange> var_to_live_range; 

    std::vector<LivenessInfo> instruction_liveness; //liveness at each instruction i. not specific to temps 

    std::unordered_map<std::string, int> var_def_index;

    void analyze(const std::vector<std::unique_ptr<TACNode>>& tac_instructions) {
        size_t n = tac_instructions.size(); 
        instruction_liveness.resize(n); 

        // track last use of temporary for range calculation

        std::set<int> live_set; 
        std::set<std::string> live_vars_set; 

        for (int i = n - 1; i >= 0; --i) {
            instruction_liveness[i].live_after = live_set;
            instruction_liveness[i].live_var_after = live_vars_set; // get live variables

            // get temp defs and uses
            std::vector<int> defs = get_definition(tac_instructions[i].get());
            std::vector<int> uses = get_uses(tac_instructions[i].get());

            // get var defs and uses
            std::vector<std::string> var_defs = get_var_definition(tac_instructions[i].get());
            std::vector<std::string> var_uses = get_var_uses(tac_instructions[i].get());

            // remove all definitions from the live set at this time, they aren't referenced anywhere above (chain starts at the definition, so the temporary will never show up in a line above)
            for (int def : defs) { // temps
                live_set.erase(def); 
            }
            for (std::string def : var_defs) { // vars
                live_vars_set.erase(def); 
                var_def_index[def] = i;
            }

            // insert all uses into the live set, as they are still live. A use in line i means there are potentially more uses in earlier lines, and definitely a def. 
            for (int use : uses) { // temps
                live_set.insert(use);
            }
            for (std::string use : var_uses) { // temps
                live_vars_set.insert(use);
            }

            instruction_liveness[i].live_before = live_set; 
            instruction_liveness[i].live_var_before = live_vars_set; 
        }
        // compute ranges of each temporary from the liveness info at each line
        compute_live_ranges(); 

        // var to live range
        for (const auto& [var, def_idx] : var_def_index) {
            if (var_to_live_range.find(var) == var_to_live_range.end()) {
                var_to_live_range[var] = {def_idx, def_idx};
                instruction_liveness[def_idx].live_var_after.insert(var);
            }
        }
    }

private: 
    void compute_live_ranges() {
        std::unordered_map<int,int> first_use;
        std::unordered_map<int,int> last_use; 

        std::unordered_map<std::string, int> var_first_use; 
        std::unordered_map<std::string, int> var_last_use; 

        for (size_t i = 0; i < instruction_liveness.size(); ++i) {
            for (int temp : instruction_liveness[i].live_before) {
                if (first_use.find(temp) == first_use.end()) {
                    first_use[temp] = i; 
                }
                last_use[temp] = i; 
            }
            for (int temp : instruction_liveness[i].live_after) {
                if (first_use.find(temp) == first_use.end()) {
                    first_use[temp] = i; 
                }
                last_use[temp] = i; 
            }
            for (std::string var : instruction_liveness[i].live_var_before) {
                if (var_first_use.find(var) == var_first_use.end()) {
                    var_first_use[var] = i; 
                }
                var_last_use[var] = i; 
            }
            for (std::string var : instruction_liveness[i].live_var_after) {
                if (var_first_use.find(var) == var_first_use.end()) {
                    var_first_use[var] = i; 
                }
                var_last_use[var] = i; 
            }
        }
        temp_to_live_range.clear(); 
        for (const auto& [temp, first] : first_use) {
            temp_to_live_range[temp] = {first, last_use[temp]}; 
        }

        var_to_live_range.clear(); 
        for (const auto& [var, first] : var_first_use) {
            var_to_live_range[var] = {first, var_last_use[var]};
        }

        return; 
    }

    bool is_temp(const Operand& op) {
        return std::holds_alternative<Temp>(op); 
    }

    std::vector<int> get_definition(TACNode* instr) {
        std::vector<int> defs; 

        if (auto* tac_constant = dynamic_cast<TACConstant*>(instr)) {
            defs.push_back(tac_constant->_dest.identifer); 
        } else if (auto* tac_binary = dynamic_cast<TACBinaryOp*>(instr)) {
            defs.push_back(tac_binary->_dest.identifer); 
        } else if (auto* tac_load = dynamic_cast<TACLoad*>(instr)) {
            defs.push_back(tac_load->_dest_temp.identifer);
        } else if (auto* tac_store = dynamic_cast<TACStore*>(instr)) {
        }
        return defs;
    }

    std::vector<std::string> get_var_definition(TACNode* instr) {
        std::vector<std::string> var_defs; 
        if (auto* tac_store = dynamic_cast<TACStore*>(instr)) {
            var_defs.push_back(tac_store->_dest_variable.varname);
        }
        return var_defs;
    }

    std::vector<int> get_uses(TACNode* instr) {        
        std::vector<int> uses;  

        if (auto* tac_binary = dynamic_cast<TACBinaryOp*>(instr)) {
            if (is_temp(tac_binary->_operand1)) uses.push_back(std::get<Temp>(tac_binary->_operand1).identifer); //TODO doesn't fully work I think, need to also factor in TACVariable
            if (is_temp(tac_binary->_operand2)) uses.push_back(std::get<Temp>(tac_binary->_operand2).identifer);
        } else if (auto* tac_store = dynamic_cast<TACStore*>(instr)) {
            if (is_temp(tac_store->_temporary)) uses.push_back(tac_store->_temporary.identifer);
        }
        return uses; 
    } 

    std::vector<std::string> get_var_uses(TACNode* instr) {
        std::vector<std::string> var_uses; 

        if (auto* tac_load = dynamic_cast<TACLoad*>(instr)) {
            var_uses.push_back(tac_load->_var.varname);
        }
        return var_uses; 
    }
};

#endif