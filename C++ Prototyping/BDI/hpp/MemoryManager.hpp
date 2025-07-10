 #ifndef BDI_RUNTIME_MEMORYMANAGER_HPP
 #define BDI_RUNTIME_MEMORYMANAGER_HPP
 #include "MemoryRegion.hpp"
 #include <vector>
 #include <cstddef> // For std::byte
 #include <cstdint>
 #include <unordered_map>
 #include <optional>
 #include <mutex> // For basic thread safety if needed later
 #include <list> // For free list
 #include <map>  // For tracking allocated block sizes maybe
 namespace bdi::runtime {
 class MemoryManager {
 public:
    // Initialize with a total simulated memory size
    explicit MemoryManager(size_t total_memory_bytes);
    // Allocate a new memory region
    std::optional<RegionID> allocateRegion(size_t size_bytes, bool read_only = false);
    // Free a previously allocated region
    bool freeRegion(RegionID region_id);
    // Get region info
    std::optional<MemoryRegion> getRegionInfo(RegionID region_id) const;
    // Read data from memory
    // address is relative to the start of the MemoryManager's space
    bool readMemory(uintptr_t address, std::byte* buffer, size_t size_bytes) const;
    // Write data to memory
    // address is relative to the start of the MemoryManager's space
    bool writeMemory(uintptr_t address, const std::byte* buffer, size_t size_bytes);
    // Helper to get raw pointer (use with extreme caution!)
    std::byte* getRawPointer(uintptr_t address);
    const std::byte* getRawPointer(uintptr_t address) const;
    size_t getTotalSize() const { return memory_block_.size(); }
    size_t getUsedSize() const; // Needs allocator implementation
 private:
    std::vector<std::byte> memory_block_; // The simulated memory space
    std::unordered_map<RegionID, MemoryRegion> allocated_regions_;
    RegionID next_region_id_;
    // --- Basic Allocator State --
    // Very simple bump allocator for this example
    // ... memory_block_, allocated_regions_, next_region_id_ ...
    // --- Free List Allocator State --
    struct FreeBlock {
        uintptr_t address;
        size_t size;
        // Overload operator< for sorting/merging if needed
        bool operator<(const FreeBlock& other) const { return address < other.address; }
    };
    std::list<FreeBlock> free_list_; // List of available blocks, kept sorted by address
    // std::map<uintptr_t, size_t> allocated_block_sizes_; // Track allocated block sizes for freeing
    // Helper methods for allocator logic
    void initializeFreeList();
    void mergeFreeBlocks(); // Merge adjacent free blocks
    // Remove bump allocator state
    uintptr_t next_allocation_offset_;
    // TODO: Implement a more robust allocator (e.g., free list, buddy system) for freeRegion to work properly
    // Mutex for thread safety if the VM becomes multi-threaded
    // mutable std::mutex memory_mutex_;
 };
 } // namespace bdi::runtime
 #endif // BDI_RUNTIME_MEMORYMANAGER_HPP
