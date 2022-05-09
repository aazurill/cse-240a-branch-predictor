//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"
//
// TODO:Student Information
//
const char *studentName = "Ethan Tan";
const char *studentID   = "A15814695";
const char *email       = "ettan@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//
// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

//define number of bits required for indexing the BHT here.
int ghistoryBits = 10; // Number of bits used for Global History


int ghBits= 10; // Number of bits used for first level Local Branch Pattern History
int lhBits = 10; // Number of bits used for Local Branch Pattern
int choiceBits = 10; // Number of bits used for Choice Predictor
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
//gshare
uint64_t ghistory;
uint8_t *bht_gshare;

//tournament
uint32_t *choice_prediction;
uint64_t *local_bht;
uint32_t *local_pht;
uint32_t *global_bht;

// Helper outcome function
uint8_t outcome_generator(uint32_t counter) {
  if (counter == WN || counter == SN) {
    return 0;
  } else {
    return 1;
  }
  return 0;
}


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

//gshare functions
void init_gshare() {
  printf("Hello World. Initializing gshare\n");
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t*)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for(i = 0; i< bht_entries; i++){
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc) {
  //get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries-1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries -1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch(bht_gshare[index]){
    case WN:
      return NOTTAKEN;
    case SN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf("Warning: Undefined state of entry in GSHARE BHT!\n");
      return NOTTAKEN;
  }
}

void train_gshare(uint32_t pc, uint8_t outcome) {
  //get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries-1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries -1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  //Update state of entry in bht based on outcome
  switch(bht_gshare[index]){
    case WN:
      bht_gshare[index] = (outcome==TAKEN)?WT:SN;
      break;
    case SN:
      bht_gshare[index] = (outcome==TAKEN)?WN:SN;
      break;
    case WT:
      bht_gshare[index] = (outcome==TAKEN)?ST:WN;
      break;
    case ST:
      bht_gshare[index] = (outcome==TAKEN)?ST:WT;
      break;
    default:
      printf("Warning: Undefined state of entry in GSHARE BHT!\n");
  }

  //Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_gshare() {
  free(bht_gshare);
}

// -------------------------------------------------------------------
// tournamnet predictor

//gshare functions
void init_trnmt() {
  global_bht = (uint32_t*)malloc((1 << ghBits) * sizeof(uint32_t));
  local_bht = (uint64_t*)malloc((1 << lhBits) * sizeof(uint64_t));
  local_pht = (uint32_t*)malloc((1 << lhBits) * sizeof(uint32_t));
  choice_prediction = (uint32_t*)malloc((1 << choiceBits) * sizeof(uint32_t));

  int i = 0;
  for(i = 0; i < (1 << ghBits); i++){
    global_bht[i] = WN;
    choice_prediction[i] = WN;


    // reminder that:
      // local_history_table[pc] = pattern
      // local_pattern_table[pattern] = saturating counter prediction
    local_pht[i] = 0;
    local_bht[i] = WN;
  }
  ghistory = 0;
}



uint8_t trnmt_predict(uint32_t pc) {

  // printf("\n\nMade it to predict\n\n");
  //get lower ghistoryBits of pc
  int global_bht_ind = ghistory & ((1 << ghBits) -1);

  // printf("global ind %d\n", global_bht_ind);
  int global_prediction = global_bht[global_bht_ind];
  // printf("global prediction is %d\n", global_prediction);
  // REMIND: local_pht map pc -> branch history
  //         local_bht map branch history -> prediction
  int local_pht_ind = pc & ((1 << lhBits) -1);

  int local_bht_ind = local_pht[local_pht_ind];
  // printf("local ind is %d\n", local_bht_ind);


  int local_prediction = local_bht[local_bht_ind];
  // printf("local prediction is %d\n", local_prediction);


  // WN and SN = global_choice | ST and WT = local_choice
  int choice = choice_prediction[global_bht_ind];

  if(choice == WN || choice == SN) {
    if (global_prediction == WN || global_prediction == SN) {
      return NOTTAKEN;
    } else {
      return TAKEN;
    }
  } else {
    if (local_prediction == WN || local_prediction == SN) {
      return NOTTAKEN;
    } else {
      return TAKEN;
    }
  }

  // printf("Finished predict\n");
}

void train_trnmt(uint32_t pc, uint8_t outcome) {
  // printf("\n\nMade it to train\n\n");
  //get lower ghistoryBits of pc
  int global_bht_ind = ghistory & ((1 << ghBits) -1);
  int global_prediction = global_bht[global_bht_ind];

  // printf("Global check is clear \n");
  // REMIND: local_pht map pc -> branch history
  //         local_bht map branch history -> prediction
  int local_pht_ind = pc & ((1 << lhBits) -1);
  // printf("local pht ind %d\n", local_pht_ind);
  int local_bht_ind = local_pht[local_pht_ind];
  // printf("local bht ind %d\n", local_pht_ind);
  int local_prediction = local_bht[local_bht_ind];
  // printf("Local check is clear \n");


  // WN and SN = global_choice | ST and WT = local_choice
  int choice = choice_prediction[global_bht_ind];

  // printf("glob: %d local: %d choice: %d\n", global_prediction, local_prediction, choice);

  //Update state of entry in bht based on outcome
  switch(global_bht[global_bht_ind]){
    case WN:
      global_bht[global_bht_ind] = (outcome==TAKEN)?WT:SN;
      break;
    case SN:
      global_bht[global_bht_ind] = (outcome==TAKEN)?WN:SN;
      break;
    case WT:
      global_bht[global_bht_ind] = (outcome==TAKEN)?ST:WN;
      break;
    case ST:
      global_bht[global_bht_ind] = (outcome==TAKEN)?ST:WT;
      break;
    default:
      printf("Warning: Undefined state of entry in GSHARE BHT!\n");
  }

  //Update history register
  ghistory = ((ghistory << 1) | outcome);

  // Training local
  switch(local_bht[local_bht_ind]) {
    case SN:
      local_bht[local_bht_ind] = (outcome == TAKEN) ? WN : SN;
      break;
    case WN:
      local_bht[local_bht_ind] = (outcome == TAKEN) ? WT : SN;
      break;
    case WT:
      local_bht[local_bht_ind] = (outcome == TAKEN) ? ST : WN;
      break;
    case ST:
      local_bht[local_bht_ind] = (outcome == TAKEN) ? ST : WT;
      break;
    default:
      printf("Warning: Undefined state of entry in LOCAL BHT!\n");
  }

  int local_hist = ((local_pht[local_pht_ind] <<1) & ((1 << lhBits) -1)| outcome);
  local_pht[local_pht_ind] = local_hist;

  // Training choice
  int choice_train;
  if (outcome_generator(local_prediction) != outcome_generator(global_prediction)) {
    if (outcome == NOTTAKEN) {
    // If local is in the correct direction, move choice predictor to stronger
    if (local_prediction == WN || local_prediction == SN) {
      choice_train = 1;
    } else {
      choice_train = 0;
    }
    } else {
      if (global_prediction == WN || global_prediction == SN) {
        choice_train = 0;
      } else {
        choice_train = 1;
      }
    }
  }

  int old_choice = choice_prediction[global_bht_ind];

// if choice is local, means that choice_prediction needs to be more positive
// if choice is global, means choice_prediction needs to be more negative
switch(choice_prediction[global_bht_ind]) {
  case SN:
    choice_prediction[global_bht_ind] = (choice_train == TAKEN) ? WN : SN;
    break;
  case WN:
    choice_prediction[global_bht_ind] = (choice_train == TAKEN) ? WT : SN;
    break;
  case WT:
      choice_prediction[global_bht_ind] = (choice_train == TAKEN) ? ST : WN;
    break;
  case ST:
    choice_prediction[global_bht_ind] = (choice_train == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in LOCAL BHT!\n");
  }

  // if (old_choice != 0 && old_choice != 3) {
  // printf("Old local: %d Old global: %d Old choice: %d | N local: %llu N global: %d N Choice: %d\n Outcome: %d", local_prediction, global_prediction, old_choice, local_bht[local_bht_ind], global_bht[global_bht_ind], choice_prediction[global_bht_ind], outcome);
  // }
}

void
cleanup_trnmt() {
  free(global_bht);
  free(local_bht);
  free(local_pht);
  free(choice_prediction);
}

void
init_predictor()
{
  switch (bpType) {
    case STATIC:
    case GSHARE:
      init_gshare();
      break;
    case TOURNAMENT:
      init_trnmt();
      break;
    case CUSTOM:
    default:
      break;
  }

}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_predict(pc);
    case TOURNAMENT:
      return trnmt_predict(pc);
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void
train_predictor(uint32_t pc, uint8_t outcome)
{

  switch (bpType) {
    case STATIC:
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_trnmt(pc, outcome);
    case CUSTOM:
    default:
      break;
  }


}