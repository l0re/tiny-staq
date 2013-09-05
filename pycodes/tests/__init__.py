
"""
The tests modules contain various tests both to insure that the
program is running correctly as well as to simulate codes on
graphs with various parameters and decoding algorithms.

Tests for channel coding are in the sub-module channel_coding
while tests for quantization are in the sub-module quantization.

The function DoRegressionTests() provided by this module
runs various regression tests to insure that everything is working
correctly.
"""

import sys

def DoRegressionTests():
    sys.stdout.write('Doing quantization tests:\n')
    sys.stdout.flush()
    
    from pycodes.tests.quantization.BEC import bec_quant

    sys.stdout.write('Doing channel coding tests:\n')
    sys.stdout.flush()
    
    from pycodes.tests.channel_coding.BSC import cmp_exact_apps_on_tree
    from pycodes.tests.channel_coding.BSC import regress
    from pycodes.tests.channel_coding.BEC import regress

