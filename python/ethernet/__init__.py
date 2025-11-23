# Import the Python bindings
try:
    from .ethernet_python import *
except ImportError as e:
    import sys
    print(f"Error importing ethernet_python: {e}", file=sys.stderr)
    pass
