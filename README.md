# jobyaviation
simulate eVTOL aircrafts navigation pattern

## ASSUMPTIONS
- all hours values in companies are directly converted to seconds
- all simulation time is converted from hours to seconds
  - e.g. 0.5 hours is 30 seconds

## HOW TO USE ?
- Install jsoncpp
  - mac: brew install meson ninja jsoncpp
  - ubuntu: sudo apt install libjsoncpp-dev
- cd {project-root}
- make all

## COULD HAVE DONE
- implement classes in header file
  - did not separate them out in to .cpp and .h
- use floats for metrics
