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
const char *studentName = "Akhil Deshmukh";
const char *studentID = "A69032277";
const char *email = "a1deshmukh@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 17; // Number of bits used for Global History
int lhistoryBits = 17;
int chooserBits = 17;
int bpType;            // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

// tournament
uint8_t *bht_lht;  
uint64_t *local_history; 

uint8_t *selector;  

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
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

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_gshare()
{
  free(bht_gshare);
}




void init_tournament()
{
  //entries
  int bht_entries = 1 << ghistoryBits;
  int lht_entries = 1 << lhistoryBits;
  int selector_entries = 1 << chooserBits;

  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  bht_lht = (uint8_t *)malloc(lht_entries * sizeof(uint8_t)); 
  local_history = (uint64_t *)malloc(lht_entries * sizeof(uint64_t)); // Local history registers

  selector = (uint8_t *)malloc(selector_entries * sizeof(uint8_t)); // Selector table 

  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;  
    bht_lht[i] = WN;    
    local_history[i] = 0; 
    selector[i] = WN;  
  }

  ghistory = 0; 
}

uint32_t tournament_predict(uint32_t pc)
{
  uint32_t gshare_index = (pc ^ ghistory) & ((1 << ghistoryBits) - 1);
  uint32_t lht_index = pc & ((1 << lhistoryBits) - 1);
  uint32_t selector_index = (pc ^ ghistory) & ((1 << chooserBits) - 1);

  //make two predictions
  uint8_t gshare_prediction = (bht_gshare[gshare_index] >= WT) ? TAKEN : NOTTAKEN;
  uint8_t lht_prediction = (bht_lht[lht_index] >= WT) ? TAKEN : NOTTAKEN;

  //selector prediction
  uint8_t selector_value = selector[selector_index];

  //choose gshare or lht prediction based on selecgtor
  return (selector_value >= WT) ? gshare_prediction : lht_prediction;
}

void train_tournament(uint32_t pc, uint8_t outcome)
{
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);

  uint32_t gshare_index = pc_lower_bits ^ ghistory_lower_bits;
  uint32_t lht_index = pc_lower_bits;

  // Update gshare
  train_gshare(pc, outcome);

  // update lht
  if (outcome == TAKEN)
  {
    if (bht_lht[lht_index] < ST)
    {
      bht_lht[lht_index]++;
    }
  }
  else
  {
    if (bht_lht[lht_index] > SN)
    {
      bht_lht[lht_index]--;
    }
  }

  // update lhr
  local_history[lht_index] = (local_history[lht_index] << 1) | outcome;

  // updat selector
  uint8_t gshare_prediction = gshare_predict(pc);
  uint8_t lht_prediction = (bht_lht[lht_index] == WT || bht_lht[lht_index] == ST) ? TAKEN : NOTTAKEN;

  if (gshare_prediction == outcome && lht_prediction != outcome)
  {
    if (selector[gshare_index] < ST)
    {
      selector[gshare_index]++;
    }
  }
  else if (lht_prediction == outcome && gshare_prediction != outcome)
  {
    if (selector[gshare_index] > SN)
    {
      selector[gshare_index]--;
    }
  }
}

void cleanup_tournament()
{
  free(bht_gshare);
  free(bht_lht);
  free(local_history);
  free(selector);
}




void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_tournament(); 
    break;
  case CUSTOM:
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return tournament_predict(pc);
  case CUSTOM:
    return NOTTAKEN;
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

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tournament(pc, outcome);
    case CUSTOM:
      return;
    default:
      break;
    }
  }
}
