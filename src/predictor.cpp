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
#include <string.h>
#include <cstring>
#include <stdint.h>

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
int ghistoryBitsTournament = 16; // Number of bits used for Global History

int ghistoryBitsGshare = 17;

int lhistoryBits = 16;
int chooserBits = 10;

int chooserBitsTournament = 10;
int bpType;            // Branch Prediction Type
int verbose;

int ghistoryBitsCustom = 15;
int lhistoryBitsCustom = 15;
int chooserBitsCustom = 16;

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
void init_gshare(int gHistoryBitsX)
{
  int bht_entries = 1 << gHistoryBitsX;

  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc, int gHistoryBitsX)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << gHistoryBitsX;

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

void train_gshare(uint32_t pc, uint8_t outcome, int ghistoryBitsX)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBitsX;

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
  int bht_entries = 1 << ghistoryBitsTournament;
  int lht_entries = 1 << lhistoryBits;
  int selector_entries = 1 << chooserBitsTournament;

  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  bht_lht = (uint8_t *)malloc(lht_entries * sizeof(uint8_t)); 
  local_history = (uint64_t *)malloc(lht_entries * sizeof(uint64_t)); // Local history registers

  selector = (uint8_t *)malloc(selector_entries * sizeof(uint8_t)); // Selector table 

  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;  
  }
  for (i = 0; i< lht_entries; i++)
  {
    bht_lht[i] = WN;    
    local_history[i] = 0; 
  }
  for (i = 0; i< selector_entries; i++)
  {
    selector[i] = WN;  
  }

  ghistory = 0; 
}

uint32_t tournament_predict(uint32_t pc)
{
  uint32_t gshare_index = (pc ^ ghistory) & ((1 << ghistoryBitsTournament) - 1);
  uint32_t lht_index = pc & ((1 << lhistoryBits) - 1);
  uint32_t selector_index = (pc ^ ghistory) & ((1 << chooserBitsTournament) - 1);

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
  uint32_t bht_entries = 1 << ghistoryBitsTournament;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);

  uint32_t gshare_index = pc_lower_bits ^ ghistory_lower_bits;
  uint32_t lht_index = pc_lower_bits;

  //make two predictions
  uint8_t gshare_prediction = gshare_predict(pc, ghistoryBitsTournament);
  uint8_t lht_prediction = (bht_lht[lht_index] == WT || bht_lht[lht_index] == ST) ? TAKEN : NOTTAKEN;

  uint32_t selector_index = (pc ^ ghistory) & ((1 << chooserBitsTournament) - 1);

  // Update selector based on otucome for each selector and actual outcome
  if (gshare_prediction == outcome && lht_prediction != outcome) //if gshare is correct and local is wrong
  {
    if (selector[selector_index] < ST)
    {
      selector[selector_index]++; //increment gshare priority
    }
  }
  else if (lht_prediction == outcome && gshare_prediction != outcome) // if local is correct and gshare is wrong
  {
    if (selector[selector_index] > SN)
    {
      selector[selector_index]--; //decrement gshare priority
    }
  }

  // update lht based on actual outcome
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

  // update local history
  local_history[lht_index] = (local_history[lht_index] << 1) | outcome;

  // update gshare ght
  train_gshare(pc, outcome, ghistoryBitsTournament);
}

void cleanup_tournament()
{
  free(bht_gshare);
  free(bht_lht);
  free(local_history);
  free(selector);
}
//30, 130

#define HISTORY_LENGTH 5
#define NUM_PERCEPTRONS 5
#define THRESHOLD (1.93 * HISTORY_LENGTH + 14)
#define perceptron_threshold 250

uint32_t globalHistory = 0;  
int perceptrons[NUM_PERCEPTRONS][HISTORY_LENGTH + 1]; 

void init_perceptron() {
    memset(perceptrons, 0, sizeof(perceptrons));
}

uint8_t perceptron_prediction(uint32_t pc) {
    int index = pc % NUM_PERCEPTRONS;
    int y = perceptrons[index][0]; 
    
    for (int i = 0; i < HISTORY_LENGTH; i++) {
        int historyBit = (globalHistory >> i) & 1;
        y += perceptrons[index][i + 1] * (historyBit ? 1 : -1);
    }
    
    return y >= 0 ? TAKEN : NOTTAKEN;
}

void train_perceptron(uint32_t pc, uint8_t outcome) {
    int index = pc % NUM_PERCEPTRONS;
    int y = perceptrons[index][0];
    
    for (int i = 0; i < HISTORY_LENGTH; i++) {
        int historyBit = (globalHistory >> i) & 1;
        y += perceptrons[index][i + 1] * (historyBit ? 1 : -1);
    }
    
    int actual = outcome == TAKEN ? 1 : -1;
    
    if ((y >= 0) != (actual == 1) || abs(y) <= THRESHOLD) {
        perceptrons[index][0] += actual;
        for (int i = 0; i < HISTORY_LENGTH; i++) {
            int historyBit = (globalHistory >> i) & 1;
            perceptrons[index][i + 1] += actual * (historyBit ? 1 : -1);
        }
    }
    
    globalHistory = ((globalHistory << 1) | (outcome == TAKEN ? 1 : 0)) & ((1ULL << HISTORY_LENGTH) - 1); 
}

void init_custom()
{
  init_perceptron();
  //entries
  int bht_entries = 1 << ghistoryBitsCustom;
  int lht_entries = 1 << lhistoryBitsCustom;
  int selector_entries = 1 << chooserBitsCustom;

  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  bht_lht = (uint8_t *)malloc(lht_entries * sizeof(uint8_t)); 
  local_history = (uint64_t *)malloc(lht_entries * sizeof(uint64_t)); // Local history registers

  selector = (uint8_t *)malloc(selector_entries * sizeof(uint8_t)); // Selector table 

  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;  
  }
  for (i = 0; i< lht_entries; i++)
  {
    bht_lht[i] = WN;    
    local_history[i] = 0; 
  }
  for (i = 0; i< selector_entries; i++)
  {
    selector[i] = WN;  
  }

  ghistory = 0; 
}

uint32_t custom_predict(uint32_t pc)
{
  uint32_t gshare_index = (pc ^ ghistory) & ((1 << ghistoryBitsCustom) - 1);
  uint32_t lht_index = pc & ((1 << lhistoryBitsCustom) - 1);
  uint32_t selector_index = (pc ^ ghistory) & ((1 << chooserBitsCustom) - 1);

  //make two predictions
  uint8_t gshare_prediction = (bht_gshare[gshare_index] >= WT) ? TAKEN : NOTTAKEN;
  uint8_t lht_prediction = (bht_lht[lht_index] >= WT) ? TAKEN : NOTTAKEN;
  uint8_t perceptron_pred = perceptron_prediction(pc);

  //selector prediction
  uint8_t selector_value = selector[selector_index];

  uint8_t final_prediction = 0;

  //choose gshare or lht prediction based on selecgtor
  if (gshare_prediction == lht_prediction){
    return (selector_value >= WT) ? gshare_prediction : lht_prediction;
  } else {
    //return (selector_value >= WT) ? gshare_prediction : lht_prediction;
    if ((pc & 0xFF) > perceptron_threshold){
      final_prediction = (gshare_prediction + lht_prediction + (perceptron_pred >> 2)) / 2;
      return final_prediction >= 0 ? TAKEN : NOTTAKEN;
    }
    return (selector_value >= WT) ? gshare_prediction : lht_prediction;
  }
}

void train_custom(uint32_t pc, uint8_t outcome)
{
  train_perceptron(pc, outcome);
  uint32_t bht_entries = 1 << ghistoryBitsCustom;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);

  uint32_t gshare_index = pc_lower_bits ^ ghistory_lower_bits;
  uint32_t lht_index = pc_lower_bits;

  //make two predictions
  uint8_t gshare_prediction = gshare_predict(pc, ghistoryBitsCustom);
  uint8_t lht_prediction = (bht_lht[lht_index] == WT || bht_lht[lht_index] == ST) ? TAKEN : NOTTAKEN;
  uint8_t perceptron_pred = perceptron_prediction(pc);

  uint32_t selector_index = (pc ^ ghistory) & ((1 << chooserBitsCustom) - 1);

  // Update selector based on otucome for each selector and actual outcome
  if (gshare_prediction == outcome && lht_prediction != outcome) //if gshare is correct and local is wrong
  {
    if (selector[selector_index] < ST)
    {
      selector[selector_index]++; //increment gshare priority
    }
  }
  else if (lht_prediction == outcome && gshare_prediction != outcome) // if local is correct and gshare is wrong
  {
    if (selector[selector_index] > SN)
    {
      selector[selector_index]--; //decrement gshare priority
    }
  }

  // update lht based on actual outcome
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

  // update local history
  local_history[lht_index] = (local_history[lht_index] << 1) | outcome;

  // update gshare ght
  train_gshare(pc, outcome, ghistoryBitsCustom);
}




void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare(ghistoryBitsGshare);
    break;
  case TOURNAMENT:
    init_tournament(); 
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
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc, ghistoryBitsGshare);
  case TOURNAMENT:
    return tournament_predict(pc);
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

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome, ghistoryBitsGshare);
    case TOURNAMENT:
      return train_tournament(pc, outcome);
    case CUSTOM:
      return train_custom(pc, outcome);
    default:
      break;
    }
  }
}
