#ifndef BDI_HAL_HARDWAREINFO_HPP 
#define BDI_HAL_HARDWAREINFO_HPP 
#include <cstdint> 
#include <vector> 
#include <string> 
namespace bdi::hal { 
/** @brief Describes a contiguous physical memory region available to the system. */ 
struct PhysicalMemoryRegion { 
uint64_t base_address;      ///< Start physical address. 
uint64_t size_bytes;        ///< Size of the region in bytes. 
enum class Type { RAM, MMIO, RESERVED, ACPI } type; ///< Type of memory. 
// Add flags? e.g., Cacheable, WriteBack/WriteThrough 
}; 
/** @brief Describes a detected hardware device (simplified). */ 
struct DeviceInfo { 
enum class Type { UNKNOWN, TIMER, INTERRUPT_CONTROLLER, SERIAL_PORT, DISK, NETWORK, GPU } type; 
std::string name;           ///< Human-readable name.          
uint64_t mmio_address;      ///< Base physical address for MMIO registers (if applicable). 
uint64_t mmio_size;         ///< Size of MMIO region. 
uint32_t interrupt_vector;  ///< Hardware interrupt number (if applicable). 
// Add bus info (PCI ID, etc.) 
}; 
/** 
 * @brief Structure passed by the Bootloader/Firmware to the BDIVM. 
 * Contains essential information about the hardware environment. 
 * Assumed to be located at a well-known physical address or passed via register. 
 */ 
struct FirmwareInfoBlock { 
uint32_t signature; // e.g., 'BDIH' 
uint16_t version; 
uint16_t flags; 
uint64_t bdivm_load_address;    ///< Physical address where BDIVM code is loaded. 
uint64_t bdivm_size; 
uint64_t genesis_graph_address; ///< Physical address of the serialized Genesis BDI Graph. 
uint64_t genesis_graph_size; 
uint64_t initial_stack_address; ///< Physical address for the initial stack top/base. 
uint64_t initial_stack_size; 
uint32_t memory_region_count;   ///< Number of entries in the memory map array. 
uint64_t memory_map_address;    ///< Physical address of the PhysicalMemoryRegion array. 
uint32_t device_info_count;     ///< Number of entries in the device info array. 
uint64_t device_info_address;   ///< Physical address of the DeviceInfo array. 
// Add pointers/offsets to other firmware tables (ACPI, SMBIOS) if needed. 
// Add CPU info (core count, features)? 
}; 
// Expected signature 
constexpr uint32_t FIRMWARE_INFO_SIGNATURE = 0x42444948; // 'BDIH' 
} // namespace bdi::hal 
#endif // BDI_HAL_HARDWAREINFO_HPP
