#ifndef REGISTER_ALLOC_HPP
#define REGISTER_ALLOC_HPP

#include "tac_nodes.hpp"
#include "liveness_analysis.hpp"

#include <vector>
#include <unordered_map>
#include <algorithm>

// temps, if spilled, are stored in PTX local memory
struct AllocationTable {
    bool spill_happened;
    std::unordered_map<int,int> in_register_before; // registernumber:temp
    std::unordered_map<int,int> in_register_after; 
    std::unordered_map<int,int> in_memory; // temp_number:memory_offset
};

class RegisterAllocation {
public: 
    std::vector<AllocationTable> _allocations; // register allocation & spills for temps

    RegisterAllocation(
        int n, 
        const std::vector<std::unique_ptr<TACNode>>& tac_nodes,
        const std::unordered_map<int, LiveRange>& ranges,
        std::vector<LivenessInfo>& live_info_at_instruction, 
        const std::unordered_map<std::string, LiveRange>& var_ranges) 
            : _avail_pregisters{n}, 
            _instructions{tac_nodes}, // reg alloc needs to know what each tac node/line does. for arithmetic,load,store
            _live_ranges{ranges}, // live ranges to compute
            _live_info_at_instruction{live_info_at_instruction}, 
            _var_live_ranges{var_ranges} 
    {};

    void allocate() {
        _allocations.resize(_instructions.size()); 
        size_t i = 0; 
        while (i < _instructions.size()) {
            std::cout << "Instruction " << i << std::endl; 

            /* step 1: remove dead temps from active set */
            for (auto it = _active_temps_in_window.begin(); it != _active_temps_in_window.end(); ) {
                int active_temp = *it;
                LiveRange cur_temp_range = _live_ranges.at(active_temp);
                
                if (cur_temp_range.end <= i) {
                    _temp_state[active_temp] = TempState::DEAD;
                    it = _active_temps_in_window.erase(it);
                } else {
                    ++it;
                }
            }

            /* step 2: initialize before state from previous after state */
            if (i != 0) {
                _allocations[i].in_register_before = _allocations[i-1].in_register_after;
                _allocations[i].in_memory = _allocations[i-1].in_memory;
            }
            
            // remove dead temps from before state 
            for (auto it = _allocations[i].in_register_before.begin(); it != _allocations[i].in_register_before.end(); ) {
                if (_temp_state[it->second] == TempState::DEAD) {
                    it = _allocations[i].in_register_before.erase(it);
                } else {
                    ++it;
                }
            }

            /* step 3: load operands into registers (if spilled) */

            
            // live_after is important here. add all non-duplicate temps to the current active temporaries (active temps need a register)
            // non-duplicate because a duplicate just means that the temp was live before and still remains alive at this instruction
            // only one temp gets added to the active temp window at a time bc only one temp is created in a line
            std::set<int>::iterator inserted_it = _active_temps_in_window.end(); 
            for (auto& before : _live_info_at_instruction[i].live_before) {
                bool needs_register = false; 
                switch (_temp_state[before]) {
                    case TempState::IN_REGISTER:
                        auto it = _active_temps_in_window.insert(before);
                        if (it.second) {
                            inserted_it = it.first;
                            std::cout << "instruction " << i << " temp to add somewhere: " << *inserted_it << std::endl; 
                        }
                        break;
                    case TempState::IN_MEMORY:
                        // check if this temp 'after' is needed in this instruction for a binaryop, load, or a store  
                        std::cout << "check if temp " << before << " is needed in instruction" << i << std::endl; 
                        if (auto* tac_binary = dynamic_cast<TACBinaryOp*>(_instructions[i].get())) {
                            int operand1_id = -1, operand2_id = -1; 
                            if (std::holds_alternative<Temp>(tac_binary->_operand1)) {
                                if (std::get<Temp>(tac_binary->_operand1).identifer == before) {
                                    operand1_id = std::get<Temp>(tac_binary->_operand1).identifer;
                                }
                                // return std::get<Temp>(op).identifer == temp; 
                            }
                            if (std::holds_alternative<Temp>(tac_binary->_operand2)) {
                                if (std::get<Temp>(tac_binary->_operand2).identifer == before) {
                                    operand2_id = std::get<Temp>(tac_binary->_operand2).identifer;
                                }
                            }
                            if (operand1_id != -1) {
                                int reg1 = get_register_for_temp(i, operand1_id);
                                for (auto& [reg, t] : _allocations[i].in_register_before) {
                                    if (t == operand1_id) {
                                        reg1 = reg;
                                    }
                                }
                                
                                // Find its spill location
                                int spill_offset = -1;
                                for (auto& [offset, t] : _allocations[i].in_memory) {
                                    if (t == operand1_id) {
                                        spill_offset = offset;
                                        break;
                                    }
                                }
                                
                                if (spill_offset != -1) { // if in memory, have to get and load
                                    int free_reg = -1;
                                    // get the free register
                                    for (int r = 0; r < _avail_pregisters; ++r) {
                                        if (_allocations[i].in_register_before.find(r) == _allocations[i].in_register_before.end()) {
                                            free_reg = r;
                                            break;
                                        }
                                    }
                                    
                                    if (free_reg == -1) { // no free registers, so need to spill
                                        free_reg = 0; // TODO add spill logic
                                    }
                                    
                                    // load from memory to register
                                    _allocations[i].in_register_before[free_reg] = operand1_id;
                                    _temp_state[operand1_id] = TempState::IN_REGISTER;
                                    std::cout << "Loaded temp " << operand1_id << " from offset " << spill_offset 
                                            << " into register " << free_reg << std::endl;
                                    reg1 = free_reg;
                                }
                                
                                if (reg1 == -1) {
                                    // need to allocate register for operand1
                                    for (int r = 0; r < _avail_pregisters; ++r) {
                                        if (_allocations[i].in_register_before.find(r) == _allocations[i].in_register_before.end()) {
                                            _allocations[i].in_register_before[r] = operand1_id;
                                            _temp_state[operand1_id] = TempState::IN_REGISTER;
                                            break;
                                        }
                                    }
                                }
                            }

                            if (operand2_id != -1) {
                                int reg2 = get_register_for_temp(i, operand2_id);
                                for (auto& [reg, t] : _allocations[i].in_register_before) {
                                    if (t == operand2_id) {
                                        reg2 = reg;
                                    }
                                }
                                
                                // Find its spill location
                                int spill_offset = -1;
                                for (auto& [offset, t] : _allocations[i].in_memory) {
                                    if (t == operand2_id) {
                                        spill_offset = offset;
                                        break;
                                    }
                                }
                                
                                if (spill_offset != -1) { // if in memory, have to get and load
                                    int free_reg = -1;
                                    // get the free register
                                    for (int r = 0; r < _avail_pregisters; ++r) {
                                        if (_allocations[i].in_register_before.find(r) == _allocations[i].in_register_before.end()) {
                                            free_reg = r;
                                            break;
                                        }
                                    }
                                    
                                    if (free_reg == -1) { // no free registers, so need to spill
                                        free_reg = 0; // TODO add spill logic
                                    }
                                    
                                    // load from memory to register
                                    _allocations[i].in_register_before[free_reg] = operand2_id;
                                    _temp_state[operand2_id] = TempState::IN_REGISTER;
                                    std::cout << "Loaded temp " << operand2_id << " from offset " << spill_offset 
                                            << " into register " << free_reg << std::endl;
                                    reg2 = free_reg;
                                }
                                
                                if (reg2 == -1) {
                                    // need to allocate register for operand1
                                    for (int r = 0; r < _avail_pregisters; ++r) {
                                        if (_allocations[i].in_register_before.find(r) == _allocations[i].in_register_before.end()) {
                                            _allocations[i].in_register_before[r] = operand1_id;
                                            _temp_state[operand2_id] = TempState::IN_REGISTER;
                                            break;
                                        }
                                    }
                                }
                            }
                        } 
                        break;
                    case TempState::DEAD:
                        break; 
                    default:
                        _temp_state[before] = TempState::IN_REGISTER;
                        auto it = _active_temps_in_window.insert(before);
                        if (it.second) {
                            inserted_it = it.first;
                            std::cout << "instruction " << i << " temp to add somewhere: " << *inserted_it << std::endl; 
                        }
                        break;
                }
            }

            std::cout << "instruction " << i << " active temps: ["; 
            for (auto& at : _active_temps_in_window) {
                std::cout << at << ",";
            }
            std::cout << "]" << std::endl; 


            std::cout << "debug line 74" << std::endl; 
            // declare data structures for new allocation table
            std::unordered_map<int,int> i_instruction_registers;
            std::unordered_map<int,int> i_instruction_memory;

            // if the first instruction, don't need to / can't look back at previous allocation
            if (i == 0) {
                std::cout << "debug line 81" << std::endl; 
                int j = 0; 
                for (auto& temp : _active_temps_in_window) {
                    i_instruction_registers[j] = temp; 
                    ++j;
                }
                _allocations[i].spill_happened = false; 
                _allocations[i].in_register_before = {};
                _allocations[i].in_register_after = i_instruction_registers;
                _allocations[i].in_memory = {};
                ++i;
                continue;
            }

            std::cout << "debug line 91" << std::endl; 

            // the rest of the logic in the function is for instructions i > 0. 
            
            // only add values from registers if they are still live
            // AllocationTable prev_alloca = _allocations[i-1];
            std::cout << "debug line 94" << std::endl;
            for (auto& [reg,temp] : _allocations[i-1].in_register_after) {
                if (_temp_state[temp] != TempState::DEAD) {
                     i_instruction_registers[reg] = temp; 
                }
            }

            // only add values from memory if they are still live
            for (auto& [offset,temp] : _allocations[i-1].in_memory) {
                if (_temp_state[temp] != TempState::DEAD) {
                     i_instruction_memory[offset] = temp; 
                }
            }

            // if tac node is a store, just store TODO check logic on this
            if (auto* tac_store = dynamic_cast<TACStore*>(_instructions[i].get())) {
                TACVariable var = tac_store->_dest_variable;
                Temp temp = tac_store->_temporary; 
            }   

            std::cout << "debug line 100" << std::endl; 

            // check if more actives than physical register available. if yes, must swap values in registers and spill
            // TODO: find a way to save that a spill occured here
            // example: if there are 3 temps looking for registers and two physical registers available, 1 temp has to be spilled to memory. 
            if (_active_temps_in_window.size() > _avail_pregisters) {
                std::cout << "line 144 more active temps than avail registers" << std::endl; 
                /* 
                    the logic in this conditional only runs if 
                    there are more temps that need registers than 
                    the # of available physical registers. At
                    three-address code line 0, it is impossible for 
                    this to occur, and this it is ok to say i-1 below. 
                */
                if (_allocations[i-1].in_register_before.size() > _avail_pregisters) std::exit; 
                
                // check if r1 or r2 temp lives longer
                int longest_living_temp = -1; 
                int reg_being_modified; 
                for (auto& [reg,temp] : _allocations[i-1].in_register_before) {
                    if (_live_ranges.at(temp).end > longest_living_temp) {
                        longest_living_temp = temp; 
                        reg_being_modified = reg;
                    }
                }

                // save to memory 
                i_instruction_memory[_temp_memory_offset] = longest_living_temp;
                _temp_state[longest_living_temp] = TempState::IN_MEMORY;
                ++_temp_memory_offset;
                // remove spilled temp from active temps needing registers
                _active_temps_in_window.erase(longest_living_temp);
        
                // change the temp at the register that should be modified, to the inserted
                i_instruction_registers = _allocations[i-1].in_register_before;

                if (inserted_it != _active_temps_in_window.end()) {
                    i_instruction_registers[reg_being_modified] = *inserted_it;
                }
                

                // i_instruction_registers.assign(prev_alloca.in_register.begin(), prev_alloca.in_register.end());
                // auto it = find(i_instruction_registers.begin(), i_instruction_registers.end(), longest_living_temp);
                // i_instruction_registers.erase(it); 

                // create the allocation table for this instruction i
                _allocations[i].spill_happened = true;
                _allocations[i].in_register_after = i_instruction_registers;
                _allocations[i].in_memory = i_instruction_memory;
            } else {
                std::cout << "line 186 less active temps than avail registers" << std::endl; 
                // simply add to instruction registers and memory
                for (size_t k = 0; k < _avail_pregisters; ++k) {
                    std::cout << "line 189 for loop " << std::endl; 
                    if (i_instruction_registers.find(k) == i_instruction_registers.end() && inserted_it != _active_temps_in_window.end()) {
                        i_instruction_registers[k] = *inserted_it;
                        std::cout << "line 191 can't find register k in " << std::endl; 
                        break; 
                    // if the register exists/ is populated but the value inside the register is dead, insert *inserted_it into that register
                    // } else if (prev_alloca.in_register.find(k) != prev_alloca.in_register.end() && )
                    }
                }
                _allocations[i].spill_happened = false;
                _allocations[i].in_register_after = i_instruction_registers;
                _allocations[i].in_memory = _allocations[i-1].in_memory;
            }
            std::cout << "Allocation Table - registers{";
            for (auto& [r,t] : _allocations[i].in_register_after) {
                std::cout << r << ":" << t << ",";
            }
            std::cout << "}" << std::endl;
            ++i;
        }
    }

private:
    int _avail_pregisters; // this is the # of physical registers available, but also the size of the sliding window in the linear scan algorithm
    int _temp_memory_offset = 0; // will be used in instruction selection
    std::set<int> _active_temps_in_window; // a list of live intervals that are currently in registers. if size of this > _avail_pregisters then spill 
    
    // // added data structures for variables
    // std::set<std::string> _active_vars_in_window; 
    const std::unordered_map<std::string, LiveRange>& _var_live_ranges; 
    
    // temp to memory offset

    const std::vector<std::unique_ptr<TACNode>>& _instructions; 
    const std::unordered_map<int, LiveRange>& _live_ranges; 
    const std::vector<LivenessInfo>& _live_info_at_instruction;

    enum class TempState {
        IN_REGISTER, // the default
        IN_MEMORY, 
        DEAD
    };

    std::unordered_map<int, TempState> _temp_state; 
    std::unordered_map<std::string, TempState> _var_state; 
    
    // bool is_used_at_instruction(TACNode* instr, int temp, std::unordered_map<int,int>& reg_map, std::unordered_map<int,int>& mem_map) {
    //     if (auto* tac_binary = dynamic_cast<TACBinaryOp*>(instr)) {
    //         int operand1_id = -1, operand2_id = -1; 
    //         if (std::holds_alternative<Temp>(tac_binary->_operand1)) {
    //             operand1_id = std::get<Temp>(tac_binary->_operand1).identifer;
    //         }
    //         if (std::holds_alternative<Temp>(tac_binary->_operand2)) {
    //             operand2_id = std::get<Temp>(tac_binary->_operand2).identifer;
    //         }

    //         if (operand1_id != -1) {
    //             int reg1 = get_register_for_temp(operand1_id, reg_map, mem_map);
    //             if (reg1 == -1) {
    //                 // need to allocate register for operand1
    //                 for (int r = 0; r < _avail_pregisters; ++r) {
    //                     if (reg_map.find(r) == reg_map.end()) {
    //                         reg_map[r] = operand1_id;
    //                         _temp_state[operand1_id] = TempState::IN_REGISTER;
    //                         break;
    //                     }
    //                 }
    //             }
    //         }

    //         // auto check_operand = [temp](const Operand& op) -> bool {
    //         //     if (std::holds_alternative<Temp>(op)) {
    //         //         return std::get<Temp>(op).identifer == temp; 
    //         //     }
    //         //     return false; 
    //         // };
    //         // return check_operand(tac_binary->_operand1) || check_operand(tac_binary->_operand2); 
    //     } 
    //     if (auto* tac_store = dynamic_cast<TACStore*>(instr)) {
    //         return tac_store->_temporary.identifer == temp; 
    //     }
    //     if (auto* tac_load = dynamic_cast<TACLoad*>(instr)) {
    //         return tac_load->_dest_temp.identifer == temp; 
    //     }
    //     return false; 
    // }

    // get a register for a temp, loading from memory if needed
    int get_register_for_temp(int idx, int temp_id) {
        for (auto& [reg, t] : _allocations[idx].in_register_before) {
            if (t == temp_id) {
                return reg;
            }
        }
        
        // Temp not in register - check if it's spilled
        if (_temp_state[temp_id] == TempState::IN_MEMORY) {
            // Find its spill location
            int spill_offset = -1;
            for (auto& [offset, t] : _allocations[idx].in_memory) {
                if (t == temp_id) {
                    spill_offset = offset;
                    break;
                }
            }
            
            if (spill_offset != -1) { // if in memory, have to get and load
                int free_reg = -1;
                // get the free register
                for (int r = 0; r < _avail_pregisters; ++r) {
                    if (_allocations[idx].in_register_before.find(r) == _allocations[idx].in_register_before.end()) {
                        free_reg = r;
                        break;
                    }
                }
                
                if (free_reg == -1) { // no free registers, so need to spill
                    free_reg = 0; // TODO add spill logic
                }
                
                // load from memory to register
                _allocations[idx].in_register_before[free_reg] = temp_id;
                _temp_state[temp_id] = TempState::IN_REGISTER;
                std::cout << "Loaded temp " << temp_id << " from offset " << spill_offset 
                        << " into register " << free_reg << std::endl;
                return free_reg;
            }
        }
        return -1; // Not found
    }
};

#endif