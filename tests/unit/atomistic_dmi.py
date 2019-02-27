import unittest
import arrayfire as af
import numpy as np
import magnum_af
import math

class AtomisticDMITest(unittest.TestCase):
  # arbitrary parameters:
  p  = 1e-20
  D  = 1e20
  dx = 1.1

  def test_atomistic_dmi_2_1_1_z_z(self):
    mesh=magnum_af.Mesh(2, 1, 1, self.dx, self.dx, self.dx)
    param=magnum_af.Param()
    param.p =self.p
    param.D_atom =self.D
    m=af.constant(0.0,2,1,1,3,dtype=af.Dtype.f64)
    m[0,0,0,0] = 0
    m[0,0,0,1] = 0
    m[0,0,0,2] = 1

    m[1,0,0,0] = 0
    m[1,0,0,1] = 0
    m[1,0,0,2] = 1

    state=magnum_af.State(mesh,param,m)
    atom_ani=magnum_af.AtomisticDMI(mesh,param)
    Llg=magnum_af.pyLLG(atom_ani)

    self.assertAlmostEqual(Llg.get_E(state), 0)

    af_heff = Llg.get_fheff(state)
    np_heff = af_heff.__array__()

    self.assertAlmostEqual(np_heff[0,0,0,0], -param.D_atom/param.mu0/param.p )
    self.assertLess(math.fabs((np_heff[0,0,0,0] - (-param.D_atom/param.mu0/param.p))/np_heff[0,0,0,0]), 1e-15 )
    self.assertAlmostEqual(np_heff[0,0,0,1], 0 )
    self.assertAlmostEqual(np_heff[0,0,0,2], 0 )
    self.assertAlmostEqual(np_heff[1,0,0,0], param.D_atom/param.mu0/param.p)
    self.assertLess(math.fabs((np_heff[1,0,0,0] - param.D_atom/param.mu0/param.p)/np_heff[1,0,0,0]), 1e-15)
    self.assertAlmostEqual(np_heff[1,0,0,1], 0 )
    self.assertAlmostEqual(np_heff[1,0,0,2], 0 )

  def test_atomistic_dmi_2_1_1_z_x(self):
    mesh=magnum_af.Mesh(2, 1, 1, self.dx, self.dx, self.dx)
    param=magnum_af.Param()
    param.p =self.p
    param.D_atom =self.D
    m=af.constant(0.0,2,1,1,3,dtype=af.Dtype.f64)
    m[0,0,0,0] = 0
    m[0,0,0,1] = 0
    m[0,0,0,2] = 1

    m[1,0,0,0] = 1
    m[1,0,0,1] = 0
    m[1,0,0,2] = 0

    state=magnum_af.State(mesh,param,m)
    atom_ani=magnum_af.AtomisticDMI(mesh,param)
    Llg=magnum_af.pyLLG(atom_ani)

    self.assertAlmostEqual(Llg.get_E(state), - param.D_atom)

    af_heff = Llg.get_fheff(state)
    np_heff = af_heff.__array__()

    self.assertAlmostEqual(np_heff[0,0,0,0], 0 )
    self.assertAlmostEqual(np_heff[0,0,0,1], 0 )
    self.assertAlmostEqual(np_heff[0,0,0,2], param.D_atom/param.mu0/param.p )
    self.assertLess(math.fabs((np_heff[0,0,0,2] - param.D_atom/param.mu0/param.p)/np_heff[0,0,0,2]), 1e-15 )

    self.assertAlmostEqual(np_heff[1,0,0,0], param.D_atom/param.mu0/param.p)
    self.assertLess(math.fabs((np_heff[1,0,0,0] - param.D_atom/param.mu0/param.p)/np_heff[1,0,0,0]), 1e-15)
    self.assertAlmostEqual(np_heff[1,0,0,1], 0 )
    self.assertAlmostEqual(np_heff[1,0,0,2], 0 )

  def test_atomistic_dmi_1_1_2_z_z(self):
    mesh=magnum_af.Mesh(1, 1, 2, self.dx, self.dx, self.dx)
    param=magnum_af.Param()
    param.p =self.p
    param.D_atom =self.D
    m=af.constant(0.0,1,1,2,3,dtype=af.Dtype.f64)
    m[0,0,0,0] = 0
    m[0,0,0,1] = 0
    m[0,0,0,2] = 1

    m[0,0,1,0] = 1
    m[0,0,1,1] = 0
    m[0,0,1,2] = 0

    state=magnum_af.State(mesh,param,m)
    atom_ani=magnum_af.AtomisticDMI(mesh,param)
    Llg=magnum_af.pyLLG(atom_ani)

    self.assertAlmostEqual(Llg.get_E(state), 0)

    af_heff = Llg.get_fheff(state)
    np_heff = af_heff.__array__()

    self.assertAlmostEqual(np_heff[0,0,0,0], 0 )
    self.assertAlmostEqual(np_heff[0,0,0,1], 0 )
    self.assertAlmostEqual(np_heff[0,0,0,2], 0 )
    self.assertAlmostEqual(np_heff[0,0,1,0], 0 )
    self.assertAlmostEqual(np_heff[0,0,1,1], 0 )
    self.assertAlmostEqual(np_heff[0,0,1,2], 0 )

if __name__ == '__main__':
  unittest.main()
