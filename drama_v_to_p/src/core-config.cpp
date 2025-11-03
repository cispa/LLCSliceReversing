#include "core-config.hpp"

// Constructor with parameters and default values
CoreConfig::CoreConfig(int number_of_p_cores, int number_of_e_cores,
                       int e_core_offset)
    : number_of_p_cores(number_of_p_cores),
      number_of_e_cores(number_of_e_cores), e_core_offset(e_core_offset) {
  for (int i = 0; i < number_of_p_cores; i++) {
    slice_cores.push_back(i);
  }
  int start_of_e_cores = (number_of_p_cores - 1) * 2;
  for (int i = number_of_p_cores; i < number_of_e_cores; i++) {
    slice_cores.push_back(start_of_e_cores + 4 * i);
    offset_cores.push_back(start_of_e_cores + 4 * i);
  }
  this->num_slices = number_of_p_cores + number_of_e_cores;
}

// Get the core for a given slice
int CoreConfig::get_core_for_slice(int slice) { return slice_cores[slice]; }

int CoreConfig::is_offset_slice(int slice) {
  return slice >= this->number_of_p_cores;
}
