/* Author: Jared Stark;   Created: Tue Jul 27 13:39:15 PDT 2004
 * Description: This file defines a gshare branch predictor.
*/

#ifndef PREDICTOR_H_SEEN
#define PREDICTOR_H_SEEN

#include <cstddef>
#include <inttypes.h>
#include <vector>
#include "op_state.h"   // defines op_state_c (architectural state) class 
#include "tread.h"      // defines branch_record_c class
#include <iostream>
#include <map>
class PREDICTOR
{
  public:
    typedef uint32_t address_t;

  private:
    static const int num_DNF_terms = 6;
	 
    struct branch_info_t
    {
		std::vector < std::vector<int> > DNF;
		int num_terms;    
		int last_N_pred; 
		int iteration; //determine if just beginning building the branch 
		int num_pred_T_but_NT;
    };
    typedef uint32_t history_t;
    typedef uint8_t counter_t;
    static const int BHR_LENGTH = 4; //global BHR 
    static const history_t BHR_MSB = (history_t(1) << (BHR_LENGTH - 1));
    static const std::size_t PHT_SIZE = (std::size_t(1) << BHR_LENGTH);
    static const std::size_t PHT_INDEX_MASK = (PHT_SIZE - 1);
    static const counter_t PHT_INIT = /* weakly taken */ 2;

    history_t bhr;                // 4 bits
    std::vector<counter_t> pht;   // 64K bits
    std::map<address_t, branch_info_t> branch_table; 
    
    void update_bhr(bool taken) { bhr >>= 1; if (taken) bhr |= BHR_MSB; }//right shift so most recent is MSB 
    static std::size_t pht_index(address_t pc, history_t bhr) 
        { return (static_cast<std::size_t>(pc ^ bhr) & PHT_INDEX_MASK); }
    static bool counter_msb(/* 2-bit counter */ counter_t cnt) { return (cnt >= 2); }
    static counter_t counter_inc(/* 2-bit counter */ counter_t cnt)
        { if (cnt != 3) ++cnt; return cnt; }
    static counter_t counter_dec(/* 2-bit counter */ counter_t cnt)
        { if (cnt != 0) --cnt; return cnt; }

  public:
    PREDICTOR(void) : bhr(0), pht(PHT_SIZE, counter_t(PHT_INIT)) { }
    // uses compiler generated copy constructor
    // uses compiler generated destructor
    // uses compiler generated assignment operator

    // get_prediction() takes a branch record (br, branch_record_c is defined in
    // tread.h) and architectural state (os, op_state_c is defined op_state.h).
    // Your predictor should use this information to figure out what prediction it
    // wants to make.  Keep in mind you're only obligated to make predictions for
    // conditional branches.
    bool get_prediction(const branch_record_c* br, const op_state_c* os)
        {
            bool prediction = false;
			
            if (/* conditional branch */ br->is_conditional) 
			{
				/*
                address_t pc = br->instruction_addr;
                std::size_t index = pht_index(pc, bhr);
                counter_t cnt = pht[index];
                prediction = counter_msb(cnt);
				*/
				address_t pc = br->instruction_addr; 
				if (branch_table.find(pc) == branch_table.end())
				{
					//branch does not exist in map
					branch_info_t branch_info;  
					branch_info.DNF.resize(num_DNF_terms); 
					for (int i = 0; i < num_DNF_terms; i++)
					{
						(branch_info.DNF[i]).resize(BHR_LENGTH); 
					} 
					branch_info.num_terms = 1; 
					branch_info.last_N_pred = 0;  
					branch_info.iteration = 0;  
					branch_info.num_pred_T_but_NT = 0; 
					branch_table[pc] = branch_info; //insert into map  
				}
				else
				{
					//branch exists in map
					int result = 1;  
					for (int i = 0; i < branch_table[pc].num_terms; i++)
					{
						for (int j = 0; j < BHR_LENGTH; j++)
						{
							if (branch_table[pc].DNF[i][j] == 1)
							{
								result = result & ((bhr >> (BHR_LENGTH-j)) & 1);  
								if (result == 0) {
									break; 
								}
							}
							else if (branch_table[pc].DNF[i][j] == 0)
							{
								result = result & ~((bhr >> (BHR_LENGTH-j)) & 1);
 								if (result == 0) {
                                    break;
                                }
 
							}		
							else //-1 
							{
								//do nothing 
							}

							if (j == BHR_LENGTH-1)
							{
								if (result == 1)
								{
									prediction = true; 
									goto end; 
								}
							}
						}
					}
				}
				
            }
			end:
		
			
	    	//printf("bhr: %d \n",bhr);  
	    	return prediction;   // true for taken, false for not taken
        }

    // Update the predictor after a prediction has been made.  This should accept
    // the branch record (br) and architectural state (os), as well as a third
    // argument (taken) indicating whether or not the branch was taken.
    void update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
        {
            if (/* conditional branch */ br->is_conditional) {
                address_t pc = br->instruction_addr;
				branch_info_t branch_info = branch_table[pc]; 
				/*
                std::size_t index = pht_index(pc, bhr);
                counter_t cnt = pht[index];
                if (taken)
                    cnt = counter_inc(cnt);
                else
                    cnt = counter_dec(cnt);
                pht[index] = cnt;
                update_bhr(taken);
				*/
				if (get_prediction(br,os) == false && taken == true)
				{
					if (branch_info.iteration == 1)
					{
						//DNF = bhr 
						for(int i = 0; i < BHR_LENGTH; i++)
                    	{
                        	branch_table[pc].DNF[0][i] =  ((bhr >> (BHR_LENGTH-i)) & 1); 
						} 
	
					}
					else if ((((double)branch_info.last_N_pred/10.0) < .7))
					{
						int num_deleted = 0; 
						int delete_index = branch_info.num_terms-1; //delete from most recent disjunction 
						for(int i = 0; i < BHR_LENGTH; i++)
						{
							if ( ( (((bhr >> (BHR_LENGTH-i)) & 1) == 1) && branch_info.DNF[delete_index][i] == 0 ) ||
							( (((bhr >> (BHR_LENGTH-i)) & 1) == 0) && branch_info.DNF[delete_index][i] == 1 ) )
							{
								branch_table[pc].DNF[delete_index][i] = -1; 
							}
						} 
						if (num_deleted == 0)
						{
							//add disjunction 
						}
					}
				}
				
				
            }
        }
};

#endif // PREDICTOR_H_SEEN

