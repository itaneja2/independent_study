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
#include <stdlib.h>     
#include <time.h>       /* time */



using namespace std; 
class PREDICTOR
{
  public:
    typedef uint32_t address_t;

  private:
    static const int num_DNF_terms = 2;
	 
    struct branch_info_t
    {
		std::vector < std::vector<int> > DNF;
		int num_terms; //number of vectors in vector 
		int num_incorrect; //how many times incorrect for each branch 
		int iteration; //determine if just beginning building the branch 
		int num_pred_T_but_NT; //how many predicted taken but actually not taken
                int num_pred_NT_but_T; 
                int num_pred_T_actually_T; 
		int num_elim; //so we don't delete all terms from branch 
                int delete_all; //times we could have potentially deleted all the terms but instead possibly add a new vector
                std::map<int, double> exceptions; //which ones to block
                std::map<int, int> data; //branch data
                std::map<int, int> theoretical; //theoreitcal
                std::map<int, int> ghr_num_pred_T_but_NT; //mapping ghr to num predicted T but actually NT
                std::map<int, int> ghr_num_pred_NT_actually_NT; 
                int num_pred_NT_actually_NT;
                int consec_pred_T_but_NT;
                double num_actual_T; 
                double num_actual_NT;
                bool initialized; 
                std::vector <int> weight_vector; 
              

    };
    typedef uint32_t history_t;
    typedef uint8_t counter_t;
    int global_pred_NT;
    int global_pred_nonexcep; 
    int global_incorrect_nonexcep; 
    int global_incorrect;
    int global_wrong_nt_but_t; 
    int global_wrong_t_but_nt;
    int global_correct; 
    int thresh; 
    int count; 
    static const int BHR_LENGTH = 10; //global BHR 
    static const history_t BHR_MSB = (history_t(1) << (BHR_LENGTH - 1));
    static const std::size_t PHT_SIZE = (std::size_t(1) << BHR_LENGTH);
    static const std::size_t PHT_INDEX_MASK = (PHT_SIZE - 1);
    static const counter_t PHT_INIT = /* weakly taken */ 2;
    int total_pred_T_but_NT; 
    history_t bhr;                // 4 bits
    std::vector<counter_t> pht;   // 64K bits
    std::map<address_t, branch_info_t> branch_table; 
    std::map<address_t, std::vector<double> > pc_statistics;
    std::map<address_t, std::vector<double> > pc_x_coords; 
    
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
            address_t pc = br->instruction_addr; 
            count++; 
			
            if (/* conditional branch */ br->is_conditional) 
	    	{
		
                /*
                address_t pc = br->instruction_addr;
                std::size_t index = pht_index(pc, bhr);
                counter_t cnt = pht[index];
                prediction = counter_msb(cnt);
                */
				
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
					branch_info.num_incorrect = 0;  
					branch_info.iteration = 0;  
					branch_info.num_pred_T_but_NT = 0; 
					branch_info.num_elim = 0;
                                        branch_info.delete_all = 0; 
                                        branch_info.num_pred_NT_actually_NT = 0;
                                        branch_info.consec_pred_T_but_NT = 0; 
                                        branch_info.num_pred_NT_but_T = 0; 
                                        branch_info.num_pred_T_actually_T = 0; 
                                        branch_info.num_actual_T = 0; 
                                        branch_info.num_actual_NT = 0; 
                                        branch_info.initialized = false; 
                                        branch_info.weight_vector.resize(num_DNF_terms);
					branch_table[pc] = branch_info; //insert into map 
				}
				else
				{
					//branch exists in map
					int result = 1;  
					//for (int i = 0; i < branch_table[pc].num_terms; i++)
					//{
                                        int i = 0; 
                                        if (branch_table[pc].num_terms == 2)
                                        {
                                                if (branch_table[pc].weight_vector[1] > branch_table[pc].weight_vector[0])
                                                {
                                                    //printf("HERE \n"); 
                                                    i = 1; 
                                                }
                                        }
                                        
                                                //LOOKING AT DATA
                                                if (i == 0)
                                                {
                                                        if (count > thresh)
                                                        {
                                                            thresh += 100000;
                                                            printf("count %d\n",count); 
                                                            printf("predicted nt but t %d\n",global_wrong_nt_but_t); 
                                                            printf("predicted t but nt %d\n", global_wrong_t_but_nt);
                                                            printf("num exception wrong %d\n",global_incorrect);
                                                            printf("non exception wrong %d\n",global_incorrect_nonexcep); 
                                                            printf("total correct %d\n",global_correct); 
                                                            if (count == 15000001)
                                                            {
								for(map<address_t, vector<double> >::const_iterator it = pc_statistics.begin(); it != pc_statistics.end(); ++it)
							        {
									printf("PC = %d\n",it->first); 
									for (double k = 0; k < it->second.size(); k++)
									{
                                               					printf("pc num NT/T = %f ",it->second[k]);
                                               					printf("iter = %f ", pc_x_coords[it->first][k]); 
                                               				}
                                               			        printf("\n"); 
							        }	
                                                            }

                                                        }
                                                }
                                                if (branch_table[pc].exceptions[bhr] == 1 || branch_table[pc].initialized == false)
                                                {
                                                    goto end; 
                                                }
						for (int j = 0; j < BHR_LENGTH; j++)
						{
							if (j == 0)
							{ 
                                                                
								for( std::vector<int>::const_iterator k = branch_table[pc].DNF[i].begin(); k != branch_table[pc].DNF[i].end(); ++k)
								{
    									//std::cout << *k << ' ';
								}
								//printf("\n"); 
							}
							if (branch_table[pc].DNF[i][j] == 1)
							{
								if (((bhr >> (BHR_LENGTH-1-j)) & 1) == 0)  
								{
                                                                        goto end; 
									//break; 
								}
							}
							else if (branch_table[pc].DNF[i][j] == 0)
							{
								if (((bhr >> (BHR_LENGTH-1-j)) & 1) == 1)
 								{
                                                                    goto end; 
								    //break;
								}
 
							}		
							else //-1 
							{
								//do nothing 
							}

							if (j == BHR_LENGTH-1)
							{
								//printf("bhr: %d \n", bhr); 
								if (result == 1)
								{
									prediction = true; 
									goto end; 
								}
							}
						}
					}
				}
				
	   // }
	    end:
	    if (prediction == true && branch_table[pc].num_terms == 2)
            {
                for (int r = 0; r <= 1; r++)
                {
                        
								for( std::vector<int>::const_iterator k = branch_table[pc].DNF[r].begin(); k != branch_table[pc].DNF[r].end(); ++k)
								{
    								//        std::cout << *k << ' ';
								}
                                                                //printf("\n");
                }
                                                                //printf("bhr %d \n", bhr); 
                                                                for (int j = 0; j < BHR_LENGTH; j++)
                                                                {
								  // printf("%d", ((bhr >> (BHR_LENGTH-1-j)) & 1));
                                                                }
                                                               //printf("\n"); 
           }
 
           			
	    	//printf("bhr: %d \n",bhr);  
	    return prediction;   // true for taken, false for not taken
        }

    // Update the predictor after a prediction has been made.  This should accept
        // the branch record (br) and architectural state (os), as well as a third
    // argument (taken) indicating whether or not the branch was taken.
    void update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
        {
            if (br->is_conditional) {
		    
		    address_t pc = br->instruction_addr;
                    std::vector<double> y;
                    std::vector<double> x; 
                    if (taken)
                    {
                        branch_table[pc].num_actual_T++; 
                    }
                    if (taken == false)
                    {
                        branch_table[pc].num_actual_NT++; 
                    }
                    
		    if ( (get_prediction(br,os) == false && taken == true)) //|| (get_prediction(br,os) == true && taken == false) )
		    {
				//branch_table[pc].num_incorrect++;
                                branch_table[pc].num_pred_NT_but_T++; 
		    }
		    branch_info_t branch_info = branch_table[pc]; 
	        
                    branch_table[pc].data[bhr] = get_prediction(br,os); 
                    if (branch_table[pc].initialized == true)
                    {
                        if (get_prediction(br,os) != taken)
                        {
                                if (branch_table[pc].num_terms == 2)
                                {
                                     if (branch_table[pc].weight_vector[0] >= branch_table[pc].weight_vector[1]) //used DNF_prediction
                                     {
                                        branch_table[pc].weight_vector[1]++; 
                                     }
                                     else
                                     {
                                         branch_table[pc].weight_vector[0]++; 
                                     }
                                }
                        }
                        if (get_prediction(br,os) == taken)
                        {
                                if (branch_table[pc].num_terms == 2)
                                {
                                     if (branch_table[pc].weight_vector[0] >= branch_table[pc].weight_vector[1]) //used DNF_prediction
                                     {
                                        branch_table[pc].weight_vector[0]++; 
                                     }
                                     else
                                     {
                                         branch_table[pc].weight_vector[1]++; 
                                     }

                                }
                        }
                   }


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
                if (branch_info.iteration % 100 == 0)
                {
                                               //printf("pc = %d\n",pc); 
                                               //printf("pc num T = %f\n",branch_table[pc].num_actual_T);
                                               //printf("pc num NT = %f\n",branch_table[pc].num_actual_NT);
                                               if (branch_table[pc].num_actual_NT + branch_table[pc].num_actual_T > 5) //normally 25
                                               {
                                                        if (pc_statistics.find(pc) == pc_statistics.end())
                                                        {
                                                                y.push_back(branch_table[pc].num_actual_NT/branch_table[pc].num_actual_T);
                                                                x.push_back(branch_info.iteration); 
                                                                pc_statistics[pc] = y; 
                                                                pc_x_coords[pc] = x; 
                                                        }
                                                        else
                                                        {
                                                                if (branch_table[pc].num_actual_T != 0)
                                                                {
                                                                        pc_statistics[pc].push_back(branch_table[pc].num_actual_NT/branch_table[pc].num_actual_T); 
                                                                        pc_x_coords[pc].push_back(branch_info.iteration);
                                                                }

                                                        }
                                                        branch_table[pc].num_actual_T = 0; 
                                                        branch_table[pc].num_actual_NT = 0;
                                               }
              }

				if (get_prediction(br,os) == false && taken == true)
				{
                                        global_wrong_nt_but_t++;
                                        branch_table[pc].consec_pred_T_but_NT = 0; 
                                        branch_table[pc].theoretical[bhr] = true;
                                        double total = (double)branch_table[pc].num_pred_T_but_NT + (double)branch_table[pc].num_pred_NT_but_T;
                                        double total_2 = (double)branch_info.num_pred_NT_but_T + (double)branch_info.num_pred_T_actually_T;
                                        //printf("total %f\n",total); 
                                        //printf("pred T but NT %d\n",branch_table[pc].ghr_num_pred_T_but_NT[bhr]);
				        double ratio = (double)branch_info.num_pred_T_but_NT/total; 
                                        //printf("ratio %f \n", ratio); 
                                        //printf("total %f \n", total); 
					if (branch_info.iteration == 0)
					{
						//DNF = bhr 
						for(int i = 0; i < BHR_LENGTH; i++)
						{
						    branch_table[pc].DNF[0][i] =  ((bhr >> (BHR_LENGTH-1-i)) & 1);
						}
                                                branch_table[pc].num_terms = 1; 
                                                branch_table[pc].initialized = true; 
						branch_table[pc].iteration++; //update iterations
	
					}
					else if (( (double)branch_info.num_pred_NT_but_T/total_2 > .3) || branch_table[pc].iteration < 5)
					{
						branch_table[pc].iteration++; 
						//printf("incorrect %G \n",((double)branch_info.num_incorrect));  
						//printf ("total %G \n",(double)(branch_table[pc].iteration));
						int num_deleted = 0; 
						int potential_deletes = 0; 
						int delete_index = branch_info.num_terms-1; //delete from most recent disjunction
						for(int i = 0; i < BHR_LENGTH; i++)
						{
							
							if ( ( (((bhr >> (BHR_LENGTH-1-i)) & 1) == 1) && branch_info.DNF[delete_index][i] == 0 ) ||
							( (((bhr >> (BHR_LENGTH-1-i)) & 1) == 0) && branch_info.DNF[delete_index][i] == 1 ) )
							{
								potential_deletes++; 
							}
						}
						//printf("potenti %d\n",potential_deletes);
						//printf("num el %d\n",branch_table[pc].num_elim);  
						for(int i = 0; i < BHR_LENGTH; i++)
						{
							if (potential_deletes + branch_table[pc].num_elim >= (BHR_LENGTH))
							{
                                                                branch_table[pc].delete_all++; 
								break;
							}

							 if ( ( (((bhr >> (BHR_LENGTH-1-i)) & 1) == 1) && branch_info.DNF[delete_index][i] == 0 ) ||
							( (((bhr >> (BHR_LENGTH-1-i)) & 1) == 0) && branch_info.DNF[delete_index][i] == 1 ) )
							{
								branch_table[pc].DNF[delete_index][i] = -1;
								branch_table[pc].num_elim++; 
								num_deleted++; 
								//printf("here \n"); 
							}

							
						} 
						if (num_deleted == 0 && branch_table[pc].num_terms < num_DNF_terms && branch_table[pc].delete_all > 3) //delete all- "confidence"
						{
							//add disjunction
							//printf("adding disj \n"); 
							branch_table[pc].num_terms++; //update num_terms
							int pos = branch_table[pc].num_terms-1; 
							//printf("pos %d\n",pos);   
							for(int i = 0; i < BHR_LENGTH; i++)
							{
							    branch_table[pc].DNF[pos][i] =  ((bhr >> (BHR_LENGTH-1-i)) & 1); 
							}
                                                        branch_table[pc].delete_all = 0; 
                                                        branch_table[pc].num_elim = 0; 
							//printf("DELETING VALUES %d \n",pos+1); 

	
						}
					}
				}
				else if (get_prediction(br,os) == true && taken == false)
				{
                                    branch_table[pc].consec_pred_T_but_NT++; 
                                    global_wrong_t_but_nt++; 
                                    if (branch_table[pc].exceptions[bhr] == 0) {
                                            global_incorrect_nonexcep++;                               
                                    }
                                    else {
                                            global_incorrect++;
                                    }

                                    branch_table[pc].theoretical[bhr] = false; 
			            total_pred_T_but_NT++; 
                                    //printf("total pred %d\n",total_pred_T_but_NT);
                                    branch_table[pc].num_pred_T_but_NT++;
                                    branch_table[pc].ghr_num_pred_T_but_NT[bhr]++; 
                                    //printf("lol %d \n", branch_table[pc].ghr_num_pred_T_but_NT[bhr]); 
                                    double total = (double)branch_table[pc].ghr_num_pred_T_but_NT[bhr] + (double)branch_table[pc].ghr_num_pred_NT_actually_NT[bhr];
                                    if (pc_statistics.find(pc) != pc_statistics.end() && (pc_statistics[pc][pc_statistics[pc].size()-1] > 2.0) 
                                    && total > 10)// || branch_table[pc].consec_pred_T_but_NT > 20) //if greater than some threshold
                                    {
                                        //printf("wrong %d\n",branch_table[pc].ghr_num_pred_T_but_NT[bhr]);
                                        //printf("total %f \n",total);
                                        //printf("pc stat = %f \n", pc_statistics[pc][pc_statistics[pc].size()-1]); 
                                        if (branch_table[pc].exceptions[bhr] == 0 && (global_wrong_nt_but_t/global_wrong_t_but_nt) < 2)
                                        {
                                          //  printf("ya \n"); 
                                            ////branch_table[pc].exceptions[bhr] = 1;
                                          
                                        }

                                    }
                                    else
                                    {
                                        if (branch_table[pc].exceptions[bhr] == 1)
                                        {
                                                printf("here \n");
                                        }
                                        //branch_table[pc].exceptions[bhr] = 0;  
                                    }
				    branch_table[pc].iteration++; 
				}
				else
				{
        			    if (get_prediction(br,os) == false && taken == false)
                                    {
                                        branch_table[pc].num_pred_NT_actually_NT++;
                                        branch_table[pc].ghr_num_pred_NT_actually_NT[bhr]++; 

                                    }
                                    if (get_prediction(br,os) == true && taken == true)
                                    {
                                        branch_table[pc].num_pred_T_actually_T++; 
                                    }

                                    global_correct++; 
                                    branch_table[pc].theoretical[bhr] = taken; 
				    branch_table[pc].iteration++; 
				}

				
				update_bhr(taken); 			
	    }
        }
};

#endif // PREDICTOR_H_SEEN
 
