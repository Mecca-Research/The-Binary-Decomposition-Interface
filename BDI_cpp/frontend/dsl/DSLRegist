#include "DSLRegistry.hpp" 
#include <iostream> // For warnings/debug 
namespace chimera::frontend::dsl { 
bool DSLRegistry::registerDSL(std::shared_ptr<DSLSpecification> dsl_spec, std::shared_ptr<IDSLMapper> mapper) { 
if (!dsl_spec || !mapper) { 
return false; 
    }
 std::unique_lock lock(registry_mutex_); // Exclusive lock for writing 
const std::string& name = dsl_spec->name;
 if (specifications_.count(name)) { 
std::cerr << "DSLRegistry Warning: DSL '" << name << "' already registered. Overwriting." << std::endl; 
// Or return false? Depends on policy. 
    }
    specifications_[name] = std::move(dsl_spec); 
    mappers_[name] = std::move(mapper); 
std::cout << "DSLRegistry: Registered DSL '" << name << "'." << std::endl; 
return true; 
} 
std::shared_ptr<const DSLSpecification> DSLRegistry::getDSLSpecification(const std::string& dsl_name) const { 
std::shared_lock lock(registry_mutex_); // Shared lock for reading 
auto it = specifications_.find(dsl_name);
if (it != specifications_.end()) { 
return it->second; 
    }
 return nullptr; 
std::shared_ptr<IDSLMapper> DSLRegistry::getDSLMapper(const std::string& dsl_name) const {
 std::shared_lock lock(registry_mutex_); // Shared lock for reading 
auto it = mappers_.find(dsl_name); 
if (it != mappers_.end()) { 
return it->second; 
    }
 return nullptr; 
} 
bool DSLRegistry::isRegistered(const std::string& dsl_name) const { 
std::shared_lock lock(registry_mutex_); 
return specifications_.count(dsl_name); 
} 
} // namespace chimera::frontend::dsl 
