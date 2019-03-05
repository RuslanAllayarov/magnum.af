import unittest
import arrayfire as af
import numpy as np
import magnum_af
import math

class AtomisticAnisotropyTest(unittest.TestCase):
  # arbitrary parameters:
  p  = 1e-20
  k  = 1e-20
  dx = 2.715e-10

  def test_atomistic_anisotropy_2_1_1_z_z(self):
    mesh=magnum_af.Mesh(2, 1, 1, self.dx, self.dx, self.dx)
    material=magnum_af.Material()
    material.p=self.p
    material.Ku1_atom=self.k
    m=af.constant(0.0,2,1,1,3,dtype=af.Dtype.f64)
    m[0,0,0,0] = 0
    m[0,0,0,1] = 0
    m[0,0,0,2] = 1

    m[1,0,0,0] = 0
    m[1,0,0,1] = 0
    m[1,0,0,2] = 1

    pystate=magnum_af.State(mesh,material,m)
    atom_ani=magnum_af.AtomisticUniaxialAnisotropyField(mesh, material)
    Llg=magnum_af.LLGIntegrator(atom_ani)

    self.assertAlmostEqual(Llg.get_E(pystate), -2*material.Ku1_atom)

    af_heff = Llg.get_fheff(pystate)
    np_heff = af_heff.__array__()

    self.assertAlmostEqual(np_heff[0,0,0,0], 0 )
    self.assertAlmostEqual(np_heff[0,0,0,1], 0 )
    self.assertAlmostEqual(np_heff[0,0,0,2], 2*material.Ku1_atom/magnum_af.Constants.mu0/material.p )
    self.assertAlmostEqual(np_heff[1,0,0,0], 0 )
    self.assertAlmostEqual(np_heff[1,0,0,1], 0 )
    self.assertAlmostEqual(np_heff[1,0,0,2], 2*material.Ku1_atom/magnum_af.Constants.mu0/material.p )

  def test_atomistic_anisotropy_2_1_1_z_x(self):
    mesh=magnum_af.Mesh(2, 1, 1, self.dx, self.dx, self.dx)
    material=magnum_af.Material()
    material.p =self.p
    material.Ku1_atom =self.k
    m=af.constant(0.0,2,1,1,3,dtype=af.Dtype.f64)
    m[0,0,0,0] = 0
    m[0,0,0,1] = 0
    m[0,0,0,2] = 1

    m[1,0,0,0] = 1
    m[1,0,0,1] = 0
    m[1,0,0,2] = 0

    pystate=magnum_af.State(mesh,material,m)
    atom_ani=magnum_af.AtomisticUniaxialAnisotropyField(mesh, material)
    Llg=magnum_af.LLGIntegrator(atom_ani)

    self.assertAlmostEqual(Llg.get_E(pystate), -material.Ku1_atom)

    af_heff = Llg.get_fheff(pystate)
    np_heff = af_heff.__array__()

    self.assertAlmostEqual(np_heff[0,0,0,0], 0 )
    self.assertAlmostEqual(np_heff[0,0,0,1], 0 )
    self.assertAlmostEqual(np_heff[0,0,0,2], 2*material.Ku1_atom/magnum_af.Constants.mu0/material.p )
    self.assertAlmostEqual(np_heff[1,0,0,0], 0 )
    self.assertAlmostEqual(np_heff[1,0,0,1], 0 )
    self.assertAlmostEqual(np_heff[1,0,0,2], 0 )

  def test_atomistic_anisotropy_1_2_1_z_z(self):
    mesh=magnum_af.Mesh(1, 2, 1, self.dx, self.dx, self.dx)
    material=magnum_af.Material()
    material.p =self.p
    material.Ku1_atom =self.k
    m=af.constant(0.0,1,2,1,3,dtype=af.Dtype.f64)
    m[0,0,0,0] = 0
    m[0,0,0,1] = 0
    m[0,0,0,2] = 1

    m[0,1,0,0] = 0
    m[0,1,0,1] = 0
    m[0,1,0,2] = 1

    pystate=magnum_af.State(mesh,material,m)
    atom_ani=magnum_af.AtomisticUniaxialAnisotropyField(mesh, material)
    Llg=magnum_af.LLGIntegrator(atom_ani)

    self.assertAlmostEqual(Llg.get_E(pystate), -2*material.Ku1_atom)

    af_heff = Llg.get_fheff(pystate)
    np_heff = af_heff.__array__()

    self.assertAlmostEqual(np_heff[0,0,0,0], 0 )
    self.assertAlmostEqual(np_heff[0,0,0,1], 0 )
    self.assertAlmostEqual(np_heff[0,0,0,2], 2*material.Ku1_atom/magnum_af.Constants.mu0/material.p )
    self.assertAlmostEqual(np_heff[0,1,0,0], 0 )
    self.assertAlmostEqual(np_heff[0,1,0,1], 0 )
    self.assertAlmostEqual(np_heff[0,1,0,2], 2*material.Ku1_atom/magnum_af.Constants.mu0/material.p )

if __name__ == '__main__':
  unittest.main()
