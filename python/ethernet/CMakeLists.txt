file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/__init__.py "
# Import GNU Radio core first
from gnuradio import gr

# Then import our bindings
try:
    from .ethernet_python import *
except ImportError as e:
    import sys
    print(f'Error importing ethernet_python: {e}', file=sys.stderr)
    raise
")

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/__init__.py
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python3.13/site-packages/gnuradio/ethernet
)

add_subdirectory(bindings)
