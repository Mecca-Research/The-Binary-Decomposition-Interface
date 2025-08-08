 #include "MemoryManager.hpp"
 #include <stdexcept> // For invalid_argument
 #include <cstring> // For memcpy
 #include <iostream> // For debug/warnings
 #include <algorithm> // For std::find_if, std::sort (if list not kept sorted)
 namespace bdi::runtime {
 MemoryManager::MemoryManager(size_t total_memory_bytes)
    : memory_block_(total_memory_bytes), next_region_id_(1), next_allocation_offset_(0)
 {
    if (total_memory_bytes == 0) { /* ... error ... */ }
    initializeFreeList();
        throw std::invalid_argument("MemoryManager size cannot be zero.");
    std::cout << "MemoryManager: Initialized with " << total_memory_bytes << " bytes." << std::endl;
 }
 void MemoryManager::initializeFreeList() {
    free_list_.clear();
    // Start with one large free block spanning the entire memory
    if (!memory_block_.empty()) {
        free_list_.push_back({0, memory_block_.size()});
    }
    // allocated_block_sizes_.clear();
 }
 std::optional<RegionID> MemoryManager::allocateRegion(size_t size_bytes, bool read_only) {
    // Very simple bump allocator - DOES NOT HANDLE FREEING PROPERLY YET
    // std::lock_guard<std::mutex> lock(memory_mutex_); // If thread-safe
    // Find first free block large enough (First Fit)
     auto it = std::find_if(free_list_.begin(), free_list_.end(),
                           [size_bytes](const FreeBlock& block){ return block.size >= size_bytes; });
     if (it == free_list_.end()) {
         std::cerr << "MemoryManager Error: Out of memory (Free List) trying to allocate " << size_bytes << " bytes." << std::endl;
         return std::nullopt; // No suitable block found
     }
     // Allocate from the found block
     uintptr_t allocated_address = it->address;
     RegionID new_id = next_region_id_++;
     // If block is exactly the right size, remove it from free list
     if (it->size == size_bytes) {
         free_list_.erase(it);
     }
     // If block is larger, resize it and update its address
     else {
         it->address += size_bytes;
         it->size -= size_bytes;
     }
     // Record the allocation
     allocated_regions_.emplace(new_id, MemoryRegion(new_id, allocated_address, size_bytes, read_only));
     // allocated_block_sizes_[allocated_address] = size_bytes; // Track size for freeing
     std::cout << "MemoryManager: Allocated Region " << new_id << " (" << size_bytes << " bytes) at address " << allocated_address << " (Free List)." <<
 std::endl;
     return new_id;
 }
    if (next_allocation_offset_ + size_bytes > memory_block_.size()) {
        std::cerr << "MemoryManager Error: Out of memory trying to allocate " << size_bytes << " bytes." << std::endl;
        return std::nullopt; // Out of memory
    }
    RegionID new_id = next_region_id_++;
    uintptr_t base_address = next_allocation_offset_;
    next_allocation_offset_ += size_bytes;
    allocated_regions_.emplace(new_id, MemoryRegion(new_id, base_address, size_bytes, read_only));
    std::cout << "MemoryManager: Allocated Region " << new_id << " (" << size_bytes << " bytes) at address " << base_address << std::endl;
    return new_id;
 }
 bool MemoryManager::freeRegion(RegionID region_id) {
    // STUB: Freeing requires a more complex allocator than bump allocation
    // std::lock_guard<std::mutex> lock(memory_mutex_); // If thread-safe
auto region it = allocated_regions_.find(region_id);
    if (region_it = allocated_regions_.end()) {
        std::cout << "MemoryManager: Freeing Region " << region_id << " (Allocator Stub - No actual free)" << std::endl;
        // With a real allocator, mark space as free here
        allocated_regions_.erase(it); // Remove tracking info
        return true;
    }
    std::cerr << "MemoryManager Error: Cannot free non-existent region " << region_id << std::endl;
    return false;
 }
 std::optional<MemoryRegion> MemoryManager::getRegionInfo(RegionID region_id) const {
    // std::lock_guard<std::mutex> lock(memory_mutex_); // If thread-safe
    auto it = allocated_regions_.find(region_id);
    if (it != allocated_regions_.end()) {
        return it->second;
    }
    return std::nullopt;
 }
 bool MemoryManager::readMemory(uintptr_t address, std::byte* buffer, size_t size_bytes) const {
    // std::lock_guard<std::mutex> lock(memory_mutex_); // If thread-safe
    if (address + size_bytes > memory_block_.size()) {
         std::cerr << "MemoryManager Error: Read access out of bounds (Address: " << address << ", Size: " << size_bytes << ")" << std::endl;
        return false; // Out of bounds
    }
    // TODO: Check if read overlaps with valid allocated regions? (More complex check)
    std::memcpy(buffer, memory_block_.data() + address, size_bytes);
    return true;
 }
 bool MemoryManager::writeMemory(uintptr_t address, const std::byte* buffer, size_t size_bytes) {
    // std::lock_guard<std::mutex> lock(memory_mutex_); // If thread-safe
    if (address + size_bytes > memory_block_.size()) {
        std::cerr << "MemoryManager Error: Write access out of bounds (Address: " << address << ", Size: " << size_bytes << ")" << std::endl;
        return false; // Out of bounds
    }
    uintptr_t address_to_free = region_it->second.base_address;
    size_t size_to_free = region_it->second.size;
    allocated_regions_.erase(region_it); // Remove from allocated map
    // Find where to insert the freed block into the sorted free list
    auto insert_pos = std::lower_bound(free_list_.begin(), free_list_.end(),
                                      FreeBlock{address_to_free, 0}); // Find position based on address
    free_list_.insert(insert_pos, {address_to_free, size_to_free});
    std::cout << "MemoryManager: Freed Region " << region_id << " (" << size_to_free << " bytes) at address " << address_to_free << std::endl;
    // Merge adjacent free blocks
    mergeFreeBlocks();
    return true;
 }
 void MemoryManager::mergeFreeBlocks() {
    // Assumes free_list_ is sorted by address
    if (free_list_.size() < 2) return;
    auto it = free_list_.begin();
    auto next_it = std::next(it);
    while (next_it != free_list_.end()) {
        // Check if current block + its size = next block's address
        if (it->address + it->size == next_it->address) {
            // Merge next_it into it
            it->size += next_it->size;
            // Erase next_it and update next_it for the next iteration
            next_it = free_list_.erase(next_it);
            // Don't advance 'it' - check if the newly merged block can merge again
        } else {
            // No merge possible, advance both iterators
            ++it;
            ++next_it;
        }
    }
 }
 size_t MemoryManager::getUsedSize() const {
     // Calculate used size by subtracting total free size from total size
     size_t free_size = 0;
     for(const auto& block : free_list_) {
         free_size += block.size;
     }
     return memory_block_.size() - free_size;
 }
    // TODO: Check if write overlaps with valid allocated regions?
    // TODO: Check read-only flags of overlapping regions?
    // This simple implementation doesn't check region permissions.
    std::memcpy(memory_block_.data() + address, buffer, size_bytes);
    return true;
 }
 // --- Raw Pointer Access (Use with Caution) --
std::byte* MemoryManager::getRawPointer(uintptr_t address) {
     if (address >= memory_block_.size()) return nullptr;
     return memory_block_.data() + address;
 }
 const std::byte* MemoryManager::getRawPointer(uintptr_t address) const {
      if (address >= memory_block_.size()) return nullptr;
     return memory_block_.data() + address;
 }
 size_t MemoryManager::getUsedSize() const {
    // For bump allocator, this is just the next offset
    return next_allocation_offset_;
 }
 } // namespace bdi::runtime
