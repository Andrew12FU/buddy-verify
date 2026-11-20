# Uses the existing frontend's LLVM/Clang discovery only; no MiniMLIR linkage.
if(NOT LLVM_VERSION_MAJOR EQUAL 18)
  message(FATAL_ERROR "ExtractMiniMLIR requires Clang 18.1.8")
endif()
add_executable(buddy-extract-minimlir
  src/driver/Main.cpp src/model/Registry.cpp src/analysis/Collect.cpp
  src/analysis/Relations.cpp src/generation/Policy.cpp src/generation/Emit.cpp)
target_compile_features(buddy-extract-minimlir PRIVATE cxx_std_17)
target_include_directories(buddy-extract-minimlir PRIVATE include)
target_include_directories(buddy-extract-minimlir SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_link_libraries(buddy-extract-minimlir PRIVATE clang-cpp LLVM)
if(BUILD_TESTING)
  add_test(NAME extract-skeleton COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_skeleton.py
    --tool $<TARGET_FILE:buddy-extract-minimlir> --clang ${LLVM_TOOLS_BINARY_DIR}/clang++)
endif()
