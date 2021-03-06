# ===========================================================================
#                  SeqAn - The Library for Sequence Analysis
# ===========================================================================
# File: /sandbox/h4nn3s/apps/lambda/CMakeLists.txt
#
# CMakeLists.txt file for lambda.
# ===========================================================================

# ----------------------------------------------------------------------------
# App version
# ----------------------------------------------------------------------------

# change this after every release
set (SEQAN_APP_VERSION_MAJOR "1")
set (SEQAN_APP_VERSION_MINOR "0")
set (SEQAN_APP_VERSION_PATCH "2")

# don't change the following
set (SEQAN_APP_VERSION "${SEQAN_APP_VERSION_MAJOR}.${SEQAN_APP_VERSION_MINOR}.${SEQAN_APP_VERSION_PATCH}")

# ----------------------------------------------------------------------------
# Dependencies (continued)
# ----------------------------------------------------------------------------

# Search SeqAn and select dependencies.
find_package(OpenMP QUIET)
find_package(ZLIB   QUIET)
find_package(BZip2  QUIET)
find_package(SeqAn  QUIET REQUIRED)

message(STATUS "These dependencies where found:")
message(   "     OPENMP     ${OPENMP_FOUND}      ${OpenMP_CXX_FLAGS}")
message(   "     ZLIB       ${ZLIB_FOUND}      ${ZLIB_VERSION_STRING}")
message(   "     BZIP2      ${BZIP2_FOUND}      ${BZIP2_VERSION_STRING}")
message(   "     SEQAN      ${SEQAN_FOUND}      ${SEQAN_VERSION_STRING}")

# Warn if OpenMP was not found.
if (NOT OPENMP_FOUND)
    message (WARNING "WARNING WARNING WARNING\nWARNING: OpenMP not found. Lambda will be built without multi-threading! "
    "This is probably not what you want! Use GCC >= 4.9.1, Clang >= 3.8.0 or ICC >= 16.0.2\nWARNING WARNING WARNING")
endif (NOT OPENMP_FOUND)

# Warn if Zlib was not found.
if (NOT ZLIB_FOUND)
  message (WARNING "WARNING: Zlib not found. Building lambda without support for gzipped input and output (this includes support for .bam).")
endif (NOT ZLIB_FOUND)

# Warn if BZip2 was not found.
if (NOT BZIP2_FOUND)
  message (WARNING "WARNING: BZip2 not found. Building lambda without support for bzipped input and output.")
endif (NOT BZIP2_FOUND)

if (CMAKE_COMPILER_IS_GNUCXX)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9.1)
        message (FATAL_ERROR "Your GCC version is too old. Minimum version is GCC-4.9.1!")
        return ()
    endif ()
elseif (COMPILER_IS_CLANG)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.6)
        message (FATAL_ERROR "Your Clang version is too old. Please upgrade to 3.8.0 or use GCC.")
        return()
    elseif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.8)
        message (WARNING "Your Clang version is too old, you will not have parallelism! Upgrade to 3.8.0 or newer.")
    endif ()
elseif (COMPILER_IS_INTEL)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16.0.2)
        message(WARNING "Your Intel Compiler version is too old. Please upgrade to 16.0.2 or newer!")
    endif ()
endif ()

if (SEQAN_VERSION_STRING VERSION_LESS "2.2.0")
    message (FATAL_ERROR "The minimum SeqAn verison required is 2.2.0!")
    return ()
endif ()

message(STATUS "The requirements where met.")

# ----------------------------------------------------------------------------
# App-Level Configuration
# ----------------------------------------------------------------------------

message ("\n${ColourBold}Build configuration${ColourReset}")

message (STATUS "LAMBDA version is: ${SEQAN_APP_VERSION}")

option (LAMBDA_FASTBUILD    "Build only blastp and blastx modes (speeds up build)."                             OFF)
option (LAMBDA_NATIVE_BUILD "Architecture-specific optimizations, i.e. g++ -march=native."                      ON)
option (LAMBDA_STATIC_BUILD "Include all libraries in the binaries."                                            OFF)
option (LAMBDA_MMAPPED_DB   "Use mmapped access to the database."                                               ON)
option (LAMBDA_LINGAPS_OPT  "Add optimized codepaths for linear gap costs (inc. bin size and compile time)."    OFF)

if (LAMBDA_FASTBUILD)
    add_definitions (-DFASTBUILD=1)
endif (LAMBDA_FASTBUILD)

if (LAMBDA_NATIVE_BUILD)
    add_definitions (-DLAMBDA_NATIVE_BUILD=1)
    set (SEQAN_CXX_FLAGS "${SEQAN_CXX_FLAGS} -march=native")
    if (COMPILER_IS_INTEL)
        set (SEQAN_CXX_FLAGS "${SEQAN_CXX_FLAGS} -xHOST -ipo -no-prec-div -fp-model fast=2")
    endif (COMPILER_IS_INTEL)
endif (LAMBDA_NATIVE_BUILD)

if (LAMBDA_STATIC_BUILD)
    add_definitions (-DLAMBDA_STATIC_BUILD=1)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    # apple does not support fully static builds, but at least libgcc and libstdc++
    if (APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
        message (WARNING "WARNING: Builds on Mac are never fully static.")
    else (APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif (APPLE)
    # on linux cmake adds -rdynamic automatically which clang can't handle in static builds
    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
    endif (CMAKE_SYSTEM_NAME MATCHES "Linux")
endif (LAMBDA_STATIC_BUILD)

if (LAMBDA_MMAPPED_DB)
    add_definitions (-DLAMBDA_MMAPPED_DB=1)
endif (LAMBDA_MMAPPED_DB)

if (LAMBDA_LINGAPS_OPT)
    add_definitions (-DLAMBDA_LINGAPS_OPT=1)
endif ()

message(STATUS "The following options are selected for the build:")
message(   "     LAMBDA_FASTBUILD         ${LAMBDA_FASTBUILD}")
message(   "     LAMBDA_LINGAPS_OPT       ${LAMBDA_LINGAPS_OPT}")
message(   "     LAMBDA_MMAPPED_DB        ${LAMBDA_MMAPPED_DB}")
message(   "     LAMBDA_NATIVE_BUILD      ${LAMBDA_NATIVE_BUILD}")
message(   "     LAMBDA_STATIC_BUILD      ${LAMBDA_STATIC_BUILD}")
message(STATUS "Run 'cmake -LH' to get a comment on each option.")
message(STATUS "Remove CMakeCache.txt and re-run cmake with -DOPTIONNAME=ON|OFF to change an option.")

# deactivate the version check until all issues are resolved
add_definitions (-DSEQAN_DISABLE_VERSION_CHECK="YES")

# ----------------------------------------------------------------------------
# Compiler specifics
# ----------------------------------------------------------------------------

if (COMPILER_IS_CLANG)
    set (SEQAN_CXX_FLAGS "${SEQAN_CXX_FLAGS} -ftemplate-depth-1024")

    # do not warn for variable length arrays
    set (SEQAN_CXX_FLAGS "${SEQAN_CXX_FLAGS} -Wno-vla-extension")
endif (COMPILER_IS_CLANG)

if(CMAKE_COMPILER_IS_GNUCXX)
    # do not warn for variable length arrays
    set (SEQAN_CXX_FLAGS "${SEQAN_CXX_FLAGS} -Wno-vla")

    # parallelize parts of build even for one translation unit
    include(ProcessorCount)
    ProcessorCount(NCPU)
    if(NCPU GREATER 1)
        set (SEQAN_CXX_FLAGS "${SEQAN_CXX_FLAGS} -flto=${NCPU}")
    endif()
endif(CMAKE_COMPILER_IS_GNUCXX)

if (NOT COMPILER_IS_INTEL)
    # strip binaries to make them smaller
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
endif ()

# ----------------------------------------------------------------------------
# Build Setup
# ----------------------------------------------------------------------------

# Enable global exception handler for all seqan apps.
set (SEQAN_DEFINITIONS "${SEQAN_DEFINITIONS} -DSEQAN_GLOBAL_EXCEPTION_HANDLER")

# Add include directories.
include_directories (${SEQAN_INCLUDE_DIRS})

# Add definitions set by find_package (SeqAn).
add_definitions (${SEQAN_DEFINITIONS})

# Add definitions set by the build system.
add_definitions (-DSEQAN_VERSION_STRING="${SEQAN_VERSION_STRING}")
add_definitions (-DSEQAN_REVISION="${SEQAN_REVISION}")
add_definitions (-DSEQAN_DATE="${SEQAN_DATE}")
add_definitions (-DSEQAN_APP_VERSION="${SEQAN_APP_VERSION}")
add_definitions (-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")

# Set the right output directory
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Update the list of file names below if you add source files to your application.
add_executable (lambda lambda.cpp
                lambda.hpp
                misc.hpp
                match.hpp
                options.hpp
                holders.hpp
                output.hpp)

add_executable (lambda_indexer lambda_indexer.cpp
                lambda_indexer.hpp
                options.hpp
                lambda_indexer_misc.hpp
                radix_inplace.h)

# Add dependencies found by find_package (SeqAn).
target_link_libraries (lambda ${SEQAN_LIBRARIES})
target_link_libraries (lambda_indexer ${SEQAN_LIBRARIES})

# Add CXX flags found by find_package (SeqAn).
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SEQAN_CXX_FLAGS}")

set(BUILD_SHARED_LIBS OFF)

# ----------------------------------------------------------------------------
# Installation
# ----------------------------------------------------------------------------

# Install lambda in ${PREFIX}/bin directory
install (TARGETS lambda lambda_indexer
         DESTINATION bin)

# Install non-binary files for the package to share/lambda
install (FILES ../LICENSE.rst
               ../LICENSE-BSD.rst
               ../LICENSE-GPL3.rst
               ../README.rst
         DESTINATION "share/doc/lambda")

# ----------------------------------------------------------------------------
# CPack Install
# ----------------------------------------------------------------------------

# Information
set (CPACK_PACKAGE_NAME "lambda")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "lambda -- the local aligner for massive bioligical data")
set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../README.rst")
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE.rst")
set (CPACK_PACKAGE_VENDOR "Hannes Hauswedell <hannes.hauswedell@fu-berlin.de>")
set (CPACK_PACKAGE_CONTACT "${CPACK_PACKAGE_VENDOR}")
set (CPACK_PACKAGE_VERSION_MAJOR "${SEQAN_APP_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${SEQAN_APP_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${SEQAN_APP_VERSION_PATCH}")
set (CPACK_PACKAGE_VERSION "${SEQAN_APP_VERSION}")
set (CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")

# Package format(s)
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CPACK_GENERATOR "ZIP;NSIS")
elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(CPACK_GENERATOR "ZIP;DragNDrop")
elseif (CMAKE_VERSION VERSION_LESS "3.1") # TXZ support since 3.1
    set(CPACK_GENERATOR "TBZ2")
else()
    set(CPACK_GENERATOR "TXZ")
endif ()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CPACK_GENERATOR "${CPACK_GENERATOR};DEB;RPM")
endif ()

# Package architecture
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")
    set(CMAKE_SYSTEM_PROCESSOR "x86_64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
endif ()
if (CMAKE_CXX_FLAGS MATCHES "avx2")
    set (CMAKE_SYSTEM_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}_avx2")
elseif (CMAKE_CXX_FLAGS MATCHES "sse4")
    set (CMAKE_SYSTEM_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}_sse4")
endif()

# Include architecture in package name
if (NOT DEFINED CPACK_SYSTEM_NAME)
  set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif (NOT DEFINED CPACK_SYSTEM_NAME)

include (CPack)
