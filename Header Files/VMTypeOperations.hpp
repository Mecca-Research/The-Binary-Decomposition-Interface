 #include "BDIValueVariant.hpp"
 #include "../core/types/TypeSystem.hpp"
 #include "../core/payload/MapCppTypeToBdiType.hpp" // Include mapping
 #include <optional>
 #include <cmath>
 #include <limits>
 #include <stdexcept>
 #include <iostream> // For errors
 namespace bdi::runtime::vm_ops {
 // --- Conversion Helper --
template <typename TargetType>
 std::optional<TargetType> convertValue(const BDIValueVariant& value_var) {
    // ... (Implementation of convertVariantTo from previous step) ...
    // Should handle standard numeric conversions, potentially bool<->int
     using Type = core::types::BDIType;
     constexpr Type target_bdi_type = core::payload::MapCppTypeToBdiType<TargetType>::value;
     TargetType result;
     bool success = false;
     std::visit([&](auto&& arg) {
         using SourceCppType = std::decay_t<decltype(arg)>;
         constexpr Type source_bdi_type = core::payload::MapCppTypeToBdiType<SourceCppType>::value;
         if constexpr (std::is_same_v<SourceCppType, TargetType>) {
             result = arg; success = true;
         } else if constexpr (std::is_convertible_v<SourceCppType, TargetType>) {
              // Check BDI type system rules before C++ conversion
              if (core::types::TypeSystem::canImplicitlyConvert(source_bdi_type, target_bdi_type)) {
                  // WARNING: Need explicit checks for narrowing conversions / precision loss / overflow
                  if constexpr (std::is_floating_point_v<TargetType> && std::is_integral_v<SourceCppType>) {
                      result = static_cast<TargetType>(arg); // Int -> Float
                      success = true;
                  } else if constexpr (std::is_integral_v<TargetType> && std::is_floating_point_v<SourceCppType>) {
                       // Float -> Int (Truncation) - check range?
                      // if (arg < static_cast<SourceCppType>(std::numeric_limits<TargetType>::min()) || arg > static_cast<SourceCppType>
 (std::numeric_limits<TargetType>::max())) { // Range check }
                       result = static_cast<TargetType>(arg);
                       success = true;
                  } else if constexpr (std::is_integral_v<TargetType> && std::is_integral_v<SourceCppType>) {
                       // Int -> Int - check range for narrowing?
                       result = static_cast<TargetType>(arg);
                       success = true;
                  } else if constexpr (std::is_floating_point_v<TargetType> && std::is_floating_point_v<SourceCppType>) {
                       // Float -> Float
                       result = static_cast<TargetType>(arg);
                       success = true;
                  }
                   // Add Bool <-> Int etc.
             }
         }
     }, value_var);
     if (success) return result;
     return std::nullopt;
 }
 // --- Operation Implementations using Variants --
inline BDIValueVariant performAddition(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var) {
    BDIType type1 = getBDIType(lhs_var);
    BDIType type2 = getBDIType(rhs_var);
    BDIType result_type = core::types::TypeSystem::getPromotedType(type1, type2);
    // Perform based on promoted type
    if (result_type == BDIType::INT32) {
        auto v1 = convertValue<int32_t>(lhs_var); auto v2 = convertValue<int32_t>(rhs_var);
        if (v1 && v2) return *v1 + *v2;
    } else if (result_type == BDIType::INT64) {
         auto v1 = convertValue<int64_t>(lhs_var); auto v2 = convertValue<int64_t>(rhs_var);
         if (v1 && v2) return *v1 + *v2;
    } else if (result_type == BDIType::FLOAT32) {
        auto v1 = convertValue<float>(lhs_var); auto v2 = convertValue<float>(rhs_var);
        if (v1 && v2) return *v1 + *v2;
    } else if (result_type == BDIType::FLOAT64) {
         auto v1 = convertValue<double>(lhs_var); auto v2 = convertValue<double>(rhs_var);
         if (v1 && v2) return *v1 + *v2;
    }
    // Add UINT cases...
    std::cerr << "VM Op Error: Addition failed for types " << core::types::bdiTypeToString(type1) << ", " << core::types::bdiTypeToString(type2) <<
 std::endl;
    return std::monostate{}; // Error
 }
 // Implement performSubtraction, performMultiplication, performDivision (with zero check), etc. similarly
 inline BDIValueVariant performComparisonEQ(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var) {
     BDIType type1 = getBDIType(lhs_var);
     BDIType type2 = getBDIType(rhs_var);
     BDIType promoted_type = core::types::TypeSystem::getPromotedType(type1, type2);
      if (promoted_type == BDIType::UNKNOWN) return std::monostate{}; // Cannot compare
      // Perform comparison based on promoted type
      if (promoted_type == BDIType::INT32) {
          auto v1 = convertValue<int32_t>(lhs_var); auto v2 = convertValue<int32_t>(rhs_var);
          if (v1 && v2) return bool(*v1 == *v2);
      } // ... other numeric types ...
      else if (type1 == BDIType::BOOL && type2 == BDIType::BOOL) { // Explicit bool comparison
          auto v1 = convertValue<bool>(lhs_var); auto v2 = convertValue<bool>(rhs_var);
           if (v1 && v2) return bool(*v1 == *v2);
      }
      // Add pointer comparison?
     return std::monostate{}; // Error / comparison failed
 }
 // Implement performComparisonLT, performComparisonGT, etc.
 // Implement performBitwiseAND, performBitwiseOR, etc. (ensure integer types)
 // Implement performConversion (handles CONV_ ops)
 inline BDIValueVariant performConversion(const BDIValueVariant& input_var, BDIType target_bdi_type) {
     // Use convertValue template based on target_bdi_type
     if (target_bdi_type == BDIType::INT32)   { auto v = convertValue<int32_t>(input_var); return v ? BDIValueVariant{*v} : std::monostate{}; }
     if (target_bdi_type == BDIType::FLOAT32) { auto v = convertValue<float>(input_var);   return v ? BDIValueVariant{*v} : std::monostate{}; }
     // Add all target types...
     return std::monostate{};
 }
 } // namespace bdi::runtime::vm_ops
 #endif // BDI_RUNTIME_VMTYPEOPERATIONS_HPP
