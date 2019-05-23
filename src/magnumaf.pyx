#!python
#distutils: language = c++
#cython: language_level=3

# Links
# [1] https://stackoverflow.com/questions/33764094/cython-how-do-i-wrap-a-c-class-where-public-member-variables-are-custom-objec
#HOTTIPS
#https://stackoverflow.com/questions/44396749/handling-c-arrays-in-cython-with-numpy-and-pytorch
#https://stackoverflow.com/questions/44686590/initializing-cython-objects-with-existing-c-objects
#https://stackoverflow.com/questions/47044866/how-to-convert-python-object-to-c-type-in-cython
#clib library_with_useful_functions

## numpy to arrayfire
#af.interop.np_to_af_array
## arrayfire to numpy
#af.Array.__array__()

import arrayfire as af

from ctypes import addressof, c_void_p
from libcpp.memory cimport shared_ptr
from libcpp.vector cimport vector
from libcpp cimport bool
from cython.operator cimport dereference as deref
from math import sqrt
from math import pi
from numpy import zeros as np_zeros

from magnumaf_decl cimport Mesh as cMesh
from magnumaf_decl cimport Material as cParam
from magnumaf_decl cimport State as cState
from magnumaf_decl cimport Controller as cController
from magnumaf_decl cimport LLGIntegrator as cLLGIntegrator
from magnumaf_decl cimport DemagField as cDemagField
from magnumaf_decl cimport UniaxialAnisotropyField as cUniaxialAnisotropyField
from magnumaf_decl cimport ExchangeField as cExchangeField
from magnumaf_decl cimport SparseExchangeField as cSparseExchangeField
from magnumaf_decl cimport SpinTransferTorqueField as cSpinTransferTorqueField
#TODO#from magnum_af_decl cimport DmiField as cDMI

from magnumaf_decl cimport AtomisticDipoleDipoleField as cAtomisticDipoleDipoleField
from magnumaf_decl cimport AtomisticExchangeField as cAtomisticExchangeField
from magnumaf_decl cimport AtomisticUniaxialAnisotropyField as cAtomisticUniaxialAnisotropyField
from magnumaf_decl cimport AtomisticDmiField as cAtomisticDmiField
from magnumaf_decl cimport ExternalField as cExternalField
from magnumaf_decl cimport LBFGS_Minimizer as cLBFGS_Minimizer
from magnumaf_decl cimport LLGTerm as cLLGTerm

def array_from_addr(array_addr):
    array=af.Array()
    array.arr=c_void_p(array_addr)
    return array

class Util:
    @staticmethod
    def normed_homogeneous_field(nx = 1, ny = 1, nz = 1, axis=[1,0,0], factor = 1.):
        """Returns a homogeneous field of dimension [nx, ny, nz, 3] pointing into the direction of axis and normed to factor."""
        norm = sqrt(axis[0]**2+axis[1]**2+axis[2]**2)
        array = af.constant(0.0, 1, 1, 1, 3, dtype=af.Dtype.f64)
        array [0,0,0,0] = factor * axis[0]/norm
        array [0,0,0,1] = factor * axis[1]/norm
        array [0,0,0,2] = factor * axis[2]/norm
        return af.tile(array, nx, ny, nz)
 
    @staticmethod
    def disk(nx, ny, nz, axis=[1,0,0]):
        norm = sqrt(axis[0]**2+axis[1]**2+axis[2]**2)
        n_cells=0
        m = np_zeros((nx, ny, nz, 3));
        for ix in range (0, nx):
            for iy in range(0, ny):
                for iz in range(0, nz):
                    a= nx/2
                    b= ny/2
                    rx=ix-nx/2.
                    ry=iy-ny/2.
                    r = pow(rx,2)/pow(a,2)+pow(ry,2)/pow(b,2);
                    if(r<=1):
                            m[ix,iy,iz,0]=axis[0]/norm
                            m[ix,iy,iz,1]=axis[1]/norm
                            m[ix,iy,iz,2]=axis[2]/norm
                            n_cells = n_cells +1
        return af.from_ndarray(m), n_cells

    @staticmethod
    def vortex(mesh, positive_z = True):
        """Returns a vortex configuration of dimension [mesh.nx, mesh.ny, mesh.nz, 3]. The option positive_z decides whether the vortex core points in positive (=True) or negative (=False) z-direction."""
        m = np_zeros((mesh.nx, mesh.ny, mesh.nz, 3));
        for ix in range (0, mesh.nx):
            for iy in range(0, mesh.ny):
                rx=float(ix)-mesh.nx/2.
                ry=float(iy)-mesh.ny/2.
                r = sqrt(pow(rx,2)+pow(ry,2))

                if r < mesh.nx/2.:
                    for iz in range(0, mesh.nz):
                        if r==0.:
                            if positive_z==True:
                                m[ix,iy,:,2]= 1.
                            else:
                                m[ix,iy,:,2]= -1
                        else:
                            m[ix,iy,:,0]=-ry/r
                            m[ix,iy,:,1]= rx/r
                            if positive_z==True:
                                m[ix,iy,:,2]= sqrt(mesh.nx)/r
                            else:
                                m[ix,iy,:,2]= - sqrt(mesh.nx)/r
                        norm = sqrt(m[ix, iy, iz,0]**2+m[ix, iy, iz,1]**2+m[ix, iy, iz,2]**2)
                        m[ix, iy, iz,:]=m[ix, iy, iz,:]/norm
        return af.from_ndarray(m)

    @classmethod
    def sum_of_difference_of_abs(cls, a, b):
        return af.sum(af.sum(af.sum(af.sum(af.abs(a)-af.abs(b),0),1),2),3).scalar()
 
    @classmethod
    def test_sum_of_difference_of_abs(cls, a, b, verbose = True):
            c = cls.sum_of_difference_of_abs(a, b)
            if (c != 0.):
                    if (verbose == True):
                            print ("Error")
                    return False
            else:
                    if (verbose == True):
                            print ("Success")
                    return True


# From http://code.activestate.com/recipes/440514-dictproperty-properties-for-dictionary-attributes/
class dictproperty(object):

    class _proxy(object):

        def __init__(self, obj, fget, fset, fdel):
            self._obj = obj
            self._fget = fget
            self._fset = fset
            self._fdel = fdel

        def __getitem__(self, key):
            if self._fget is None:
                raise TypeError, "can't read item"
            return self._fget(self._obj, key)

        def __setitem__(self, key, value):
            if self._fset is None:
                raise TypeError, "can't set item"
            self._fset(self._obj, key, value)

        def __delitem__(self, key):
            if self._fdel is None:
                raise TypeError, "can't delete item"
            self._fdel(self._obj, key)

    def __init__(self, fget=None, fset=None, fdel=None, doc=None):
        self._fget = fget
        self._fset = fset
        self._fdel = fdel
        self.__doc__ = doc

    def __get__(self, obj, objtype=None):
        if obj is None:
            return self
        return self._proxy(obj, self._fget, self._fset, self._fdel)


#NOTE#@cython.embedsignature(True)# error: Cdef functions/classes cannot take arbitrary decorators. https://stackoverflow.com/questions/42668252/cython-cdef-class-not-displaying-doc-string-or-init-parameters
# Docstring does work, todo: check type etc. 
cdef class Mesh:
    """
    The Mesh object stores information about the discretization of our sample.
    

    Parameters
    ----------
    nx : int
        Number of cells in the x-direction
    ny : int
        Number of cells in the y-direction
    nz : int
        Number of cells in the z-direction
    dx : float
        Distance of one cell in the x-direction
    dy : float
        Distance of one cell in the y-direction
    dz : float
        Distance of one cell in the z-direction


    Attributes
    ----------
    All parameters are stored in equally named attributes


    Examples
    ----------
    mesh = Mesh(100, 25, 1, 1e-9, 1e-9, 1e-9)
    mesh = Mesh(nx = 100, ny = 25, nz = 1, dx = 1e-9, dy = 1e-9, dz = 1e-9)
    print(mesh.nx)
    """
    def __init__(self, nx, ny, nz, dx, dy, dz): # todo: For dockstring, but currently does not change signature
        pass

    cdef cMesh* thisptr
    cdef object owner # None if this is our own # From [1]
    def __cinit__(self,int nx, int ny, int nz, double dx, double dy, double dz):
        self.thisptr = new cMesh(nx, ny, nz, dx, dy, dz)
        owner = None # see [1]
    cdef set_ptr(self, cMesh* ptr, owner):
        if self.owner is None:
            del self.thisptr
        self.thisptr = ptr
        self.owner = owner
    def __dealloc__(self):
        if self.owner is None: # only free if we own it: see [1]
            del self.thisptr
            self.thisptr = NULL
    def print_nx(self):
        print (self.thisptr.n0)

    @property
    def nx(self):
        return self.thisptr.n0
    @nx.setter
    def nx(self,value):
        self.thisptr.n0=value

    @property
    def ny(self):
        return self.thisptr.n1
    @ny.setter
    def ny(self,value):
        self.thisptr.n1=value

    @property
    def nz(self):
        return self.thisptr.n2
    @nz.setter
    def nz(self,value):
        self.thisptr.n2=value

    @property
    def dx(self):
        return self.thisptr.dx
    @dx.setter
    def dx(self,value):
        self.thisptr.dx=value

    @property
    def dy(self):
        return self.thisptr.dy
    @dy.setter
    def dy(self,value):
        self.thisptr.dy=value

    @property
    def dz(self):
        return self.thisptr.dz
    @dz.setter
    def dz(self,value):
        self.thisptr.dz=value



cdef class State:
    """
    The State object represents one realization of a magnetization configuration on a defined mesh.

    Parameters
    ----------
    mesh : Mesh
        The discretization is passed by a Mesh object
    Ms : float or af.array
            Saturation magnetization either set globally (float) or at each node (af.array)
    m : af.array
        Magnetization configuration which defines a vector at each node.
        The expected size is [mesh.nx, mesh.ny, mesh.nz, 3] and dtype=af.Dtype.f64
    verbose : bool(True)
        Print info messages, defaults to true
    mute_warning : bool(False)
        Mutes warnings, defaults to false

    Attributes
    ----------
    mesh : Mesh
    Ms : float
        Only set when input parameter Ms is float
    Ms_field : af.array
        Only set when input parameter Ms is af.array
    m : af.array [mesh.nx, mesh.ny, mesh.nz, 3, dtype=af.Dtype.f64]
    t : float
        Physical time in [s] manipulated by the LLGIntegrator object and initialized to 0

    Methods
    -------
    write_vti(outputname) : str
        Writes the current magnetization m into the file 'outputname.vti'
    normalize()
        Normalizes m to 1 for every node
    m_mean(i = None) : int
        Calculates the average magnetization along all three dimensions (i = None) or along dimension i = {0, 1, 2}

    Examples
    ----------
    mesh = Mesh(1, 1, 1, 0.1, 0.1, 0.1)
    m0 = af.constant(0.0, nx, ny, nz, 3, dtype=af.Dtype.f64)
    m0[:,:,:,0] = 1
    state = State(mesh, 8e5, m0)
    """
    cdef cState* thisptr
    def __cinit__(self, Mesh mesh, Ms, m, verbose = True, mute_warning = False, Material material = None, evaluate_mean = None):
        # switch for evaluate_mean value
        if hasattr(Ms, 'arr'):
            self.thisptr = new cState (deref(mesh.thisptr), <long int> addressof(Ms.arr), <long int> addressof(m.arr), <bool> verbose, <bool> mute_warning)
        # legacy
        elif (material is not None and evaluate_mean is None):
            print("Warning: State constructor: legacy code, will be dopped soon")
            self.thisptr = new cState (deref(mesh.thisptr), deref(material.thisptr), addressof(m.arr))
            self.thisptr.Ms = Ms
        elif (material is not None):
            print("Warning: State constructor: legacy code, will be dopped soon")
            self.thisptr = new cState (deref(mesh.thisptr), deref(material.thisptr), addressof(m.arr), addressof(evaluate_mean.arr))
            self.thisptr.Ms = Ms
        # end legacy
        else:
            self.thisptr = new cState (deref(mesh.thisptr), <double> Ms, <long int> addressof(m.arr), <bool> verbose, <bool> mute_warning)
        #af.device.lock_array(m_in)#This does not avoid memory corruption caused by double free
    def __dealloc__(self): # causes segfault on every cleanup
        del self.thisptr
        self.thisptr = NULL
    @property
    def t(self):
        return self.thisptr.t
    @t.setter
    def t(self, value):
        self.thisptr.t = value
    def pythisptr(self):
            return <size_t><void*>self.thisptr
    @property
    def Ms(self):
        return self.thisptr.Ms
    #@Ms.setter
    #def Ms(self,value):
    #  self.thisptr.Ms=value


    def write_vti(self, outputname):
        self.thisptr._vti_writer_micro( outputname.encode('utf-8')) 
    def write_vti_boolean(self, outputname):
        self.thisptr._vti_writer_micro_boolean( outputname.encode('utf-8')) 
    def write_vti_atomistic(self, outputname):
        self.thisptr._vti_writer_atom( outputname.encode('utf-8')) 
    def read_vti(self, outputname):
        self.thisptr._vti_reader( outputname.encode('utf-8')) 

    #def py_vtr_writer(self, outputname):
    #  self.thisptr._vtr_writer( outputname.encode('utf-8'))
    #def py_vtr_reader(self, outputname):
    #  self.thisptr._vtr_reader( outputname.encode('utf-8'))
    def normalize(self):
        self.thisptr.Normalize()

    property mesh:
        def __get__(self):
            mesh = Mesh(0,0,0,0,0,0)
            mesh.set_ptr(&self.thisptr.mesh,self)
            return mesh
        def __set__(self, Mesh mesh):
            self.thisptr.mesh = deref(mesh.thisptr)

    property material:
        def __get__(self):
            material = Material()
            material.set_ptr(&self.thisptr.material,self)
            return material
        def __set__(self, Material material_in):
            self.thisptr.material = deref(material_in.thisptr)

    @property
    def m(self):
        return array_from_addr(self.thisptr.get_m_addr())
    @m.setter
    def m(self, m_in):
        self.thisptr.set_m(addressof(m_in.arr))

    def get_m_partial(self, key):
        return array_from_addr(self.thisptr.get_m_addr())[key]

    def set_m_partial(self, key, value):
        if value.dims() == self.m[key].dims():
            temp = self.m
            temp[key] = value
            self.thisptr.set_m(addressof(temp.arr))
        else:
                print("Error: State.m_partial: Dimensions do not match. m_partial[key].dims()=",self.m[key].dims()," != rhs.dims()=",value.dims(),". Setting m_partial is ignored.")
    # setting dictionary as property
    m_partial = dictproperty(get_m_partial, set_m_partial, None)

    # Method for setting parts of m[key] to given af.array
    # Should be overloading m.setter, not completed
    #def __setitem__(self, key, m_in):
    #  temp = self.m
    #  temp[key] = m_in
    #  self.thisptr.set_m(addressof(temp.arr))
    #  print("min", self.m[key].dims(),"min", m_in.dims(), "m", self.m.dims(), "temp", temp.dims())
    #  #self.thisptr.set_m(addressof(m_in.arr))

    @property
    def Ms_field(self):
        return array_from_addr(self.thisptr.get_Ms_field())
    @Ms_field.setter
    def Ms_field(self, Ms_field):
        self.thisptr.set_Ms_field(addressof(Ms_field.arr))

    @property
    def steps(self):
        return self.thisptr.steps
    def m_mean(self, i = None):
        """
        Method calculating the average magnetization along all (i = None) or along a specific dimension ( i = {0, 1, 2})
        Parameter
        --------
        i : int (None)
        Returns
        ------
        <mx>, <my>, <mz>
            When i is omitted
        <mi>
            When i is either 0 = mx, 1 = my or 2 = mz
        """
        if(i == None):
            return self.thisptr.meani(0), self.thisptr.meani(1), self.thisptr.meani(2)
        else:
            return self.thisptr.meani(i)


cdef class LLGIntegrator:
    cdef cLLGIntegrator* thisptr
    def __cinit__(self, alpha, terms=[], mode="RKF45", hmin = 1e-15, hmax = 3.5e-10, atol = 1e-6, rtol = 1e-6):
        cdef vector[shared_ptr[cLLGTerm]] vector_in
        if not terms:
            print("LLGIntegrator: no terms provided, please add some either by providing a list LLGIntegrator(terms=[...]) or calling add_terms(*args) after declaration.")
        else:
            for arg in terms:
                vector_in.push_back(shared_ptr[cLLGTerm] (<cLLGTerm*><size_t>arg.pythisptr()))
            self.thisptr = new cLLGIntegrator (alpha, vector_in, mode.encode('utf-8'), cController(hmin, hmax, atol, rtol))
    # TODO leads to segfault on cleanup, compiler warning eleminated by adding virtual destructor in adaptive_rk.hpp
    # NOTE not happening in minimizer class as it is not derived (i guess)
    #def __dealloc__(self):
    #  del self.thisptr
    #  self.thisptr = NULL
    def step(self, State state_in):
        self.thisptr.step(deref(state_in.thisptr))
    def get_E(self,State state_in):
        return self.thisptr.E(deref(state_in.thisptr))
    #def print_stepsize(self):
    #  return self.thisptr.h_stepped_
    def get_fheff(self, State state):
        return array_from_addr(self.thisptr.get_fheff_addr(deref(state.thisptr)))
    #def cpu_time(self):
    #  return self.thisptr.cpu_time()
    #def set_state0_alpha(self,value):
    #  self.thisptr.state0.material.alpha=value
    def add_terms(self,*args):
        for arg in args:
            self.thisptr.llgterms.push_back(shared_ptr[cLLGTerm] (<cLLGTerm*><size_t>arg.pythisptr()))
    def relax(self, State state, precision = 1e-10, ncalcE = 100, nprint = 1000):
            self.thisptr.relax(deref(state.thisptr), precision, ncalcE, nprint)
    @property
    def alpha(self):
        return self.thisptr.alpha
    @alpha.setter
    def alpha(self,value):
        self.thisptr.alpha=value

        #cdef vector[shared_ptr[cLLGTerm]] vector_in
        #for term in terms:
        #  vector_in.push_back(shared_ptr[cLLGTerm] (<cLLGTerm*><size_t>terms.pythisptr()))
        #self.thisptr = new cLLGIntegrator (vector_in)  
        
    

cdef class DemagField:
    cdef cDemagField* thisptr
    def __cinit__(self, Mesh mesh, verbose = False, caching = False, nthreads = 4):
        self.thisptr = new cDemagField (deref(mesh.thisptr), verbose, caching, nthreads)    
    #This would causes double free coruption!
    def __dealloc__(self):
        del self.thisptr
        self.thisptr = NULL
    def print_Nfft(self):
        self.thisptr.print_Nfft()
    def E(self,State state_in):
        return self.thisptr.E(deref(state_in.thisptr))
    def cpu_time(self):
        return self.thisptr.get_cpu_time()
    def pythisptr(self):
            return <size_t><void*>self.thisptr

cdef class ExchangeField:
    cdef cExchangeField* thisptr
    def __cinit__(self, A):
        if hasattr(A, 'arr'):
            self.thisptr = new cExchangeField (<long int> addressof(A.arr))
        else:
            self.thisptr = new cExchangeField (<double> A)
    def __dealloc__(self):
        del self.thisptr
        self.thisptr = NULL
    def E(self,State state_in):
        return self.thisptr.E(deref(state_in.thisptr))
    def cpu_time(self):
        return self.thisptr.get_cpu_time()
    def pythisptr(self):
            return <size_t><void*>self.thisptr
    ## Add when needed:
    # @property
    # def A(self):
    #       return self.thisptr.A
    # @A.setter
    # def A(self,value):
    #       self.thisptr.A=value
    # @property
    # def micro_A_field(self):
    #       return array_from_addr(self.thisptr.get_micro_A_field())
    # @micro_A_field.setter
    # def micro_A_field(self, micro_A_field_in):
    #       self.thisptr.set_micro_A_field(addressof(micro_A_field_in.arr))



cdef class SparseExchangeField:
    cdef cSparseExchangeField* thisptr
    def __cinit__(self, A, Mesh mesh, verbose = True):
        if hasattr('A','arr'):
            self.thisptr = new cSparseExchangeField (<long int> addressof(A.arr), deref(mesh.thisptr), <bool> verbose)
        else:
            self.thisptr = new cSparseExchangeField (<double> A, deref(mesh.thisptr), <bool> verbose)
            # Note: use <bool_t> instead of <bool> in case of ambiguous overloading error: https://stackoverflow.com/questions/29171087/cython-overloading-no-suitable-method-found
    def __dealloc__(self):
        del self.thisptr
        self.thisptr = NULL
    def E(self,State state_in):
        return self.thisptr.E(deref(state_in.thisptr))
    def cpu_time(self):
        return self.thisptr.get_cpu_time()
    def pythisptr(self):
            return <size_t><void*>self.thisptr

cdef class UniaxialAnisotropyField:
    cdef cUniaxialAnisotropyField* thisptr
    def __cinit__(self, Ku1, Ku1_axis = [0, 0, 1]):
        if hasattr(Ku1, 'arr'):
            self.thisptr = new cUniaxialAnisotropyField (<long int> addressof(Ku1.arr), <double> Ku1_axis[0], <double> Ku1_axis[1], <double> Ku1_axis[2])  
        else:
            self.thisptr = new cUniaxialAnisotropyField (<double> Ku1, <double> Ku1_axis[0], <double> Ku1_axis[1], <double> Ku1_axis[2])    
    def __dealloc__(self):
        del self.thisptr
        self.thisptr = NULL
    def h(self, State state):
        return array_from_addr(self.thisptr.h_ptr(deref(state.thisptr)))
    def E(self,State state_in):
        return self.thisptr.E(deref(state_in.thisptr))
    def cpu_time(self):
        return self.thisptr.get_cpu_time()
    def pythisptr(self):
            return <size_t><void*>self.thisptr
    @property
    def Ku1(self):
        return self.thisptr.Ku1
    #@Ku1.setter
    #def Ku1(self,value):
    #  self.thisptr.Ku1=value

    @property
    def Ku1_axis(self):
        return self.thisptr.get_ku1_axis(0), self.thisptr.get_ku1_axis(1), self.thisptr.get_ku1_axis(2)
    #@Ku1_axis.setter
    #def Ku1_axis(self, values):
    #  self.thisptr.Ku1_axis[0] = values[0]
    #  self.thisptr.Ku1_axis[1] = values[1]
    #  self.thisptr.Ku1_axis[2] = values[2]
    @property
    def Ku1_field(self):
        return array_from_addr(self.thisptr.get_Ku1_field())
    #@micro_Ku1_field.setter
    #def micro_Ku1_field(self, micro_Ku1_field_in):
    #  self.thisptr.set_micro_Ku1_field(addressof(micro_Ku1_field_in.arr))




cdef class AtomisticDipoleDipoleField:
    cdef cAtomisticDipoleDipoleField* thisptr
    def __cinit__(self, Mesh mesh):
        self.thisptr = new cAtomisticDipoleDipoleField (deref(mesh.thisptr))    
    def __dealloc__(self):
        del self.thisptr
        self.thisptr = NULL
    def E(self,State state_in):
        return self.thisptr.E(deref(state_in.thisptr))
    def cpu_time(self):
        return self.thisptr.get_cpu_time()
    def pythisptr(self):
            return <size_t><void*>self.thisptr

cdef class AtomisticUniaxialAnisotropyField:
    cdef cAtomisticUniaxialAnisotropyField* thisptr
    def __cinit__(self, Mesh mesh, Material param_in):
        self.thisptr = new cAtomisticUniaxialAnisotropyField (deref(mesh.thisptr),deref(param_in.thisptr))  
    def __dealloc__(self):
        del self.thisptr
        self.thisptr = NULL
    def E(self,State state_in):
        return self.thisptr.E(deref(state_in.thisptr))
    def cpu_time(self):
        return self.thisptr.get_cpu_time()
    def pythisptr(self):
            return <size_t><void*>self.thisptr

cdef class AtomisticExchangeField:
    cdef cAtomisticExchangeField* thisptr
    def __cinit__(self, Mesh mesh):
        self.thisptr = new cAtomisticExchangeField (deref(mesh.thisptr))    
    def __dealloc__(self):
        del self.thisptr
        self.thisptr = NULL
    def E(self,State state_in):
        return self.thisptr.E(deref(state_in.thisptr))
    def cpu_time(self):
        return self.thisptr.get_cpu_time()
    def pythisptr(self):
            return <size_t><void*>self.thisptr

cdef class AtomisticDmiField:
    cdef cAtomisticDmiField* thisptr
    def __cinit__(self, Mesh mesh, Material param_in):
        self.thisptr = new cAtomisticDmiField (deref(mesh.thisptr),deref(param_in.thisptr))  
    def __dealloc__(self):
        del self.thisptr
        self.thisptr = NULL
    def E(self,State state_in):
        return self.thisptr.E(deref(state_in.thisptr))
    def cpu_time(self):
        return self.thisptr.get_cpu_time()
    def pythisptr(self):
            return <size_t><void*>self.thisptr

cdef class ExternalField:
    cdef cExternalField* thisptr
    def __cinit__(self, array_in):
        self.thisptr = new cExternalField (addressof(array_in.arr))
    def __dealloc__(self):
        del self.thisptr
        self.thisptr = NULL
    def E(self,State state_in):
        return self.thisptr.E(deref(state_in.thisptr))
    def cpu_time(self):
        return self.thisptr.get_cpu_time()
    def set_homogenuous_field(self, x, y, z):
            self.thisptr.set_homogenuous_field(x, y, z)
    def pythisptr(self):
            return <size_t><void*>self.thisptr
    def h(self, State state):
        return array_from_addr(self.thisptr.h_ptr(deref(state.thisptr)))

cdef class LBFGS_Minimizer:
    cdef cLBFGS_Minimizer* thisptr
    def __cinit__(self, terms=[], tol = 1e-6, maxiter = 230):
        cdef vector[shared_ptr[cLLGTerm]] vector_in
        if not terms:
            print("cLBFGS_Minimizer: no terms provided, please add some either by providing a list terms=[...] or calling add_terms(*args)")
        else:
            for arg in terms:
                #print("Adding term", arg)
                vector_in.push_back(shared_ptr[cLLGTerm] (<cLLGTerm*><size_t>arg.pythisptr()))
            self.thisptr = new cLBFGS_Minimizer (vector_in, tol, maxiter, 0) # TODO: WARNING: std::cout is not handled and leads to segfault!!! (setting verbose to 0 is a temporary fix) 
#TODO#  def __dealloc__(self): #causes segfault on GTO in cleanup
#TODO#      del self.thisptr
#TODO#      self.thisptr = NULL
    def add_terms(self,*args):
        for arg in args:
            self.thisptr.llgterms_.push_back(shared_ptr[cLLGTerm] (<cLLGTerm*><size_t>arg.pythisptr()))
    def delete_last_term(self):
        self.thisptr.llgterms_.pop_back()
    #not working as set_homogenuous_field is not pure virutal:# def set_zee_xyz(self, State state, i, x, y, z):
    #not working as set_homogenuous_field is not pure virutal:#         self.thisptr.llgterms_[i].set_homogenuous_field(deref(state.thisptr), x, y, z)
    #def __cinit__(self, tol = 1e-6, maxiter = 230, verbose = 4):
    #  self.thisptr = new cLBFGS_Minimizer(tol, maxiter, verbose) # TODO handle default values 
    def minimize(self, State state_in):
        return self.thisptr.Minimize(deref(state_in.thisptr))
    def pyGetTimeCalcHeff(self):
        return self.thisptr.GetTimeCalcHeff()

cdef class Material:
    cdef cParam* thisptr
    cdef object owner # None if this is our own # From [1]
    def __cinit__(self, D = 0., D_axis = [0.,0.,-1], p = 0., J_atom = 0., D_atom = 0., K_atom = 0., D_atom_axis = [0.,0.,1.], Ku1_atom_axis = [0.,0.,1.], bool hexagonal_close_packed = False):
        # now on cpp side# Ku1_axis_renormed = [x/(sqrt(Ku1_axis[0]**2 + Ku1_axis[1]**2 + Ku1_axis[2]**2)) for x in Ku1_axis]
        Ku1_atom_axis_renormed = [x/(sqrt(Ku1_atom_axis[0]**2 + Ku1_atom_axis[1]**2 + Ku1_atom_axis[2]**2)) for x in Ku1_atom_axis]
        D_axis_renormed = [x/(sqrt(D_axis[0]**2 + D_axis[1]**2 + D_axis[2]**2)) for x in D_axis]
        D_atom_axis_renormed = [x/(sqrt(D_atom_axis[0]**2 + D_atom_axis[1]**2 + D_atom_axis[2]**2)) for x in D_atom_axis]
        self.thisptr = new cParam (D, D_axis_renormed[0], D_axis_renormed[1], D_axis_renormed[2], p, J_atom, D_atom, K_atom, D_atom_axis_renormed[0] , D_atom_axis_renormed[1], D_atom_axis_renormed[2], Ku1_atom_axis_renormed[0], Ku1_atom_axis_renormed[1], Ku1_atom_axis_renormed[2], hexagonal_close_packed)
        owner = None # see [1]
    cdef set_ptr(self, cParam* ptr, owner):
        if self.owner is None:
            del self.thisptr
        self.thisptr = ptr
        self.owner = owner
    def __dealloc__(self):
        if self.owner is None: # only free if we own it: see [1]
            del self.thisptr
            self.thisptr = NULL

    #inlcude in stochllg# @property
    #inlcude in stochllg# def T(self):
    #inlcude in stochllg#       return self.thisptr.T
    #inlcude in stochllg# @T.setter
    #inlcude in stochllg# def T(self,value):
    #inlcude in stochllg#       self.thisptr.T=value

    # Micromagnetic
    @property
    def D(self):
        return self.thisptr.D
    @D.setter
    def D(self,value):
        self.thisptr.D=value

    @property
    def D_axis(self):
        return self.thisptr.D_axis[0], self.thisptr.D_axis[1], self.thisptr.D_axis[2]
    @D_axis.setter
    def D_axis(self, values):
        self.thisptr.D_axis[0] = values[0]
        self.thisptr.D_axis[1] = values[1]
        self.thisptr.D_axis[2] = values[2]

    # Atomistic
    @property
    def p(self):
        return self.thisptr.p
    @p.setter
    def p(self,value):
        self.thisptr.p=value

    @property
    def J_atom(self):
        return self.thisptr.J_atom
    @J_atom.setter
    def J_atom(self,value):
        self.thisptr.J_atom=value

    @property
    def D_atom(self):
        return self.thisptr.D_atom
    @D_atom.setter
    def D_atom(self,value):
        self.thisptr.D_atom=value

    @property
    def Ku1_atom(self):
        return self.thisptr.K_atom
    @Ku1_atom.setter
    def Ku1_atom(self,value):
        self.thisptr.K_atom=value

    @property
    def D_atom_axis(self):
        return self.thisptr.D_atom_axis[0], self.thisptr.D_atom_axis[1], self.thisptr.D_atom_axis[2]
    @D_atom_axis.setter
    def D_atom_axis(self, values):
        self.thisptr.D_atom_axis[0] = values[0]
        self.thisptr.D_atom_axis[1] = values[1]
        self.thisptr.D_atom_axis[2] = values[2]

    @property
    def Ku1_atom_axis(self):
        return self.thisptr.K_atom_axis[0], self.thisptr.K_atom_axis[1], self.thisptr.K_atom_axis[2]
    @Ku1_atom_axis.setter
    def Ku1_atom_axis(self, values):
        self.thisptr.K_atom_axis[0] = values[0]
        self.thisptr.K_atom_axis[1] = values[1]
        self.thisptr.K_atom_axis[2] = values[2]

class Constants:
    """Common physical constants. Values were obtained from CODATA/NIST.
         Attributes:
         mu0 [H/m] magnetic constant mu_0
         gamma [m A^-1 s^-1] gyromagnetic ratio gamma
         mu_b [J/T] Bohr magneton mu_bohr
         e [C] elementary charge e
         kb [J/K] Boltzmann constant kb
         hbar [J s] reduced Planck constant"""
    mu0 = 4e-7 * pi

    gamma = 1.760859644e11 * mu0

    mu_b = 9.274009994e-24

    e = - 1.6021766208e-19

    kb = 1.38064852e-23

    hbar = 1.0545718e-34

cdef class SpinTransferTorqueField:
    cdef cSpinTransferTorqueField* thisptr
    def __cinit__(self, pol, nu_damp,  nu_field, j_e):
        self.thisptr = new cSpinTransferTorqueField (addressof(pol.arr), nu_damp, nu_field, j_e)
    def __dealloc__(self):
        del self.thisptr
        self.thisptr = NULL
    def E(self,State state):
        return self.thisptr.E(deref(state.thisptr))
    def pythisptr(self):
            return <size_t><void*>self.thisptr

    @property
    def polarization_field(self):
        return array_from_addr(self.thisptr.polarization_field.get_array_addr())
    @polarization_field.setter
    def polarization_field(self, array):
        self.thisptr.polarization_field.set_array(addressof(array.arr))
