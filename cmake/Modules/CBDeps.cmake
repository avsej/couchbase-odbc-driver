# Downloads the 'cbdep' utility and defines a macro "cbdep_install()"
# to make use of it

set (CBDEP_VERSION 1.1.8)

# Platform detection for cbdep binary download and install flags
if (WIN32)
  set (_cbdep_os "windows")
  set (_cbdep_exe_suffix ".exe")
  set (_cbdep_install_platform "windows")
elseif (APPLE)
  set (_cbdep_os "darwin")
  set (_cbdep_exe_suffix "")
  set (_cbdep_install_platform "macos")
else ()
  set (_cbdep_os "linux")
  set (_cbdep_exe_suffix "")
  set (_cbdep_install_platform "linux")
endif ()

# Architecture detection
# Note: this runs before project(), so CMAKE_SYSTEM_PROCESSOR isn't set yet.
# Use uname on non-Windows, hardcode x86_64 on Windows.
if (WIN32)
  set (_cbdep_arch "x86_64")
else ()
  execute_process (COMMAND uname -m
    OUTPUT_VARIABLE _cbdep_arch
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (_cbdep_arch MATCHES "(x86_64|amd64|AMD64)")
    set (_cbdep_arch "x86_64")
  elseif (_cbdep_arch MATCHES "(aarch64|arm64|ARM64)")
    set (_cbdep_arch "arm64")
  endif ()
endif ()

# Utilitize cbdep's own cache dir
if (WIN32)
  set (_cbdepcache_dir "$ENV{HOMEDRIVE}/$ENV{HOMEPATH}/.cbdepcache")
else ()
  set (_cbdepcache_dir "$ENV{HOME}/.cbdepcache")
endif ()
if (NOT IS_DIRECTORY "${_cbdepcache_dir}")
  file (MAKE_DIRECTORY "${_cbdepcache_dir}")
endif ()

set (CBDEP_EXE "${_cbdepcache_dir}/cbdep-${CBDEP_VERSION}${_cbdep_exe_suffix}")
if (NOT EXISTS "${CBDEP_EXE}")
  if (_cbdep_os STREQUAL "windows")
    set (_cbdep_url "https://packages.couchbase.com/cbdep/${CBDEP_VERSION}/cbdep-${CBDEP_VERSION}-windows-x86_64.exe")
  elseif (_cbdep_os STREQUAL "darwin")
    set (_cbdep_url "https://packages.couchbase.com/cbdep/${CBDEP_VERSION}/cbdep-${CBDEP_VERSION}-darwin-${_cbdep_arch}")
  else ()
    set (_cbdep_url "https://packages.couchbase.com/cbdep/${CBDEP_VERSION}/cbdep-${CBDEP_VERSION}-linux-${_cbdep_arch}")
  endif ()
  message (STATUS "Downloading cbdep ${CBDEP_VERSION}...")
  file (DOWNLOAD "${_cbdep_url}" "${CBDEP_EXE}" STATUS _stat SHOW_PROGRESS)
  list (GET _stat 0 _retval)
  if (_retval)
    file (REMOVE "${CBDEP_EXE}")
    list (GET _stat 1 _message)
    message (
      FATAL_ERROR "Error downloading ${_cbdep_url}: ${_message} (${_retval})"
    )
  endif ()
  if (NOT WIN32)
    execute_process (COMMAND chmod "+x" "${CBDEP_EXE}")
  endif ()
endif ()

include (ParseArguments)

# Generic function for installing a cbdep (2.0) package to a given directory
# Required arguments:
#   PACKAGE - package to install
#   VERSION - version number of package (must be understood by 'cbdep' tool)
# Optional arguments:
#   INSTALL_DIR - where to install to; defaults to CMAKE_CURRENT_BINARY_DIR
MACRO (CBDEP_INSTALL)
  PARSE_ARGUMENTS (cbdep "" "INSTALL_DIR;PACKAGE;VERSION" "" ${ARGN})
  IF (NOT cbdep_INSTALL_DIR)
    SET (cbdep_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  ENDIF ()
  IF(NOT IS_DIRECTORY "${cbdep_INSTALL_DIR}/${cbdep_PACKAGE}-${cbdep_VERSION}")
    MESSAGE (STATUS "Downloading and caching ${cbdep_PACKAGE}-${cbdep_VERSION}")
    EXECUTE_PROCESS (
      COMMAND "${CBDEP_EXE}" -p "${_cbdep_install_platform}"
        install -d "${cbdep_INSTALL_DIR}"
        ${cbdep_PACKAGE} ${cbdep_VERSION}
      RESULT_VARIABLE _cbdep_result
      OUTPUT_VARIABLE _cbdep_out
      ERROR_VARIABLE _cbdep_out
    )
    IF (_cbdep_result)
      FILE (REMOVE_RECURSE "${cbdep_INSTALL_DIR}")
      MESSAGE (FATAL_ERROR "Failed installing cbdep ${cbdep_PACKAGE} ${cbdep_VERSION}: ${_cbdep_out}")
    ENDIF ()
  ENDIF()
ENDMACRO (CBDEP_INSTALL)
