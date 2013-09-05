
"""
The pycodes module contains various routines for error correction
and data compression codes in python.

Currently low density parity check codes and their duals are
supported by the pycodes.pyLDPC package.

Various utilities and tests are available in pycodes.tests and
pycodes.utils.

See the doc strings for these sub-modules for more details.

"""

def TestExamples():
    """
    The following function tests all the examples in docstrings.
    """
    import doctest, pycodes, pycodes.pyLDPC
    pycodes.__doc__ = __doc__ + '\n' + pycodes.pyLDPC.__doc__
    for item in pycodes.pyLDPC.__dict__.keys():
        pycodes.__doc__ += '\n' + pycodes.pyLDPC.__dict__[item].__doc__
        
    return doctest.testmod(pycodes)
    print 'Tests Passed.'
