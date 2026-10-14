
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was minifb-config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

# filled and installed by cmake
# once filled, this file will be found by find_package

# depending on the configuration minifb was built with, make sure it has the required dependencies
include(CMakeFindDependencyMacro)
if(ON)
    find_dependency(OpenGL)
endif()
if(ON)
    find_dependency(X11)
endif()

# run the auto-generated file to create the exported minifb:: target
include("${CMAKE_CURRENT_LIST_DIR}/minifb-targets.cmake")

check_required_components(minifb)
