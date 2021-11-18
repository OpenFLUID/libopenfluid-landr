# OpenFLUID LandR library

The OpenFLUID LandR (`openfluid-landr`) library is a C++ library for topological processing of geometries, focused on digital representation of landscapes geometries.  
It was formerly part of the [OpenFLUID software sources](https://github.com/OpenFLUID).


The `openfluid-landr` library relies on the following sofware

* [OpenFLUID framework](https://openfluid-project.org)
* [GEOS library](https://trac.osgeo.org/geos)
* [Boost library](https://www.boost.org/)

:warning: **Due to major breaking changes in GEOS dependency, the openfluid-landr source code is not currently stable and does not compile as is**


## Build

_NOTE: The build of openfluid-landr library has only been tested on Linux systems (Ubuntu 18.04 and 20.04)_  


It requires the [GCC compiler](https://gcc.gnu.org/) compiler and the [CMake configuration tool](https://cmake.org/) to be built.  

* Create the build directory in the sources directory
```txt
mkdir _build
cd _build
```

* Configure the build
```txt
cmake ..
```

* Execute the build
```txt
make
```

* Run the tests
```txt
ctest
```