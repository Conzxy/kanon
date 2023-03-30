#!/bin/bash
doxygen kanon_doxy && \
doxysphinx build source docs kanon_doxy && \
sphinx-build -b html source docs

