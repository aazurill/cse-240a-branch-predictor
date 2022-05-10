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
int ghistoryBits = 14; // Number of bits used for Global History


int ghBits= 12; // Number of bits used for global + chooser
int lhBits = 10; // Number of bits used for Local Branch Pattern
int pcBits = 10; // Number of bits used for Local pattern history
int bpType;       // Branch Prediction Type
int verbose;

//bimode
int choiceBits = 13;
int bimodehistoryBits = 13;
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
uint8_t *choice_prediction;
uint8_t *local_bht;
uint8_t *local_pht;
uint8_t *global_bht;

//bimode
uint8_t *choice_pht;
uint8_t *nt_pht;
uint8_t *t_pht;

// Helper outcome function
int outcome_generator(uint32_t counter) {
  if (counter == WN || counter == SN) {
    return 0;
  } else {
    return 1;
  }
  printf("GENERATOR FAILED\n");
  return -1;
}

// Helper saturator
int saturator(uint8_t outcome, uint8_t curr_state) {
  int ans = -1;
  switch(curr_state) {
    case SN:
      ans = (outcome == TAKEN) ? WN : SN;
      break;
    case WN:
      ans = (outcome == TAKEN) ? WT : SN;
      break;
    case WT:
      ans = (outcome == TAKEN) ? ST : WN;
      break;
    case ST:
      ans = (outcome == TAKEN) ? ST : WT;
      break;
    }
    return ans;
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
  printf("This is size of bht_gshare %d\n", i);
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
  global_bht = (uint8_t*)malloc((1 << ghBits) * sizeof(uint8_t));
  local_bht = (uint8_t*)malloc((1 << lhBits) * sizeof(uint8_t));
  local_pht = (uint8_t*)malloc((1 << pcBits) * sizeof(uint8_t));
  choice_prediction = (uint8_t*)malloc((1 << ghBits) * sizeof(uint8_t));

  int i = 0;
  for(i = 0; i < (1 << ghBits); i++){
    global_bht[i] = WN;
  }
  for(i = 0; i < (1 << lhBits); i++) {
    local_bht[i] = WN;
  }
  for(i = 0; i < (1 << pcBits); i++) {
    local_pht[i] = 0;
  }
  for(i = 0; i < (1 << ghBits); i++) {
    choice_prediction[i] = WT;
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
  int local_pht_ind = pc & ((1 << pcBits) -1);

  int local_bht_ind = local_pht[local_pht_ind];
  // printf("local ind is %d\n", local_bht_ind);


  int local_prediction = local_bht[local_bht_ind];
  // printf("local prediction is %d\n", local_prediction);

  // WN and SN = global_choice | ST and WT = local_choice
  int choice = choice_prediction[global_bht_ind];

  if(choice == WN || choice == SN) {
    if (global_prediction <= 1) {
      return NOTTAKEN;
    } else {
      return TAKEN;
    }
  } else {
    if (local_prediction <= 1) {
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

  int local_pht_ind = pc & ((1 << pcBits) -1);
  int local_bht_ind = local_pht[local_pht_ind] & ((1 << lhBits) -1);

  // Old predictions
  int global_prediction = global_bht[global_bht_ind];
  int local_prediction = local_bht[local_bht_ind];
  int choice = choice_prediction[global_bht_ind];

  // Train predictions using saturators
  local_bht[local_bht_ind] = saturator(outcome, local_prediction);
  global_bht[global_bht_ind] = saturator(outcome, global_prediction);

  // Train choice in case of differing predictions -> choose the more correct one
  if (outcome_generator(global_prediction) != outcome_generator(local_prediction)) {
    // If global prediction is correct, saturate choice w/ 0
    if (outcome_generator(global_prediction) == outcome) {
      choice_prediction[global_bht_ind] = saturator(0, choice);
    } else {
    // Else, saturate choice w/ 1
      choice_prediction[global_bht_ind] = saturator(1, choice);
    }
  }

  // Modify historys - NOTE THAT LOCAL NEEDS TO BE CUT OFF BY SIZE
  ghistory = (ghistory << 1) | outcome;
  local_pht[local_pht_ind] = ((local_bht_ind << 1) | outcome) & ((1 << pcBits) -1);
}

void
cleanup_trnmt() {
  free(global_bht);
  free(local_bht);
  free(local_pht);
  free(choice_prediction);
}

// -------------------------------------------------------------------
// bimode predictor

void init_custom() {
  printf("testing");
  choice_pht = (uint8_t*)malloc((1 << choiceBits) * sizeof(uint8_t));
  nt_pht = (uint8_t*)malloc((1 << choiceBits) * sizeof(uint8_t));
  t_pht = (uint8_t*)malloc((1 << choiceBits) * sizeof(uint8_t));

  int i = 0;
  for(i = 0; i < (1 << choiceBits); i++){
    choice_pht[i] = WN;
    nt_pht[i] = WN;
    t_pht[i] = WT;
  }

  ghistory = 0;

  printf("testing");
}

uint8_t custom_predict(uint32_t pc) {
  int bht_entries = 1 << bimodehistoryBits;
  int pc_lower_bits = pc & (bht_entries-1);
  int ghistory_lower_bits = ghistory & (bht_entries -1);
  int index = pc_lower_bits ^ ghistory_lower_bits;

  int direction_chosen = outcome_generator(choice_pht[pc_lower_bits]);
  // If choice to NT, go to NT pht. Else, go to T pht
  if (direction_chosen == 0) {
    return outcome_generator(nt_pht[index]);
  } else {
    return outcome_generator(t_pht[index]);
  }
}

void train_custom(uint32_t pc, uint8_t outcome) {
  int bht_entries = 1 << bimodehistoryBits;
  int pc_lower_bits = pc & (bht_entries-1);
  int ghistory_lower_bits = ghistory & (bht_entries -1);
  int index = pc_lower_bits ^ ghistory_lower_bits;

  int choice = choice_pht[pc_lower_bits];
  int direction_chosen = outcome_generator(choice_pht[pc_lower_bits]);
  int nt_prediction = nt_pht[index];
  int t_prediction = t_pht[index];
  int bimode_prediction = custom_predict(pc);

 // If NOT (outcome of choice != outcome && direction pht (which is wrong) predicts correct) update choice
  if (!(outcome_generator(choice) != outcome && bimode_prediction == outcome)) {
    choice_pht[index] = saturator(outcome, choice);
  }

  if (outcome_generator(choice) == 0) {
    nt_pht[index] = saturator(outcome, nt_prediction);
  } else {
    t_pht[index] = saturator(outcome, t_prediction);
  }

  //Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void
cleanup_custom() {
  free(choice_pht);
  free(t_pht);
  free(nt_pht);
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
      init_custom();
      break;
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
      return custom_predict(pc);
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
      return train_custom(pc, outcome);
    default:
      break;
  }


}