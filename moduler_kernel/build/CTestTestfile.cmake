# CMake generated Testfile for 
# Source directory: /home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel
# Build directory: /home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(modular_tests "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/build/bdi_modular_test")
set_tests_properties(modular_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/CMakeLists.txt;183;add_test;/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/CMakeLists.txt;0;")
subdirs("master_memory_manager")
