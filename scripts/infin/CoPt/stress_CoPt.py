# Paul Heistracher <paul.thomas.heistracher@univie.ac.at>
import sys
import os
import arrayfire as af
import numpy as np
from magnumaf import *
import time

# Setting filepath
print ("The arguments are: " , str(sys.argv))
if sys.argv[1][-1] != "/":
    sys.argv[1] = sys.argv[1] + "/"
    print ("Info: Added '/' to sys.arvg[1]: ", sys.argv[1])

filepath = sys.argv[1]
#Note: This is done in bash: os.makedirs(filepath)# to overwrite, add: , exist_ok=True

# Setting GPU number (0-3 aviable on GTO)
if len(sys.argv) > 2:
    af.set_device(int(sys.argv[2]))
af.info()

# Initializing disk with magnetization in x, y or z
# xyz=0 initializes magnetization in x, xyz=1 in y, xyz=2 in z direction, default is 2 == z
def disk(n0, n1, n2, xyz = 2): 
    m = np.zeros((n0, n1, n2, 3));
    for ix in range (0, n0):
        for iy in range(0, n1):
            a= n0/2
            b= n1/2
            rx=ix-n0/2.
            ry=iy-n1/2.
            r = pow(rx,2)/pow(a,2)+pow(ry,2)/pow(b,2);
            if(r<1):
                m[ix,iy,:,xyz]=1
    return af.from_ndarray(m)

# Physical dimensions in [m]
x = 800e-9
y = 800e-9
z = 3.5e-9
# Discretization 
nx = 250
ny = 250
nz = 1

## Creating mesh
mesh=Mesh(nx, ny, nz, x/nx, y/ny, z/nz)
# Setting material parameters
param=Material()
param.ms=0.5/Constants.mu0 # Saturation magnetization
param.A=15e-12 # Exchange constant
param.Ku1=79e+03 # Anisotropy constant
print ("Check: param.Ku1_axis =", param.Ku1_axis)

# Second param class for stress
param_stress=Material()
param_stress.ms=param.ms
param_stress.A=param.A
param_stress.Ku1=1400 #TODO guessed worst case value fom Toni, elaborate
param_stress.Ku1_axis=[1, 0, 0] # Setting axis in x-direction
print ("Check: param_stress.Ku1_axis =", param_stress.Ku1_axis)

# Create state object with timing
start = time.time()
state = State(mesh, param, disk(nx, ny, nz, 1))
print ("Initialize disk configuration [s]= ", time.time() - start)

# Defining interaction terms
start = time.time()
demag = DemagField(mesh, param)
exch = ExchangeField(mesh, param)
aniso_z = UniaxialAnisotropyField(mesh, param)
aniso_stress = UniaxialAnisotropyField(mesh, param_stress)
zee = ExternalField(af.constant(0.0, nx, ny, nz, 3, dtype=af.Dtype.f64))
print ("Init terms [s]= ", time.time() - start)

# Creating minimizer object
minimizer = LBFGS_Minimizer([demag, exch, aniso_z, aniso_stress, zee])


# Starting minimizer loop

# A passed in mT
if len(sys.argv) > 3:
    A = float(sys.argv[3])*1e-3/Constants.mu0 # expects A in mT
else:
    A = 0.05/Constants.mu0
print ("A = ", A)

# B passed in percent of A
if len(sys.argv) > 4:
    B = float(sys.argv[4])*1e-2*A # expects B relative to A in %
else:
    B = 0.05*A
print ("B = ", B)

# Run full rotation
#stream = open(filepath+"CoPt_s_A%1.2e.dat"%(A*Constants.mu0), "w")
stream = open(filepath+"m.dat", "w")
steps = 100
for i in range(0, steps):
    phi = 2. * np.pi * i/steps;
    Bx = A*np.cos(phi)
    Bz = A*np.sin(phi)
    By = 0
    zee.set_xyz(state, Bx, By, Bz)
    start = time.time()
    minimizer.pyMinimize(state)
    a = zee.get_zee()
    stream.write("%d, %e, %e, %e, %e, %e, %e, %e\n" %(i, phi, a[0,0,0,0].scalar()*Constants.mu0, a[0,0,0,1].scalar()*Constants.mu0, a[0,0,0,2].scalar()*Constants.mu0, state.meanxyz(0), state.meanxyz(1), state.meanxyz(2)))
    stream.flush()
    print ("step ", str(i), ", phi= ", phi, ", time [s]= ", time.time() - start)
    state.py_vti_writer_micro(filepath + "m_"+ str(i))
stream.close()