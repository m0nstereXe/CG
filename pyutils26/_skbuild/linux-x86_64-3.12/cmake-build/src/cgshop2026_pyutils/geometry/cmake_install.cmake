# Install script for directory: /common/home/ndb68/CG/pyutils26/src/cgshop2026_pyutils/geometry

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/common/home/ndb68/CG/pyutils26/_skbuild/linux-x86_64-3.12/cmake-install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/src/cgshop2026_pyutils/geometry/_bindings.cpython-312-x86_64-linux-gnu.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/src/cgshop2026_pyutils/geometry/_bindings.cpython-312-x86_64-linux-gnu.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/src/cgshop2026_pyutils/geometry/_bindings.cpython-312-x86_64-linux-gnu.so"
         RPATH "/common/home/ndb68/.conan2/p/fmt091cbdaf9fb57/p/lib:/common/home/ndb68/.conan2/p/cgal43cb00323ffc3/p/lib:/common/home/ndb68/.conan2/p/boost5b5c66082395d/p/lib:/common/home/ndb68/.conan2/p/bzip29db013e43a5ba/p/lib:/common/home/ndb68/.conan2/p/zlib76e00a316e585/p/lib:/common/home/ndb68/.conan2/p/libba8423dde0707eb/p/lib:/common/home/ndb68/.conan2/p/mpfrf8624920ad5ba/p/lib:/common/home/ndb68/.conan2/p/gmpa9a8c61e0bb54/p/lib")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/src/cgshop2026_pyutils/geometry" TYPE MODULE FILES "/common/home/ndb68/CG/pyutils26/_skbuild/linux-x86_64-3.12/cmake-build/src/cgshop2026_pyutils/geometry/_bindings.cpython-312-x86_64-linux-gnu.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/src/cgshop2026_pyutils/geometry/_bindings.cpython-312-x86_64-linux-gnu.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/src/cgshop2026_pyutils/geometry/_bindings.cpython-312-x86_64-linux-gnu.so")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/src/cgshop2026_pyutils/geometry/_bindings.cpython-312-x86_64-linux-gnu.so"
         OLD_RPATH "/common/home/ndb68/.conan2/p/fmt091cbdaf9fb57/p/lib:/common/home/ndb68/.conan2/p/cgal43cb00323ffc3/p/lib:/common/home/ndb68/.conan2/p/boost5b5c66082395d/p/lib:/common/home/ndb68/.conan2/p/bzip29db013e43a5ba/p/lib:/common/home/ndb68/.conan2/p/zlib76e00a316e585/p/lib:/common/home/ndb68/.conan2/p/libba8423dde0707eb/p/lib:/common/home/ndb68/.conan2/p/mpfrf8624920ad5ba/p/lib:/common/home/ndb68/.conan2/p/gmpa9a8c61e0bb54/p/lib:"
         NEW_RPATH "/common/home/ndb68/.conan2/p/fmt091cbdaf9fb57/p/lib:/common/home/ndb68/.conan2/p/cgal43cb00323ffc3/p/lib:/common/home/ndb68/.conan2/p/boost5b5c66082395d/p/lib:/common/home/ndb68/.conan2/p/bzip29db013e43a5ba/p/lib:/common/home/ndb68/.conan2/p/zlib76e00a316e585/p/lib:/common/home/ndb68/.conan2/p/libba8423dde0707eb/p/lib:/common/home/ndb68/.conan2/p/mpfrf8624920ad5ba/p/lib:/common/home/ndb68/.conan2/p/gmpa9a8c61e0bb54/p/lib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/src/cgshop2026_pyutils/geometry/_bindings.cpython-312-x86_64-linux-gnu.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/common/home/ndb68/CG/pyutils26/_skbuild/linux-x86_64-3.12/cmake-build/src/cgshop2026_pyutils/geometry/CMakeFiles/_bindings.dir/install-cxx-module-bmi-Release.cmake" OPTIONAL)
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/common/home/ndb68/CG/pyutils26/_skbuild/linux-x86_64-3.12/cmake-build/src/cgshop2026_pyutils/geometry/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
