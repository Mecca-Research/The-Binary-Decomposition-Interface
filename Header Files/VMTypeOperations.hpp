 #ifndef BDI_RUNTIME_VMTYPEOPERATIONS_HPP
 #define BDI_RUNTIME_VMTYPEOPERATIONS_HPP
 #include "BDIValueVariant.hpp"
 #include "../core/types/TypeSystem.hpp"
 #include "../core/payload/MapCppTypeToBdiType.hpp" // Include mapping
 #include <optional>
 #include <cmath>
 #include <limits>
 #include <type_traits> // For is_integral etc.
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
         } else if constexpr (std::is_convertible_v<SourceCppType, TargetType, std::monostate>) {
             // Check BDI type system rules before C++ conversion
             // Check explicit BDI type system rule (more reliable than is_convertible)
             // This check might be redundant if caller already checked promotion,
             // but good for safety.
              if (core::types::TypeSystem::canImplicitlyConvert(source_bdi_type, target_bdi_type)) {
                  // WARNING: Need explicit checks for narrowing conversions / precision loss / overflow
                  // Perform standard C++ conversion, WATCH FOR NARROWING/OVERFLOW
                  // TODO: Add checks for potential data loss/undefined behavior!
                  // Example: Check if integer fits in target before casting float->int
                  if constexpr (std::is_floating_point_v<TargetType> && std::is_integral_v<SourceCppType>) {
                      result = static_cast<TargetType>(arg); // Int -> Float
                      success = true;
                  } else if constexpr (std::is_integral_v<TargetType> && std::is_floating_point_v<SourceCppType>) {
                      if (arg < static_cast<SourceCppType>(std::numeric_limits<TargetType>::min()) || arg > static_cast<SourceCppType>
                      // Float -> Int (Truncation) - check range?
                      // if (arg < static_cast<SourceCppType>(std::numeric_limits<TargetType>::min()) || arg > static_cast<SourceCppType>
 (std::numeric_limits<TargetType>::max())) { // Range check }
                       std::cerr << "VM Convert Warning: Float->Int overflow/underflow detected." << std::endl;
                       result = static_cast<TargetType>(arg);
                       success = true;
                       // Decide on behavior: saturate, wrap (for unsigned?), error out? For now, allow standard cast.
                       }
                  } else if constexpr (std::is_integral_v<TargetType> && std::is_integral_v<SourceCppType>&& sizeof(SourceCppType) > sizeof(TargetType))
 {
                       // Int -> Int - check range for narrowing?
                       result = static_cast<TargetType>(arg);
                       success = true;
                  } else if constexpr (std::is_floating_point_v<TargetType> && std::is_floating_point_v<SourceCppType>) {
                       // Float -> Float
                       // Check for narrowing integer conversion?
                  }
                 result = static_cast<TargetType>(arg);
                 success = true;
             } else {
                  std::cerr << "VM Convert Error: Implicit conversion not allowed by TypeSystem from "
                            << core::types::bdiTypeToString(source_bdi_type) << " to "
                            << core::types::bdiTypeToString(target_bdi_type) << std::endl;
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
 // Template helper for binary numeric/bitwise operations
 template <typename Operation>
 BDIValueVariant performNumericBitwiseBinaryOp(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var, Operation op_lambda) {
    BDIType type1 = getBDIType(lhs_var);
    BDIType type2 = getBDIType(rhs_var);
    BDIType result_type = core::types::TypeSystem::getPromotedType(type1, type2);
    if (result_type == BDIType::UNKNOWN) return std::monostate{};
    // Use if constexpr to dispatch based on promoted type
    if (result_type == BDIType::INT32) {
        auto v1 = convertValue<int32_t>(lhs_var); auto v2 = convertValue<int32_t>(rhs_var);
        if (v1 && v2) return op_lambda(*v1, *v2);
    } else if (result_type == BDIType::INT64) {
        auto v1 = convertValue<int64_t>(lhs_var); auto v2 = convertValue<int64_t>(rhs_var);
        if (v1 && v2) return op_lambda(*v1, *v2);
    } else if (result_type == BDIType::UINT32) {
         auto v1 = convertValue<uint32_t>(lhs_var); auto v2 = convertValue<uint32_t>(rhs_var);
         if (v1 && v2) return op_lambda(*v1, *v2);
    } else if (result_type == BDIType::UINT64) {
         auto v1 = convertValue<uint64_t>(lhs_var); auto v2 = convertValue<uint64_t>(rhs_var);
         if (v1 && v2) return op_lambda(*v1, *v2);
    } else if (result_type == BDIType::FLOAT32) {
        auto v1 = convertValue<float>(lhs_var); auto v2 = convertValue<float>(rhs_var);
        if (v1 && v2) return op_lambda(*v1, *v2);
    } else if (result_type == BDIType::FLOAT64) {
        auto v1 = convertValue<double>(lhs_var); auto v2 = convertValue<double>(rhs_var);
        if (v1 && v2) return op_lambda(*v1, *v2);
    }
     // Add UINT8/16, INT8/16 if needed
    return std::monostate{}; // Type conversion failed or unhandled promotion
 }
 // Template helper for unary numeric/bitwise operations
 template <typename Operation>
 BDIValueVariant performNumericBitwiseUnaryOp(const BDIValueVariant& var, Operation op_lambda) {
     BDIType type = getBDIType(var);
     if (!core::types::TypeSystem::isNumeric(type) && !core::types::TypeSystem::isInteger(type)) // Adjust check based on op
        return std::monostate{};
     if (type == BDIType::INT32) {
         auto v = convertValue<int32_t>(var); if (v) return op_lambda(*v);
     } else if (type == BDIType::INT64) {
         auto v = convertValue<int64_t>(var); if (v) return op_lambda(*v);
     } // Add other types...
     return std::monostate{};
 }
 // Implement specific ops using the helpers
 inline BDIValueVariant performAddition(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var) {
    return performNumericBitwiseBinaryOp(lhs_var, rhs_var, [](auto a, auto b){ return a + b; });
 }
 inline BDIValueVariant performSubtraction(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var) {
     return performNumericBitwiseBinaryOp(lhs_var, rhs_var, [](auto a, auto b){ return a - b; });
 }
 inline BDIValueVariant performMultiplication(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var) {
    return performNumericBitwiseBinaryOp(lhs_var, rhs_var, [](auto a, auto b){ return a * b; });
 }
 inline BDIValueVariant performDivision(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var) {
     return performNumericBitwiseBinaryOp(lhs_var, rhs_var, [](auto a, auto b){
         if (b == static_cast<decltype(b)>(0)) throw std::runtime_error("Division by zero");
         return a / b; });
 }
 // ... Implement Modulo (ensure integer), Negation, Abs ...
 // Bitwise (Needs integer check within lambda or separate helper)
 inline BDIValueVariant performBitwiseAND(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var) {
    return performNumericBitwiseBinaryOp(lhs_var, rhs_var, [](auto a, auto b){
        if constexpr (std::is_integral_v<decltype(a)>) return a & b; else throw std::runtime_error("BIT_AND requires integers"); });
 }
 // ... Implement OR, XOR, NOT, Shifts ...
 // Comparison (returns bool variant)
 template <typename Operation>
 BDIValueVariant performComparison(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var, Operation op_lambda) {
     // Similar promotion logic as numeric binary ops
     BDIType type1 = getBDIType(lhs_var);
     BDIType type2 = getBDIType(rhs_var);
     BDIType result_type = core::types::TypeSystem::getPromotedType(type1, type2);
     if (result_type == BDIType::UNKNOWN) return std::monostate{};
     if (result_type == BDIType::INT32) {
         auto v1 = convertValue<int32_t>(lhs_var); auto v2 = convertValue<int32_t>(rhs_var);
         if (v1 && v2) return bool(op_lambda(*v1, *v2));
     } // ... other numeric types ...
     else if (type1 == BDIType::BOOL && type2 == BDIType::BOOL) {
         auto v1 = convertValue<bool>(lhs_var); auto v2 = convertValue<bool>(rhs_var);
         if (v1 && v2) return bool(op_lambda(*v1, *v2)); // Need == or != lambda here
     }
      return std::monostate{};
 }
 inline BDIValueVariant performComparisonEQ(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var) { return performComparison(lhs_var,
 rhs_var, [](auto a, auto b){ return a == b; }); }
 inline BDIValueVariant performComparisonLT(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var) { return performComparison(lhs_var,
 rhs_var, [](auto a, auto b){ return a < b; }); }
 // ... other comparison helpers ...
 // Logical (Expect bool inputs)
 inline BDIValueVariant performLogicalAND(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var) {
    auto v1 = convertValue<bool>(lhs_var); auto v2 = convertValue<bool>(rhs_var);
    if (v1 && v2) return bool(*v1 && *v2);
    return std::monostate{};
 }
 inline BDIValueVariant performLogicalNOT(const BDIValueVariant& var) {
     auto v = convertValue<bool>(var);
     if (v) return bool(!*v);
     return std::monostate{};
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
