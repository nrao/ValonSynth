#!/usr/bin/env python

from distutils.core import setup, Extension

setup(name        = 'ValonSynth',
      author      = 'Patrick Brandt',
      maintainer  = 'NRAO',
      packages    = ['valon_synth'],
      package_dir = {'valon_synth': 'src'},
      requires    = ['pyserial'],
      )
