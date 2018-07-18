#ifndef ZEE_H
#define ZEE_H
#include "arrayfire.h"
#include "LLGTerm.hpp"
#include "../state.hpp"
#include "../func.hpp"

class Zee : public LLGTerm {
  public:
    //static Zeeman field
    Zee(af::array zee_in);
    Zee(long int zee_in_addr);

    //time dependent Zeeman field
    Zee(af::array (*callback_func_in)(State state));
    af::array (*callback_func)(State state);
    bool callback{false};

    //Field contribution
    af::array h(const State& state);
    //Energy contribution
    double E(const State& state);
    //CPU time
    double get_cpu_time(){return cpu_time;}

    double rate;//[T/s]
    double hzee_max;//[T]
    af::array zee_field;
    double cpu_time{0.};
    af::timer timer;
};


#endif

//for wrapping: 
//https://stackoverflow.com/questions/8800838/how-to-pass-a-function-pointer-to-an-external-program-in-cython