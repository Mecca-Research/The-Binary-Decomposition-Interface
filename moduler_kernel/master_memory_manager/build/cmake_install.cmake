# Install script for directory: /home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
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

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/build/libmaster_memory_manager.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/master_memory_manager" TYPE FILE FILES
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/master_memory_manager.h"
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/x86_core/registers/x86_registers.h"
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/x86_core/calling_abi/x86_calling_abi.h"
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/x86_core/paging_mmu/x86_paging_mmu.h"
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/x86_core/tlb_mgmt/x86_tlb_mgmt.h"
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/x86_core/cache_hints/x86_cache_hints.h"
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/hal_framework/bsp/mmm_bsp.h"
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/hal_framework/peripheral_drivers/mmm_peripheral_drivers.h"
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/hal_framework/hardware_access/mmm_hardware_access.h"
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/system_integration/interrupt_mgmt/mmm_interrupt_mgmt.h"
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/system_integration/memory_protection/mmm_memory_protection.h"
    "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/system_integration/performance/mmm_performance.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/master_memory_manager" TYPE FILE FILES "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/README.md")
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/ubuntu/github_repos/The-Binary-Decomposition-Interface/moduler_kernel/master_memory_manager/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
