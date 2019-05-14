#ifndef STOCHASTIC_INTEGRATOR_H
#define STOCHASTIC_INTEGRATOR_H
#include "arrayfire.h"
#include <memory>
#include <chrono>
#include "../llg_terms/LLGTerm.hpp"
#include "../state.hpp"
#include "../func.hpp"

class Stochastic_Integrator {
    public:
        Stochastic_Integrator (State, std::vector<std::shared_ptr<LLGTerm> >, const double, std::string );
        std::vector<std::shared_ptr<LLGTerm> > Fieldterms;
        Material material;
        Mesh mesh;
        void step(State& state, const double dt);
        double cpu_time();

        //Getter functions
        unsigned long int get_calls() const { return calls ;};
        unsigned long int get_fdmdt_calls() const { return fdmdt_calls ;};
        unsigned long int get_stochfdmdt_calls() const { return stochfdmdt_calls ;};
        double get_time() const { return time;};

    protected:
        template <class T> T Heun(const T& m, const double dt);
        template <class T> T SemiImplicitHeun(const T& m, const double dt);
        template <class T> T detRK4(const T& m, const double dt);

        unsigned long int calls{0};
        unsigned long int fdmdt_calls{0};
        unsigned long int stochfdmdt_calls{0};
        double time{0.};
    private:
        virtual af::array detfdmdt(const af::array& m)=0;
        virtual af::array stochfdmdt(const af::array& m, const af::array& h_th)=0;

        af::array m_prev;
        af::array h_th_prev;
        af::timer timer_stoch;
       
        int mode; //Integration mode

        af::randomEngine rand_engine;
};

#endif
