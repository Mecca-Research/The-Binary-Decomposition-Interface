#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../../vm/vm.h"
#include "../../vm/gc/generational_gc.h"

int main(void) {
    printf("Testing object promotion...\n");
    
    EnhancedVM* vm = enhanced_vm_create_with_sizes(10 * 1024, 100 * 1024);
    
    // Allocate object and keep it alive
    GenObject* obj = (GenObject*)vm_alloc(vm, 100);
    printf("Initial: obj=%p, generation=%d, age=%u\n", 
           (void*)obj, obj->header.generation, obj->header.age);
    
    vm_register_root(vm, (GCObject**)&obj);
    
    // Trigger multiple GCs to age the object
    for (int i = 0; i < 5; i++) {
        // Allocate some garbage to fill nursery
        for (int j = 0; j < 10; j++) {
            vm_alloc(vm, 500);
        }
        
        printf("Before GC %d: obj=%p, generation=%d, age=%u\n", 
               i+1, (void*)obj, obj->header.generation, obj->header.age);
        
        vm_gc_collect(vm);
        
        printf("After GC %d: obj=%p, generation=%d, age=%u\n", 
               i+1, (void*)obj, obj->header.generation, obj->header.age);
    }
    
    printf("Final: obj=%p, generation=%d, age=%u\n", 
           (void*)obj, obj->header.generation, obj->header.age);
    
    if (obj->header.generation == GEN_OLD) {
        printf("SUCCESS: Object promoted to old generation\n");
    } else {
        printf("FAILED: Object still in generation %d\n", obj->header.generation);
    }
    
    enhanced_vm_destroy(vm);
    return 0;
}
