 #ifndef BDI_RUNTIME_VMTYPEOPERATIONS_HPP
 #define BDI_RUNTIME_VMTYPEOPERATIONS_HPP
 #include "BDIValueVariant.hpp"
 #include "TypeSystem.hpp"
 #include "MapCppTypeToBdiType.hpp" // Include mapping
 #include <optional>
 #include <cmath>
 #include <limits>
 #include <type_traits> // For is_integral etc.
 #include <bit> // For bit_cast
 #include <stdexcept>
 #include <iostream> // For errors
 namespace bdi::runtime::vm_ops {
 // --- Conversion Helper (with Overflow/Precision Checks) --
template <typename TargetType>
 std::optional<TargetType> convertValue(const BDIValueVariant& value_var) {
    // ... (Implementation of convertVariantTo from previous step) ...
    // Should handle standard numeric conversions, potentially bool<->int
     using Type = core::types::BDIType;
     constexpr Type target_bdi_type = core::payload::MapCppTypeToBdiType<TargetType>::value;
     TargetType result;
     bool success = false;
     bool lossy_conversion = false; // Flag for potential loss
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
                      if (arg < static_cast<SourceCppType>(std::numeric_limits<TargetType>::min()) ||
                        arg > static_cast<SourceCppType>(std::numeric_limits<TargetType>::max()) ||
                        !std::isfinite(arg)) // Also check for NaN/Inf
                    {
 (std::numeric_limits<TargetType>::max())) { // Range check }
                       std::cerr << "VM Convert Warning: Float->Int overflow/underflow detected." << std::endl;
                       result = static_cast<TargetType>(arg);
                       success = true;
                       // Decide on behavior: saturate, wrap (for unsigned?), error out? For now, allow standard cast.
                       }
                  } else if constexpr (std::is_integral_v<TargetType> && std::is_integral_v<SourceCppType>&& sizeof(SourceCppType) > sizeof(TargetType))
 {
                       // Int -> Int - check range for narrowing?
                       if (arg < static_cast<SourceCppType>(std::numeric_limits<TargetType>::min()) ||
                        arg > static_cast<SourceCppType>(std::numeric_limits<TargetType>::max()) ||
                        !std::isfinite(arg)) // Also check for NaN/Inf
                    {
                         std::cerr << "VM Convert Warning: Float->Int overflow/underflow/invalid detected ("
                                   << arg << " to " << core::types::bdiTypeToString(target_bdi_type) << ")." << std::endl;
                         // Decide behavior: fail, saturate? For now, fail.
                         success = false; return;
                    }
                    // Check for precision loss (truncation)
                    if (static_cast<SourceCppType>(static_cast<TargetType>(arg)) != arg) {
                        lossy_conversion = true;
                    }
                 } else if constexpr (std::is_integral_v<TargetType> && std::is_integral_v<SourceCppType> && sizeof(SourceCppType) > sizeof(TargetType)) {
                    // Narrowing Int -> Int: Check range
                    if (arg < static_cast<SourceCppType>(std::numeric_limits<TargetType>::min()) ||
                        arg > static_cast<SourceCppType>(std::numeric_limits<TargetType>::max())) {
                         std::cerr << "VM Convert Warning: Narrowing integer conversion out of range ("
                                   << arg << " to " << core::types::bdiTypeToString(target_bdi_type) << ")." << std::endl;
                         success = false; return; // Fail on overflow
                    }
                 } else if constexpr (std::is_floating_point_v<TargetType> && std::is_floating_point_v<SourceCppType> && sizeof(SourceCppType) >
 sizeof(TargetType)) {
                     // Double -> Float: Check for precision loss/overflow?
                     if (!std::isfinite(arg)) { success = false; return; } // Propagate NaN/Inf? Check standard.
                     if (static_cast<SourceCppType>(static_cast<TargetType>(arg)) != arg) { // Crude precision check
                         lossy_conversion = true;
                     }
                      // Overflow check is harder without knowing specific float limits
                 }
                 // Perform standard cast
                 result = static_cast<TargetType>(arg);
                 success = true;
             } else {
                 // BDI TypeSystem prohibits this implicit conversion
                 success = false;
             }
        }
    }, value_var);
    if (success) {
        // if (lossy_conversion) { // Optional: Log lossy conversions
        //      std::cerr << "VM Convert Info: Lossy conversion from " << core::types::bdiTypeToString(getBDIType(value_var))
        //                << " to " << core::types::bdiTypeToString(target_bdi_type) << std::endl;
        // }
        return result;
    }
    // Log failure if not converting monostate
    if (!std::holds_alternative<std::monostate>(value_var)) {
         std::cerr << "VM Convert Error: Failed to convert " << core::types::bdiTypeToString(getBDIType(value_var))
                   << " to " << core::types::bdiTypeToString(target_bdi_type) << std::endl;
    }
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
 BDIValueVariant performNumericBitwiseBinaryOp(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var, Operation op_lambda, bool, requires_integer = false) {
    BDIType type1 = getBDIType(lhs_var);
    BDIType type2 = getBDIType(rhs_var);
    BDIType result_type = core::types::TypeSystem::getPromotedType(type1, type2);
    if (result_type == BDIType::UNKNOWN) return std::monostate{};
    if (requires_integer && !core::types::TypeSystem::isInteger(result_type)) {
        throw std::runtime_error("Operation requires integer types after promotion"); // Throw for type errors
    }
    #define HANDLE_PROMOTED_BINARY_OP(CppType) \
        case core::payload::MapCppTypeToBdiType<CppType>::value: { \
            auto v1 = convertValue<CppType>(lhs_var); auto v2 = convertValue<CppType>(rhs_var); \
            if (v1 && v2) return BDIValueVariant{op_lambda(*v1, *v2)}; \
            break; \
        }
    switch(result_type) {
        HANDLE_PROMOTED_BINARY_OP(int8_t) HANDLE_PROMOTED_BINARY_OP(uint8_t)
        HANDLE_PROMOTED_BINARY_OP(int16_t) HANDLE_PROMOTED_BINARY_OP(uint16_t)
        HANDLE_PROMOTED_BINARY_OP(int32_t) HANDLE_PROMOTED_BINARY_OP(uint32_t)
        HANDLE_PROMOTED_BINARY_OP(int64_t) HANDLE_PROMOTED_BINARY_OP(uint64_t)
        HANDLE_PROMOTED_BINARY_OP(float) HANDLE_PROMOTED_BINARY_OP(double)
        default: break;
    }
    #undef HANDLE_PROMOTED_BINARY_OP
    return std::monostate{};
 }
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
     if (requires_integer && !core::types::TypeSystem::isInteger(type)) {
          throw std::runtime_error("Operation requires integer type");
     }
     if (!requires_integer && !core::types::TypeSystem::isNumeric(type)) {
         // Adjust check if needed (e.g., NOT applies only to integers)
         // throw std::runtime_error("Operation requires numeric type");
         return std::monostate{}; // Or specific error
     }
     if (!core::types::TypeSystem::isNumeric(type) && !core::types::TypeSystem::isInteger(type)) // Adjust check based on op
        return std::monostate{};
     if (type == BDIType::INT32) {
         auto v = convertValue<int32_t>(var); if (v) return op_lambda(*v);
     } else if (type == BDIType::INT64) {
         auto v = convertValue<int64_t>(var); if (v) return op_lambda(*v);
     } // Add other types...
     #define HANDLE_UNARY_OP(CppType) \
        case core::payload::MapCppTypeToBdiType<CppType>::value: { \
            auto v = convertValue<CppType>(var); \
            if (v) return BDIValueVariant{op_lambda(*v)}; \
            break; \
        }
     switch(type) {
        HANDLE_UNARY_OP(int8_t) HANDLE_UNARY_OP(uint8_t)
        HANDLE_UNARY_OP(int16_t) HANDLE_UNARY_OP(uint16_t)
        HANDLE_UNARY_OP(int32_t) HANDLE_UNARY_OP(uint32_t)
        HANDLE_UNARY_OP(int64_t) HANDLE_UNARY_OP(uint64_t)
        HANDLE_UNARY_OP(float) HANDLE_UNARY_OP(double)
        default: break;
     }
    #undef HANDLE_UNARY_OP
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
 // --- Specific Operations (Completed) --
 // Arithmetic
 inline BDIValueVariant performAddition(const BDIValueVariant& l, const BDIValueVariant& r) { return performNumericBitwiseBinaryOp(l, r, [](auto a,
 auto b){ return a + b; }); }
 inline BDIValueVariant performSubtraction(const BDIValueVariant& l, const BDIValueVariant& r) { return performNumericBitwiseBinaryOp(l, r, [](auto
 a, auto b){ return a - b; }); }
 inline BDIValueVariant performMultiplication(const BDIValueVariant& l, const BDIValueVariant& r) { return performNumericBitwiseBinaryOp(l, r, []
 (auto a, auto b){ return a * b; }); }
 inline BDIValueVariant performDivision(const BDIValueVariant& l, const BDIValueVariant& r) { return performNumericBitwiseBinaryOp(l, r, [](auto a,
 auto b){ if (b == static_cast<decltype(b)>(0)) throw std::runtime_error("Division by zero"); return a / b; }); }
 inline BDIValueVariant performModulo(const BDIValueVariant& l, const BDIValueVariant& r) { return performNumericBitwiseBinaryOp(l, r, [](auto a,
 auto b){ if constexpr (std::is_integral_v<decltype(a)>) { if (b == 0) throw std::runtime_error("Modulo by zero"); return a % b; } else throw
 std::runtime_error("MOD requires integers"); }, true); }
 inline BDIValueVariant performNegation(const BDIValueVariant& v) { return performNumericBitwiseUnaryOp(v, [](auto a){ if constexpr
 (std::is_unsigned_v<decltype(a)>) throw std::runtime_error("Cannot negate unsigned value"); else return -a; }); }
 inline BDIValueVariant performAbsolute(const BDIValueVariant& v) {
 // Special handling as ABS changes type potentially (signed -> unsigned equivalent size?)
 // For simplicity, return same type for now.
 return performNumericBitwiseUnaryOp(v, [](auto a){ using std::abs; using std::fabs; if constexpr (std::is_integral_v<decltype(a)> &&
 std::is_signed_v<decltype(a)>) return abs(a); else if constexpr (std::is_floating_point_v<decltype(a)>) return fabs(a); else return a; /* Unsigned is already
 abs */ });
 }
 // ... Implement Modulo (ensure integer), Negation, Abs ...
 // Bitwise (Needs integer check within lambda or separate helper)
 inline BDIValueVariant performBitwiseAND(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var) {
    return performNumericBitwiseBinaryOp(lhs_var, rhs_var, [](auto a, auto b){
        if constexpr (std::is_integral_v<decltype(a)>) return a & b; else throw std::runtime_error("BIT_AND requires integers"); });
 }
 // Bitwise
 inline BDIValueVariant performBitwiseAND(const BDIValueVariant& l, const BDIValueVariant& r) { return performNumericBitwiseBinaryOp(l, r, [](auto
 a, auto b){ if constexpr (std::is_integral_v<decltype(a)>) return a & b; else throw std::runtime_error("Bitwise op requires integers"); }, true); }
 inline BDIValueVariant performBitwiseOR(const BDIValueVariant& l, const BDIValueVariant& r) { return performNumericBitwiseBinaryOp(l, r, [](auto a,
 auto b){ if constexpr (std::is_integral_v<decltype(a)>) return a | b; else throw std::runtime_error("Bitwise op requires integers"); }, true); }
 inline BDIValueVariant performBitwiseXOR(const BDIValueVariant& l, const BDIValueVariant& r) { return performNumericBitwiseBinaryOp(l, r, [](auto
 a, auto b){ if constexpr (std::is_integral_v<decltype(a)>) return a ^ b; else throw std::runtime_error("Bitwise op requires integers"); }, true); }
 inline BDIValueVariant performBitwiseNOT(const BDIValueVariant& v) { return performNumericBitwiseUnaryOp(v, [](auto a){ if constexpr
 (std::is_integral_v<decltype(a)>) return ~a; else throw std::runtime_error("Bitwise op requires integers"); }, true); }
 // Shift operations (SHL, SHR, ASHR) need careful implementation considering shift amount type and potential UB for large/negative shifts. Left as
 exercise.
 inline BDIValueVariant performBitwiseSHL(const BDIValueVariant& l, const BDIValueVariant& r) { /* ... Implement using helpers, check shift amount
 range ... */ return std::monostate{}; }
 inline BDIValueVariant performBitwiseSHR(const BDIValueVariant& l, const BDIValueVariant& r) { /* ... Implement using helpers, check shift amount
 range ... */ return std::monostate{}; }
 inline BDIValueVariant performBitwiseASHR(const BDIValueVariant& l, const BDIValueVariant& r) { /* ... Implement using helpers, check shift amount
 range, signedness ... */ return std::monostate{}; }
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
 // Comparison
 template <typename Operation> BDIValueVariant performComparison(const BDIValueVariant& l, const BDIValueVariant& r, Operation op_lambda) { /*
 ... (as before) ... */ return std::monostate{};} // Define properly
 inline BDIValueVariant performComparisonEQ(const BDIValueVariant& l, const BDIValueVariant& r) { return performComparison(l, r, [](auto a, auto b){
 return a == b; }); }
 inline BDIValueVariant performComparisonNE(const BDIValueVariant& l, const BDIValueVariant& r) { return performComparison(l, r, [](auto a, auto b){
 return a != b; }); }
 inline BDIValueVariant performComparisonLT(const BDIValueVariant& l, const BDIValueVariant& r) { return performComparison(l, r, [](auto a, auto b){
 return a < b; }); }
 inline BDIValueVariant performComparisonLE(const BDIValueVariant& l, const BDIValueVariant& r) { return performComparison(l, r, [](auto a, auto b){
 return a <= b; }); }
 inline BDIValueVariant performComparisonGT(const BDIValueVariant& l, const BDIValueVariant& r) { return performComparison(l, r, [](auto a, auto b){
 return a > b; }); }
 inline BDIValueVariant performComparisonGE(const BDIValueVariant& l, const BDIValueVariant& r) { return performComparison(l, r, [](auto a, auto b){
 return a >= b; }); }
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
 // Logical
 inline BDIValueVariant performLogicalAND(const BDIValueVariant& l, const BDIValueVariant& r) { auto v1=convertValue<bool>(l); auto
 v2=convertValue<bool>(r); if(v1&&v2) return bool(*v1 && *v2); return std::monostate{};}
 inline BDIValueVariant performLogicalOR(const BDIValueVariant& l, const BDIValueVariant& r) { auto v1=convertValue<bool>(l); auto
 v2=convertValue<bool>(r); if(v1&&v2) return bool(*v1 || *v2); return std::monostate{};}
 inline BDIValueVariant performLogicalXOR(const BDIValueVariant& l, const BDIValueVariant& r) { auto v1=convertValue<bool>(l); auto
 v2=convertValue<bool>(r); if(v1&&v2) return bool(*v1 ^ *v2); return std::monostate{};}
 inline BDIValueVariant performLogicalNOT(const BDIValueVariant& v) { auto v1=convertValue<bool>(v); if(v1) return bool(!*v1); return
 std::monostate{};}
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
 // Conversion
 inline BDIValueVariant performConversion(const BDIValueVariant& input_var, BDIType target_bdi_type) {
 // ... (Implementation from previous step remains largely the same, relies on convertValue) ...
 #define HANDLE_CONVERSION_TARGET(CppType) \
 case core::payload::MapCppTypeToBdiType<CppType>::value: { \
 auto v = convertValue<CppType>(input_var); \
 return v ? BDIValueVariant{*v} : std::monostate{}; \
        }
    switch(target_bdi_type) { /* ... cases ... */ }
    #undef HANDLE_CONVERSION_TARGET
     return std::monostate{};
 }
 // Special case for BITCAST (reinterprets bits) - VERY DANGEROUS without size check
 inline BDIValueVariant performBitcast(const BDIValueVariant& input_var, BDIType target_bdi_type) {
    BDIType source_type = getBDIType(input_var);
    size_t source_size = core::types::getBdiTypeSize(source_type);
    size_t target_size = core::types::getBdiTypeSize(target_bdi_type);
    if (source_size != target_size || source_size == 0) {
         std::cerr << "VM Error: BITCAST requires source and target types of equal non-zero size." << std::endl;
        return std::monostate{};
    }
    // Convert input variant to payload, then payload back to target variant type
    core::payload::TypedPayload temp_payload = ExecutionContext::variantToPayload(input_var); // Needs access or pass as arg
    temp_payload.type = target_bdi_type; // Change the type tag
    return ExecutionContext::payloadToVariant(temp_payload); // Reinterpret
 }
 } // namespace bdi::runtime::vm_ops
 #endif // BDI_RUNTIME_VMTYPEOPERATIONS_HPP
