#ifndef _CORE_CONFIG_H_
#define _CORE_CONFIG_H_

#include <vector>

class CoreConfig
{
private:
    int number_of_p_cores;
    int number_of_e_cores;
    int e_core_offset;


public:
    CoreConfig(
        int number_of_p_cores = 0,
        int number_of_e_cores = 0,
        int e_core_offset = 0);
    
    std::vector<int> slice_cores;
    std::vector<int> offset_cores;
    int num_slices;

    int get_core_for_slice(int slice);
    int is_offset_slice(int slice);
};

#endif // _CORE_CONFIG_H_