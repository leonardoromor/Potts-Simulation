# Potts simulation model

[![Join the chat at https://gitter.im/leonardoromor/Potts-Simulation](https://badges.gitter.im/leonardoromor/Potts-Simulation.svg)](https://gitter.im/leonardoromor/Potts-Simulation?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

## Folder tree overview

The code is split in a "parallel" which make use of MPI
(a task manager) and
a "serial" part which is the simulation itself and exploits vectorization.

* build: folder where the binary files will be generated. The parallel binary files (meant to be run with mpirun) are stored inside the sub-folder "parallel".
* docs: stores the Doxygen documentation.
* include: stores the front-end header files.
* lib: stores the back-end library source and header files
* src: stores the source files with the main function.

## Requirements
The single core simulation code is meant to be compiled and run with the following requirements:

* Intel C++ Compiler v15 or higher
* GNU gcc version 4.9 or higher for the c++11 standard library
* OpenMPI with mpic++ binded to the above defined Intel compiler
* Intel CPU with AVX

## Make commands:
Brief list of the most important make commands:

* make: compile both parallel and single core executables with separated compiling and linking phases.
* make clean: cleanup the build folder.
* make build: generate just the object files without linking.
* make debug: run a make all with the -g compiler option
