/* include needed because we are not including opencv?*/
std::vector<int> run_length_midpoints (std::vector<size_t> rl, bool is_starts_high = true) ;
float run_length_second_diff(std::vector<size_t> rl) ;
std::vector<size_t> denoise_run_length(std::vector<size_t> rl) ;

template<typename T>
std::vector<size_t> run_lengths(std::vector<T> vals, T thresh) {
  bool last ;
  std::vector<size_t> runs ;
  int this_run = 1 ;

  if(vals.size() < 1) { return runs ; }

  last = (vals[0] > thresh) ;

  for(size_t i = 1 ; i < vals.size() ; i++) {
    T v = vals[i] ;

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

/* Generate a run-length array of binary values
*/
template<typename T>
std::vector<size_t> binary_run_lengths(std::vector<T> vals) {
  std::vector<size_t> runs ;
  if(vals.size() < 1) { return runs ; }
  
  auto last = vals[0] ;
  
  size_t this_run = 1 ;
  for(size_t i = 1 ; i < vals.size() ; i++) {
    if(vals[i] == last) {
      this_run++ ;
    } else {
      runs.push_back(this_run) ;
      this_run = 1 ;
    }
    last = vals[i] ;
  }
  runs.push_back(this_run) ;
  return runs ;
}

template<typename T>
void print_vector(std::vector<T> elems) {
  for(auto e : elems) { std::cout << e << "-" ; } ;
  std::cout << std::endl ;
}
