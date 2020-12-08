import arrayfire as af
from libcpp.memory cimport unique_ptr
from libcpp.vector cimport vector
from libcpp.string cimport string
from libcpp cimport bool

cdef extern from "field_terms/field_term.hpp" namespace "magnumafcpp":
    cdef cppclass FieldTerm

cdef extern from "<arrayfire.h>" namespace "af":
    cdef cppclass array:
        array()

cdef extern from "field_terms/micro/exchange_field.hpp" namespace "magnumafcpp":
    cdef cppclass ExchangeField:
        ExchangeField (long int A_field_ptr);
        ExchangeField (double A);
        long int h_ptr(const State& state);
        double Energy(const State& state);
        double get_cpu_time();

cdef extern from "field_terms/micro/sparse_exchange_field.hpp" namespace "magnumafcpp":
    cdef cppclass SparseExchangeField:
        SparseExchangeField (long int A_exchange_field_ptr, Mesh mesh, bool verbose);
        SparseExchangeField (double A_exchange, Mesh mesh, bool verbose);
        long int h_ptr(const State& state);
        double Energy(const State& state);
        double get_cpu_time();

cdef extern from "field_terms/nonequi/nonequi_exchange_field.hpp" namespace "magnumafcpp":
    cdef cppclass NonequiExchangeField:
        NonequiExchangeField (NonequiMesh mesh, long int A_exchange_field_ptr, bool verbose);
        NonequiExchangeField (NonequiMesh mesh, double A_exchange, bool verbose);
        long int h_ptr(const State& state);
        double Energy(const State& state);
        double get_cpu_time();

cdef extern from "mesh.hpp" namespace "magnumafcpp":
    cdef cppclass Mesh:
        unsigned int nx,ny,nz;
        double dx,dy,dz;
        unsigned int n0_exp, n1_exp, n2_exp;
        Mesh (unsigned int, unsigned int, unsigned int, double, double, double)

cdef extern from "nonequispaced_mesh.hpp" namespace "magnumafcpp":
    cdef cppclass NonequiMesh:
        unsigned nx, ny, nz;
        double dx, dy;
        vector[double] z_spacing;
        NonequiMesh (unsigned, unsigned, double, double, vector[double] z_spacing);

cdef extern from "state.hpp" namespace "magnumafcpp":
    cdef cppclass State:
        State ();
        State (Mesh mesh_in, double Ms, long int m_in, bool verbose, bool mute_warning);
        State (Mesh mesh_in, long int Ms_field_ptr, long int m_in, bool verbose, bool mute_warning);
        void Normalize();

        array m;
        void set_m(long int aptr);
        long int get_m_addr();

        void set_Ms_field(long int aptr);
        long int wrapping_get_Ms_field();

        double t;
        unsigned long long steps;
        Mesh mesh;
        double Ms;

        void write_vti(string outputname);
        void _vti_writer_atom (string outputname);
        void _vti_reader(string inputname);

        void _vtr_writer(string outputname);
        void _vtr_reader(string inputname);
        double meani(const int i);
        double mean_mx();
        double mean_my();
        double mean_mz();

cdef extern from "field_terms/micro/dmi_field.hpp" namespace "magnumafcpp":
    cdef cppclass DmiField:
        DmiField(double D, double D_axis_x, double D_axis_y, double D_axis_z);
        DmiField(long int D_constants_ptr, double D_axis_x, double D_axis_y, double D_axis_z);
        long int h_ptr(const State& state);
        double Energy(const State& state);
        double get_cpu_time();

cdef extern from "field_terms/atom/atomistic_dmi_field.hpp" namespace "magnumafcpp":
    cdef cppclass AtomisticDmiField:
        AtomisticDmiField (const double D_atom, double D_atom_axis_x, double D_atom_axis_y, double D_atom_axis_z);
        long int h_ptr(const State& state);
        double Energy(const State& state);
        double get_cpu_time();

cdef extern from "integrators/controller.hpp" namespace "magnumafcpp":
    cdef cppclass Controller:
        Controller(double hmin, double hmax, double atol, double rtol);

cdef extern from "integrators/llg_integrator.hpp" namespace "magnumafcpp":
    cdef cppclass LLGIntegrator:
        LLGIntegrator (double alpha, vector[unique_ptr[FieldTerm]] vector_in, string mode, Controller, bool dissipation_term_only);
        vector[unique_ptr[FieldTerm]] llgterms;
        void step(State& state);
        double E(const State& state);
        void relax(State& state, double precision, const unsigned iloop, const unsigned iwritecout, const bool verbose);
        long int h_addr(const State& state);
        double alpha;
        unsigned long long accumulated_steps;

cdef extern from "integrators/stochastic_llg.hpp" namespace "magnumafcpp":
    cdef cppclass Stochastic_LLG:
        Stochastic_LLG(double alpha, double T, double dt, State state, vector[unique_ptr[FieldTerm]] terms, string smode);
        void step(State& state);
        double E(const State& state);

cdef extern from "field_terms/micro/demag_field.hpp" namespace "magnumafcpp":
    cdef cppclass DemagField:
        DemagField (Mesh mesh_in, bool verbose, bool caching, unsigned nthreads);
        long int h_ptr(const State& state);
        double Energy(const State& state);
        void print_Nfft();
        double get_cpu_time();

cdef extern from "field_terms/micro/uniaxial_anisotropy_field.hpp" namespace "magnumafcpp":
    cdef cppclass UniaxialAnisotropyField:
        UniaxialAnisotropyField (long int Ku1_field, double Ku1_axis_0, double Ku1_axis_1, double Ku1_axis_2);
        UniaxialAnisotropyField (double Ku1, double Ku1_axis_0, double Ku1_axis_1, double Ku1_axis_2);
        UniaxialAnisotropyField (long int Ku1_field_ptr, long int Ku1_axis_field_ptr);
        UniaxialAnisotropyField (double Ku1, long int Ku1_axis_field_ptr);
        double Energy(const State& state);
        long int h_ptr(const State& state);
        double Ku1;
        double get_ku1_axis(int i);
        long int get_Ku1_field();
        double get_cpu_time();

#cdef extern from "<array>" namespace "std" nogil:
#  cdef cppclass array_d3 "std::array<double, 3>":
#    array_d3() except+
#    int& operator[](size_t)

cdef extern from "field_terms/micro/cubic_anisotropy_field.hpp" namespace "magnumafcpp":
    cdef cppclass CubicAnisotropyField:
        #CubicAnisotropyField(double Kc1, double Kc2, double Kc3, array_d3 c1, array_d3 c2)
        CubicAnisotropyField(double Kc1, double Kc2, double Kc3, double c1x, double c1y, double c1z, double c2x, double c2y, double c2z)
        CubicAnisotropyField(long int Kc1_array_ptr, long int Kc2_array_ptr, long int Kc3_array_ptr, long int c1_array_ptr, long int c2_array_ptr);
        long int h_ptr(const State& state)
        double Energy(const State& state);
        double get_cpu_time();

cdef extern from "field_terms/nonequi/nonequi_uniaxial_anisotropy_field.hpp" namespace "magnumafcpp":
    cdef cppclass NonequiUniaxialAnisotropyField:
        NonequiUniaxialAnisotropyField (NonequiMesh nemesh, long int Ku1_field, double Ku1_axis_0, double Ku1_axis_1, double Ku1_axis_2);
        NonequiUniaxialAnisotropyField (NonequiMesh nemesh, double Ku1, double Ku1_axis_0, double Ku1_axis_1, double Ku1_axis_2);
        NonequiUniaxialAnisotropyField (NonequiMesh nemesh, long int Ku1_field_ptr, long int Ku1_axis_field_ptr);
        NonequiUniaxialAnisotropyField (NonequiMesh nemesh, double Ku1, long int Ku1_axis_field_ptr);
        double Energy(const State& state);
        long int h_ptr(const State& state);
        double Ku1;
        double get_ku1_axis(int i);
        long int get_Ku1_field();
        double get_cpu_time();

cdef extern from "field_terms/atom/atomistic_dipole_dipole_field.hpp" namespace "magnumafcpp":
    cdef cppclass AtomisticDipoleDipoleField:
        AtomisticDipoleDipoleField(Mesh);
        long int h_ptr(const State& state);
        double Energy(const State& state);
        double get_cpu_time();

cdef extern from "field_terms/atom/atomistic_uniaxial_anisotropy_field.hpp" namespace "magnumafcpp":
    cdef cppclass AtomisticUniaxialAnisotropyField:
        AtomisticUniaxialAnisotropyField(const double K_atom, double K_atom_axis_x, double K_atom_axis_y, double K_atom_axis_z);
        long int h_ptr(const State& state);
        double Energy(const State& state);
        double get_cpu_time();

cdef extern from "field_terms/atom/atomistic_exchange_field.hpp" namespace "magnumafcpp":
    cdef cppclass AtomisticExchangeField:
        AtomisticExchangeField(double J_atom);
        long int h_ptr(const State& state);
        double Energy(const State& state);
        double get_cpu_time();

cdef extern from "field_terms/micro/external_field.hpp" namespace "magnumafcpp":
    cdef cppclass ExternalField:
        ExternalField (long int m_in);
        long int get_m_addr();
        long int h_ptr(const State& state);
        double Energy(const State& state);
        double get_cpu_time();
        void set_homogeneous_field(const double x, const double y, const double z);

cdef extern from "field_terms/atom/atomistic_external_field.hpp" namespace "magnumafcpp":
    cdef cppclass AtomisticExternalField:
        AtomisticExternalField (long int m_in);
        long int get_m_addr();
        long int h_ptr(const State& state);
        double Energy(const State& state);
        double get_cpu_time();
        void set_homogeneous_field(const double x, const double y, const double z);


cdef extern from "solvers/lbfgs_minimizer.hpp" namespace "magnumafcpp":
    cdef cppclass LBFGS_Minimizer:
        LBFGS_Minimizer(double tolerance_ , size_t maxIter_ , int verbose );
        vector[unique_ptr[FieldTerm]] fieldterms;
        LBFGS_Minimizer(vector[unique_ptr[FieldTerm]] vector_in, double tolerance_, size_t maxIter_, int verbose);
        double Minimize(State& state);

cdef extern from "func.hpp" namespace "magnumafcpp":
    cdef cppclass WrappedArray:
        WrappedArray(array);
        WrappedArray(long int array_ptr);
        void set_array(long int array_ptr);
        long int get_array_addr();

cdef extern from "field_terms/micro/spin_transfer_torque_field.hpp" namespace "magnumafcpp":
    cdef cppclass SpinTransferTorqueField:
        SpinTransferTorqueField (long int polarization_field_ptr, double nu_dampinglike, double nu_field, double j_e);
        long int h_ptr(const State& state);
        double Energy(const State& state);
        double get_cpu_time();
        WrappedArray polarization_field;

cdef extern from "field_terms/micro/rkky_exchange_field.hpp" namespace "magnumafcpp":
    cdef cppclass RKKYExchangeField:
        RKKYExchangeField (long int rkky_values, long int exchange_values, Mesh mesh, long int rkky_indices, bool verbose);
        long int h_ptr(const State& state);
        double Energy(const State& state);

cdef extern from "vtk_IO.hpp" namespace "magnumafcpp":
    void pywrap_vti_writer_micro(const long int afarray_ptr, const double dx, const double dy, const double dz, string outputname);

cdef extern from "solvers/string_method.hpp" namespace "magnumafcpp":
    cdef cppclass StringMethod:
        StringMethod(State state, vector[State] inputimages, int n_interp, double dt, LLGIntegrator llg);
        double run(const string filepath, const double string_abort_rel_diff, const double string_abort_abs_diff, const int string_steps, const int every_string_to_vti, const bool verbose);


cdef extern from "<array>" namespace "std" nogil:
  cdef cppclass double_array3 "std::array<double, 3>":
    double_array3() except+
    double& operator[](size_t)

cdef extern from "func.hpp" namespace "magnumafcpp":
    double_array3 spacial_mean_in_region(long int vectorfield, long int region)

