# ============================= PROJECT SETUP =================================================
cmake_minimum_required(VERSION 3.16)
project(circus VERSION 0.1.0 LANGUAGES CXX)
add_compile_definitions(_LIBCPP_DISABLE_AVAILABILITY)

set(LIBRARY ${PROJECT_NAME}-lib)
set(EXECUTABLE ${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(QT_QML_GENERATE_QMLLS_INI ON)  # For QML language server support
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/.clangd"
  "CompileFlags:\n  CompilationDatabase: ${CMAKE_CURRENT_BINARY_DIR}\n")
add_compile_definitions(PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
include(FetchContent)

# Qt meta-build helpers:
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
# =============================================================================================

# ============================ BUILD OPTIONS ==================================================
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_PYTHON_BINDINGS "Build python bindings" ON)
# =============================================================================================


# ============================= Dependencies ==================================================
find_package(Eigen3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets Qml Quick Charts)

# Optional deps (linked only if found)
find_package(nlohmann_json QUIET)
find_package(CURL QUIET)

# Generate .clangd configuration after dependencies are found
get_target_property(EIGEN_INCLUDE_DIR Eigen3::Eigen INTERFACE_INCLUDE_DIRECTORIES)
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/.clangd"
  "CompileFlags:\n  CompilationDatabase: ${CMAKE_CURRENT_BINARY_DIR}\n  Add:\n    - -I${CMAKE_CURRENT_SOURCE_DIR}/include\n    - -isystem${EIGEN_INCLUDE_DIR}\n")

# -------------------- yaml-cpp (config -> module -> manual) [robust]
set(_YAML_HINT "/opt/homebrew/opt/yaml-cpp")
find_package(yaml-cpp CONFIG QUIET)
if(NOT (TARGET yaml-cpp::yaml-cpp OR TARGET yaml-cpp))
  find_package(yaml-cpp QUIET)
endif()
if(TARGET yaml-cpp::yaml-cpp OR TARGET yaml-cpp)
  if(TARGET yaml-cpp::yaml-cpp AND NOT TARGET yaml-cpp)
    add_library(yaml-cpp ALIAS yaml-cpp::yaml-cpp)
  elseif(TARGET yaml-cpp AND NOT TARGET yaml-cpp::yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
  endif()
else()
  find_path(YAMLCPP_INCLUDE_DIR yaml-cpp/yaml.h
    HINTS "${_YAML_HINT}/include" /opt/homebrew/include /usr/local/include)
  find_library(YAMLCPP_LIBRARY NAMES yaml-cpp
    HINTS "${_YAML_HINT}/lib" /opt/homebrew/lib /usr/local/lib)
  if(NOT YAMLCPP_LIBRARY OR NOT YAMLCPP_INCLUDE_DIR)
    message(FATAL_ERROR "yaml-cpp not found. Install via pixi/conda-forge or set YAMLCPP_LIBRARY and YAMLCPP_INCLUDE_DIR.")
  endif()
  add_library(yaml-cpp UNKNOWN IMPORTED GLOBAL)
  set_target_properties(yaml-cpp PROPERTIES IMPORTED_LOCATION "${YAMLCPP_LIBRARY}")
  target_include_directories(yaml-cpp INTERFACE "${YAMLCPP_INCLUDE_DIR}")
  add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
endif()

# -------------------- MuJoCo (package first; macOS framework fallback)
find_package(mujoco CONFIG QUIET)
if (TARGET mujoco::mujoco)
  message(STATUS "MuJoCo: using CMake package target mujoco::mujoco")
else()
  message(STATUS "MuJoCo: package not found, trying platform-specific fallback")
  if(APPLE)
    find_library(MUJOCO_FRAMEWORK
      NAMES mujoco
      PATHS /Library/Frameworks ~/Library/Frameworks /Applications/MuJoCo.app/Contents/Frameworks
      PATH_SUFFIXES mujoco.framework
    )
    if(NOT MUJOCO_FRAMEWORK)
      message(FATAL_ERROR "MuJoCo not found: neither mujoco CMake package nor system framework.\n"
                          "Install libmujoco via pixi (recommended) OR copy mujoco.framework to /Library/Frameworks.")
    endif()
    add_library(mujoco::mujoco INTERFACE IMPORTED)
    get_filename_component(_MJ_FW_DIR "${MUJOCO_FRAMEWORK}" DIRECTORY)
    target_link_options(mujoco::mujoco INTERFACE
      "-F${_MJ_FW_DIR}" "-framework" "mujoco" "-Wl,-rpath,${_MJ_FW_DIR}"
    )
    if(EXISTS "${MUJOCO_FRAMEWORK}/Headers/mujoco/mujoco.h")
      target_include_directories(mujoco::mujoco INTERFACE "${MUJOCO_FRAMEWORK}/Headers")
    endif()
  elseif(WIN32)
    message(FATAL_ERROR "Windows fallback for MuJoCo not configured. Please install libmujoco via pixi.")
  else()
    message(FATAL_ERROR "Linux fallback for MuJoCo not configured. Please install libmujoco via pixi.")
  endif()
endif()

# -------------------- pugixml (package first; auto-fetch fallback)
set(_PUGI_TGT "")
find_package(pugixml CONFIG QUIET)
if (TARGET pugixml::pugixml)
  set(_PUGI_TGT pugixml::pugixml)
  message(STATUS "pugixml: using package target pugixml::pugixml")
else()
  message(STATUS "pugixml: package not found, fetching from upstream")
  FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.14
  )
  FetchContent_MakeAvailable(pugixml)
  if(TARGET pugixml)
    set(_PUGI_TGT pugixml)
  elseif(TARGET pugixml-static)
    set(_PUGI_TGT pugixml-static)
  else()
    message(FATAL_ERROR "pugixml target not found after FetchContent; expected 'pugixml' or 'pugixml-static'.")
  endif()
endif()

# ======================= CIRCUS LIBRARY ============================================================

# -------------------- Collect sources and split into: core, main, bindings
set(_src_dir "${CMAKE_SOURCE_DIR}/src")
set(_include_dir "${CMAKE_SOURCE_DIR}/include")

# Source files
file(GLOB_RECURSE SOURCES ${_src_dir}/*.cpp)

# Headers (include them so AUTOMOC sees Q_OBJECT)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
  "${_src_dir}/*.[hH]" "${_src_dir}/*.[hH]pp"
  "${_include_dir}/*.[hH]" "${_include_dir}/*.[hH]pp"
)

# Optional Qt UI and resource files (AUTOUIC/AUTORCC will process these)
file(GLOB_RECURSE ALL_UIS  CONFIGURE_DEPENDS "${_src_dir}/*.ui")
file(GLOB_RECURSE ALL_QRCS CONFIGURE_DEPENDS "${_src_dir}/*.qrc")

# Add QML resources - only if the file exists
if(EXISTS "${CMAKE_SOURCE_DIR}/resources/qml.qrc")
  qt_add_resources(QML_RESOURCES "${CMAKE_SOURCE_DIR}/resources/qml.qrc")
else()
  set(QML_RESOURCES "")
  message(WARNING "QML resources not found at ${CMAKE_SOURCE_DIR}/resources/qml.qrc - skipping QML resources")
endif()

# Identify entry points we don't want in the core lib
set(MAIN_SRC "")
if(EXISTS "${_src_dir}/main.cpp")
  set(MAIN_SRC "${_src_dir}/main.cpp")
endif()

set(BINDINGS_SRC "")
if(EXISTS "${_src_dir}/Bindings.cpp")
  set(BINDINGS_SRC "${_src_dir}/Bindings.cpp")
endif()


if(MAIN_SRC)
  list(REMOVE_ITEM SOURCES "${MAIN_SRC}")
endif()
if(BINDINGS_SRC)
  list(REMOVE_ITEM SOURCES "${BINDINGS_SRC}")
endif()


# -------------------- Libraries to link everywhere
set(_EXTRA_LIBS
  mujoco::mujoco
  yaml-cpp::yaml-cpp
  Eigen3::Eigen
  Qt6::Core
  Qt6::Widgets
  Qt6::OpenGLWidgets
  Qt6::Qml
  Qt6::Quick
  Qt6::Charts
  ${_PUGI_TGT}
  CURL::libcurl
  nlohmann_json::nlohmann_json
)

# --- Build core library (ensure AUTOMOC is on for the target)
add_library(${LIBRARY} STATIC ${SOURCES} ${QML_RESOURCES})
target_sources(${LIBRARY} PRIVATE ${HEADERS} ${ALL_UIS} ${ALL_QRCS}) # Just used for IDE discovering headers

set_target_properties(${LIBRARY} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
target_include_directories(${LIBRARY}
  PUBLIC
  $<BUILD_INTERFACE:${_include_dir}>
)
target_link_libraries(${LIBRARY} PUBLIC ${_EXTRA_LIBS})

# -------------------- Build the app
if(MAIN_SRC)
  add_executable(${EXECUTABLE} ${MAIN_SRC})
  target_link_libraries(${EXECUTABLE} PRIVATE ${LIBRARY})

  install(TARGETS ${EXECUTABLE}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE  DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
else()
  message(WARNING "No main.cpp found in ${_src_dir}; skipping executable '${EXECUTABLE}'.")
endif()

# ==============================================================================================


# =============================== EXPORT =======================================================
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# 1. Install the library target and headers
install(TARGETS ${LIBRARY}
    EXPORT ${LIBRARY}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

# Install headers preserving directory structure
install(DIRECTORY ${_include_dir}/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# 2. Generate the relocatable package config file in the build directory
configure_package_config_file( # creates config file for find_package
    "cmake/config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY}"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)
write_basic_package_version_file( # sets version of the package
  "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# 3. Install the package config files and the export set
install(
    EXPORT ${LIBRARY}Targets
    FILE ${LIBRARY}Targets.cmake
    NAMESPACE spqr::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY}"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBRARY}ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY}
)

# ==============================================================================================


# ================================ Python bindings via nanobind =================================
if(BUILD_PYTHON_BINDINGS AND BINDINGS_SRC)
  # Python (Interpreter + Development.Module)
  set(_PY_COMPS Interpreter Development.Module)
  find_package(Python QUIET COMPONENTS ${_PY_COMPS})
  if(NOT Python_FOUND)
    find_package(Python3 REQUIRED COMPONENTS ${_PY_COMPS})
    set(Python_EXECUTABLE ${Python3_EXECUTABLE})
    set(Python_SITEARCH  ${Python3_SITEARCH})
  endif()

  # Locate nanobind's CMake package via the Python module
  execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE _NB_CMAKE_DIR
    RESULT_VARIABLE _NB_RET
  )
  if(NOT _NB_RET EQUAL 0)
    message(FATAL_ERROR "Failed to locate nanobind via 'python -m nanobind --cmake_dir'. "
                        "Is 'nanobind' installed in this pixi env?")
  endif()
  set(nanobind_DIR "${_NB_CMAKE_DIR}")
  find_package(nanobind CONFIG REQUIRED)

  # Determine Python's site-packages (platlib)
  execute_process(
      COMMAND ${Python_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('platlib'))"
      OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  # Auto-detect module name from NB_MODULE(...) in Bindings.cpp (e.g., 'circuspy')
  file(READ "${BINDINGS_SRC}" _NB_SRC)
  string(REGEX MATCH "NB_MODULE\\s*\\(([A-Za-z_][A-Za-z0-9_]*)" _m "${_NB_SRC}")
  if(NOT _m)
    message(FATAL_ERROR "Could not find NB_MODULE(...) in ${BINDINGS_SRC}")
  endif()
  set(PY_MODULE_NAME "${CMAKE_MATCH_1}")
  message(STATUS "nanobind: detected module name '${PY_MODULE_NAME}' from ${BINDINGS_SRC}")

  # Build the nanobind module
  nanobind_add_module(${PY_MODULE_NAME} ${BINDINGS_SRC})
  target_link_libraries(${PY_MODULE_NAME} PRIVATE ${LIBRARY})

  # Install Python module into the environment's site-packages
  install(
      TARGETS ${PY_MODULE_NAME}
      EXPORT ${PY_MODULE_NAME}Targets
      LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${BINDIR}
  )
endif()

# ===============================================================================================
