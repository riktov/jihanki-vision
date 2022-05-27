#include <iostream>
#include <cstdlib> //abs
#include <numeric> //accumulate
#include <vector>
#include "run_length.hpp"

extern bool cmdopt_verbose ;

/*
std::vector<size_t> run_lengths(std::vector<int> vals, int thresh) {
  bool last ;
  std::vector<int> runs ;
  int this_run = 1 ;

  if(vals.size() < 1) { return runs ; }

  last = (vals[0] > thresh) ;

  for(unsigned int i = 1 ; i < vals.size() ; i++) {
    int v = vals[i] ;

    if((v > thresh) == last) {
      this_run++ ;
    } else {
      runs.push_back(this_run) ;
      this_run = 1 ;
    }
    last = (v > thresh) ;
  }
  runs.push_back(this_run) ;

  return runs ;
}
*/

/*
Given a run-length-array, returns an array marking the midpoints of each run, along with the endpoints.

@param rl A vector of size_t representing a run-length array
@param is_starts_high Whether the initial value is high?
*/
std::vector<int> run_length_midpoints (std::vector<size_t> rl, bool is_starts_high) {
  std::vector<int> rlmids ;

  rlmids.push_back(0) ;

  int total = rl[0] ;

  for(size_t i = 1 ; i < rl.size() ; i++) {
    int run = rl[i] ;
    int mid = total + (run / 2) ;
    rlmids.push_back(mid) ;
    total += run ;
  }

  rlmids.push_back(total) ;
  
  return rlmids ;
}


/* returns the mean of the differences of the differences of the runs
   Generally represents the stable periodicity, with a stably periodic
   signal giving 0.
*/
float run_length_second_diff(std::vector<size_t> rl) {
  std::vector<int> first_diffs, second_diffs ;

  int last ;

  const float ERR_RLSD = 255.1234 ;
  if(rl.size() < 4) { return ERR_RLSD ; }//error value is positive, so it sorts last

  last = rl[1] ;

  //ignore the first and last runs, as they are "lead in" and "lead out"
  for(size_t i = 2 ; i < rl.size() - 1 ; i++) {
    int n = rl[i] ;
    int diff = abs(n - last) ;
    //std::cout << diff << " " ;
    first_diffs.push_back(diff) ;
    last = n ;
  }

  last = first_diffs[0] ;
  for(size_t i = 1 ; i < first_diffs.size() ; i++) {
    int n = first_diffs[i] ;
    int diff = abs(n - last) ;
    second_diffs.push_back(diff) ;
    last = n ;
  }

  int total = std::accumulate(second_diffs.begin(), second_diffs.end(), 0) ;
  int sz = second_diffs.size() ;

  if(sz == 0 || total == 0) {
      return ERR_RLSD ;
  }
  
  return (1.0 * total) / sz ;
}

/* Denoise a run-length array by "flipping" short runs
*/
std::vector<size_t> denoise_run_length(std::vector<size_t> rl) {
  if(rl.size() < 1) {
      return rl ;
  }

  size_t min_val = rl[0] ;
  std::vector<size_t> result ;

  //get the minimum
  for(size_t i = 1 ; i < rl.size() - 1 ; i++) {
    if(rl[i] < min_val) { min_val = rl[i] ; }
  }
  
  for(size_t i = 1 ; i < rl.size() - 1 ; i++) {
    size_t combined ;
    
    if(rl[i] <= min_val) {
      combined = rl[i - 1] + rl[i] + rl[i + 1] ;
      result.push_back(combined) ;
      i += 2 ;
    } else {
      result.push_back(rl[i - 1]) ;
    }
  }
  return result ;//bogus
}

std::vector<bool> run_length_to_values(std::vector<int> rl, bool initial_val) {
  return {} ;
}
