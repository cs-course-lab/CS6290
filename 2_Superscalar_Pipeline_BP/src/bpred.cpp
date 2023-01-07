// --------------------------------------------------------------------- //
// For part B, you will need to modify this file.                        //
// You may add any code you need, as long as you correctly implement the //
// three required BPred methods already listed in this file.             //
// --------------------------------------------------------------------- //

// bpred.cpp
// Implements the branch predictor class.

#include "bpred.h"

/**
 * Construct a branch predictor with the given policy.
 * 
 * In part B of the lab, you must implement this constructor.
 * 
 * @param policy the policy this branch predictor should use
 */
BPred::BPred(BPredPolicy policy) : policy(policy)
{
    // TODO: Initialize member variables here.
    
    /** The total number of branches this branch predictor has seen. */
    stat_num_branches = 0;
    /** The number of branches this branch predictor has mispredicted. */
    stat_num_mispred = 0;
    GHR = 0;//(1 << 13) - 1;
    // As a reminder, you can declare any additional member variables you need
    // in the BPred class in bpred.h and initialize them here.
}

/**
 * Get a prediction for the branch with the given address.
 * 
 * In part B of the lab, you must implement this method.
 * 
 * @param pc the address (program counter) of the branch to predict
 * @return the prediction for whether the branch is taken or not taken
 */
BranchDirection BPred::predict(uint64_t pc)
{
    if (policy == BPRED_ALWAYS_TAKEN){
        return TAKEN;
    }
    else{ // BPRED_GSHARE
        // Should be Modified for B2-2

        // PC(12bit) ^ GHR(12bit) ==> Key of PHT
        uint16_t cur_key = xor_12b(pc, GHR);
        if (PHT.find(cur_key) == PHT.end()){
            PHT[cur_key] = 2;
        }
        return ((((PHT[cur_key] ^ (2)) >> 1) & 1) == 1 ? NOT_TAKEN : TAKEN);
        // return PHT[Key]
        // return NOT_TAKEN;
    }
    // TODO: Return a prediction for whether the branch at address pc will be
    // TAKEN or NOT_TAKEN according to this branch predictor's policy.

    // Note that you do not have to handle the BPRED_PERFECT policy here; this
    // function will not be called for that policy.

    // return TAKEN; // This is just a placeholder.
}


/**
 * Update the branch predictor statistics (stat_num_branches and
 * stat_num_mispred), as well as any other internal state you may need to
 * update in the branch predictor.
 * 
 * In part B of the lab, you must implement this method.
 * 
 * @param pc the address (program counter) of the branch
 * @param prediction the prediction made by the branch predictor
 * @param resolution the actual outcome of the branch
 */
void BPred::update(uint64_t pc, BranchDirection prediction,
                   BranchDirection resolution)
{
    /** The total number of branches this branch predictor has seen. */
    stat_num_branches ++;
    /** The number of branches this branch predictor has mispredicted. */
    if (prediction != resolution)
        stat_num_mispred ++;
    
    if (policy == BPRED_GSHARE){
        // Key: GHR ^ PC
        uint16_t PHT_key = xor_12b(pc, GHR);
        // update PHT[Key] (2 input: cur stage, T/NT)
        PHT_update(PHT_key, resolution);
        // GHR update (resolution)
        GHR_update(resolution);
        // 
        // 
    }
    // TODO: Update the stat_num_branches and stat_num_mispred member variables
    // according to the prediction and resolution of the branch.


    // TODO: Update any other internal state you may need to keep track of.

    // Note that you do not have to handle the BPRED_PERFECT policy here; this
    // function will not be called for that policy.
}

void BPred::PHT_update(uint16_t key, BranchDirection resolution){
    if (PHT.find(key) != PHT.end()){
        PHT[key] = PHT_get_next_stage(PHT[key], resolution);
    }else{
        PHT[key] = PHT_get_next_stage(2, resolution);
    }
}

uint8_t BPred::PHT_get_next_stage(uint8_t cur_val, BranchDirection res){
    if (cur_val == 3){  // Strong Taken
        if(res == TAKEN){
            return 3;
        }else{
            return 2;
        }
    }else if(cur_val == 2){  // Weak Taken
        if(res == TAKEN){
            return 3;
        }else{
            return 1;
        }
    }else if(cur_val == 1){ // Weak NT
        if(res == TAKEN){
            return 2;
        }else{
            return 0;
        }
    }else{ // Strong NT
        if(res == TAKEN){ 
            return 1;
        }else{
            return 0;
        }
    }
}

void BPred::GHR_update(BranchDirection resolution){
    GHR = (((1 << 12) - 1) & ((GHR << 1) | (resolution == TAKEN ? 1 : 0)));
}

uint16_t BPred::xor_12b(uint64_t pc, uint16_t ghr){
    return ((1 << 12) - 1) & ((((1 << 12) - 1) & pc) ^ ghr);
}