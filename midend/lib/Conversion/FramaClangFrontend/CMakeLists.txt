cmake_minimum_required(VERSION 3.20)
project(BuddyFramaClangFrontend LANGUAGES C CXX)

# The frontend and external Frama-Clang converter default to Clang 18.
# This CMake configuration controls only the frontend, never the converter.
set(FRONTEND_LLVM_MAJOR "18" CACHE STRING "Frontend LLVM/Clang major (18 default or 23 experiment)")
if(NOT FRONTEND_LLVM_MAJOR MATCHES "^(18|23)$")
  message(FATAL_ERROR "Supported frontend LLVM/Clang majors: 18 and 23")
endif()
find_package(LLVM REQUIRED CONFIG)
if(NOT LLVM_VERSION_MAJOR EQUAL FRONTEND_LLVM_MAJOR)
  message(FATAL_ERROR "Requested LLVM ${FRONTEND_LLVM_MAJOR}, found ${LLVM_PACKAGE_VERSION}")
endif()
find_package(Clang REQUIRED CONFIG)
message(STATUS "Frontend parser: LLVM/Clang ${LLVM_PACKAGE_VERSION}; Frama-Clang is configured separately")
# 分层编译：入口不实现规则，规则只依赖配置/AST 支持和内存修改计划。
add_executable(buddy-framac-rewrite
  src/frontend/RewriteFrontend.cpp
  src/frontend/FrontendOptions.cpp
  src/rewrite/RewritePlan.cpp
  src/support/AstSupport.cpp
  src/frontend/RewritePipeline.cpp
  src/support/FrontendOutput.cpp
  src/rules/NamespaceRule.cpp
  src/rules/ConstLocalRule.cpp
  src/rules/InitializerListRule.cpp
  src/rules/ConstructorRule.cpp
  src/rules/RangeForRule.cpp
  src/rules/WhileRule.cpp
  src/rules/BorrowedCapture.cpp
  src/rules/CaptureRule.cpp
  src/rules/NestedCapture.cpp
  src/rules/GuardRule.cpp
  src/rules/IfConditionRule.cpp
  src/rules/ValueRangeRule.cpp)
target_compile_features(buddy-framac-rewrite PRIVATE cxx_std_17)
target_include_directories(buddy-framac-rewrite PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(buddy-framac-rewrite SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
target_link_libraries(buddy-framac-rewrite PRIVATE clang-cpp LLVM)

# 同一个前端的接口抽象能力；目前提供有界先驱探针，尚不生成完整 MLIR 头。
# 与改写共用 LLVM/Clang 配置和 build/，业务逻辑保持独立。
add_executable(header-extraction-probe
  src/interface/ProbeMain.cpp
  src/interface/Probe.cpp
  src/interface/Inventory.cpp
  src/interface/LibrarySlice.cpp)
target_compile_features(header-extraction-probe PRIVATE cxx_std_17)
target_include_directories(header-extraction-probe PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(header-extraction-probe SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_link_libraries(header-extraction-probe PRIVATE clang-cpp LLVM)

add_executable(buddy-mini-slice
  src/interface/MiniSliceMain.cpp
  src/interface/MiniSlice.cpp)
target_compile_features(buddy-mini-slice PRIVATE cxx_std_17)
target_include_directories(buddy-mini-slice PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(buddy-mini-slice SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_link_libraries(buddy-mini-slice PRIVATE clang-cpp LLVM)

include(CTest)
if(BUILD_TESTING)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_test(NAME imported-pass-regressions COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_imported_passes.py
    --tool $<TARGET_FILE:buddy-framac-rewrite>
    --clang ${LLVM_TOOLS_BINARY_DIR}/clang++)
  add_test(NAME range-for-regressions COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_range_for.py
    --tool $<TARGET_FILE:buddy-framac-rewrite>
    --clang ${LLVM_TOOLS_BINARY_DIR}/clang++)
  add_test(NAME output-persistence COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_output.py)
  add_test(NAME mini-header-rewrites COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mini_headers.py
    --clang ${LLVM_TOOLS_BINARY_DIR}/clang++)
  add_test(NAME mini-member-extraction COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mini_slice.py
    --tool $<TARGET_FILE:buddy-mini-slice>
    --clang ${LLVM_TOOLS_BINARY_DIR}/clang++)
  add_test(NAME rewrite-regressions COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_rewrite.py
    --tool $<TARGET_FILE:buddy-framac-rewrite>
    --clang ${LLVM_TOOLS_BINARY_DIR}/clang++)
  add_test(NAME nested-capture-regressions COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_nested_capture.py
    --tool $<TARGET_FILE:buddy-framac-rewrite>
    --clang ${LLVM_TOOLS_BINARY_DIR}/clang++)
  add_test(NAME silu-and-ime-regressions COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_silu.py
    --tool $<TARGET_FILE:buddy-framac-rewrite>
    --clang ${LLVM_TOOLS_BINARY_DIR}/clang++)
  add_test(NAME parser-version-regressions COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_parser_version.py
    --tool $<TARGET_FILE:buddy-framac-rewrite>
    --clang ${LLVM_TOOLS_BINARY_DIR}/clang++)
endif()
