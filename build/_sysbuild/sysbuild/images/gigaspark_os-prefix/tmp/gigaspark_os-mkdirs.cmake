# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/gabo/gigaspark_os")
  file(MAKE_DIRECTORY "/home/gabo/gigaspark_os")
endif()
file(MAKE_DIRECTORY
  "/home/gabo/gigaspark_os/build/gigaspark_os"
  "/home/gabo/gigaspark_os/build/_sysbuild/sysbuild/images/gigaspark_os-prefix"
  "/home/gabo/gigaspark_os/build/_sysbuild/sysbuild/images/gigaspark_os-prefix/tmp"
  "/home/gabo/gigaspark_os/build/_sysbuild/sysbuild/images/gigaspark_os-prefix/src/gigaspark_os-stamp"
  "/home/gabo/gigaspark_os/build/_sysbuild/sysbuild/images/gigaspark_os-prefix/src"
  "/home/gabo/gigaspark_os/build/_sysbuild/sysbuild/images/gigaspark_os-prefix/src/gigaspark_os-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/gabo/gigaspark_os/build/_sysbuild/sysbuild/images/gigaspark_os-prefix/src/gigaspark_os-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/gabo/gigaspark_os/build/_sysbuild/sysbuild/images/gigaspark_os-prefix/src/gigaspark_os-stamp${cfgdir}") # cfgdir has leading slash
endif()
