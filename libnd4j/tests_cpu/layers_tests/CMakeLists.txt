include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(../../include ../../layers ../../include/helpers ../../include/exceptions ../../include/execution ../../include/array ../../include/memory ../../include/loops ../../include/graph ../../include/ops ../../include/types ../../include/cnpy ../../blas ../lib/googletest-release-1.8.0/googletest/include)
if(LINUX)
    link_directories(/usr/local/lib)
    link_directories(/usr/lib)
    link_directories(/lib)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
    message("Using apple")
    link_directories(/usr/local/lib)
    link_directories(/usr/lib)
    link_directories(/lib)
endif()
if(WIN32)
    get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
    foreach(dir ${dirs})
        message(STATUS "dir='${dir}'")
    endforeach()
endif()

if (CUDA_BLAS)
	find_package(CUDA)
	message("Tests CUDA include directory: ${CUDA_INCLUDE_DIRS}")
	include_directories(${CUDA_INCLUDE_DIRS})
	add_definitions(-D__CUDABLAS__=true)

    if(WIN32)
        message("CUDA on Windows: enabling /EHsc")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /FS")
        SET_TARGET_PROPERTIES(${LIBND4J_NAME} PROPERTIES COMPILER_FLAGS "/EHsc")
    endif()

    list(APPEND CUDA_NVCC_FLAGS -DCUDA_10 ${EXPM} -w -G -g --cudart=static --expt-extended-lambda -arch=compute_${COMPUTE} -code=sm_${COMPUTE})
endif()

# -fsanitize=address
# -fsanitize=leak
if (APPLE)
    set(CMAKE_CXX_FLAGS  " -fPIC -std=c++11 -fassociative-math -funsafe-math-optimizations -fmax-errors=2 -D__APPLE_OS__=true")
elseif(WIN32)    
	if (CPU_BLAS)
		set(CMAKE_CXX_FLAGS  " -fPIC -std=c++11 -fassociative-math -funsafe-math-optimizations -fmax-errors=2")
	endif()
else()

    set(CMAKE_CXX_FLAGS  " -fPIC -std=c++11 -fassociative-math -funsafe-math-optimizations -fmax-errors=2")
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc64*")
        set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -mcpu=native")
    else()
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}")
    endif()

    if (CPU_BLAS)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -fsanitize=address")
    else()
        # CUDA?
    endif()
endif()

# TODO: get rid of this once problem confirmed solved
#if (APPLE)
#    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#        if ("${CMAKE_C_COMPILER_VERSION}" VERSION_GREATER 6.0 OR "${CMAKE_C_COMPILER_VERSION}" VERSION_EQUAL 6.0)
#            SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wa,-mavx512f -fmax-errors=1")
#        endif()
#    endif()
#endif()

# tests are always compiled with all ops included
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -DLIBND4J_ALL_OPS=true -DBUILD_TESTS=true")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${ARCH_TUNE}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
    SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${ARCH_TUNE} -fp-model fast")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux" AND NOT(MINGW))
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -Wl,-export-dynamic")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -export-dynamic")
endif()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    include_directories("/usr/include")
    include_directories("/usr/local/include")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 4.9)
    message(FATAL_ERROR "You need at least GCC 4.9")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	find_package(OpenMP)
endif()
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
else()
    message("OPENMP NOT FOUND")
endif()

if (CPU_BLAS)
	file(GLOB_RECURSE TEST_SOURCES false ./*.cpp ./*.h)
elseif (CUDA_BLAS)
	file(GLOB_RECURSE TEST_SOURCES false ./*.cpp ./*.cu ./*.h)
endif()

# Filter out any source files from */CMakeFiles/* paths. these tend to cause problems such a multiple main definitions.
set (EXCLUDE_DIR "/CMakeFiles/")
foreach (TMP_PATH ${TEST_SOURCES})
    string (FIND ${TMP_PATH} ${EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
    if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
        list (REMOVE_ITEM TEST_SOURCES ${TMP_PATH})
    endif ()
endforeach(TMP_PATH)

if (CPU_BLAS)
	add_executable(runtests ${TEST_SOURCES})
	target_link_libraries(runtests ${LIBND4J_NAME}static ${MKLDNN_LIBRARIES} ${OPENBLAS_LIBRARIES} gtest gtest_main)
elseif(CUDA_BLAS)
	CUDA_ADD_EXECUTABLE(runtests ${TEST_SOURCES})
	target_link_libraries(runtests ${LIBND4J_NAME} ${CUDA_LIBRARIES} gtest gtest_main)
endif()