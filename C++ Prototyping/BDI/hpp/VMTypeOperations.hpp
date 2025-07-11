 #ifndef BDI_RUNTIME_VMTYPEOPERATIONS_HPP
 #define BDI_RUNTIME_VMTYPEOPERATIONS_HPP
 #include "BDIValueVariant.hpp"
 #include "TypeSystem.hpp"
 #include "MapCppTypeToBdiType.hpp" // Include mapping
 #include <optional>
 #include <cmath>
 #include <limits>
 #include <stdexcept>
 #include <iostream> // For errors
 #include <type_traits>
 #include <bit> // For bit_cast and integral checks in C++23
 namespace bdi::runtime::vm_ops {
    // Custom Exception for VM runtime errors
    class BDIExecutionError : public std::runtime_error {
    public:
        BDIExecutionError(const std::string& message) : std::runtime_error(message) {}
    };
 } // namespace bdi::runtime
 namespace bdi::runtime::vm_ops {
 using namespace bdi::core::types;
 using namespace bdi::core::payload;
 // --- Conversion Helper (with Overflow/Precision Checks) --
template <typename TargetType>
TargetType convertValueOrThrow(const BDIValueVariant& value_var, const std::string& context_msg = "") {
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
             throw BDIExecutionError("Cannot convert VOID/uninitialized value " + context_msg);
             } else if constexpr (std::is_same_v<SourceCppType, TargetType>) {
             result = arg; success = true;
             } else {
             if (TypeSystem::canImplicitlyConvert(source_bdi_type, target_bdi_type)) {
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
                 // --- Perform Conversion with Checks --
                 bool conversion_ok = true;
                 // Float -> Int Checks
                 if constexpr (std::is_integral_v<TargetType> && std::is_floating_point_v<SourceCppType>) {
                    if (!std::isfinite(arg) ||
                        arg < static_cast<SourceCppType>(std::numeric_limits<TargetType>::min()) - static_cast<SourceCppType>(0.5) || // Generous range check
                        arg > static_cast<SourceCppType>(std::numeric_limits<TargetType>::max()) + static_cast<SourceCppType>(0.5)) {
                        conversion_ok = false;
                    }
                 }
                 // Narrowing Int -> Int Checks
                 else if constexpr (std::is_integral_v<TargetType> && std::is_integral_v<SourceCppType> && sizeof(SourceCppType) > sizeof(TargetType)) {
                    if (arg < static_cast<SourceCppType>(std::numeric_limits<TargetType>::min()) ||
                        arg > static_cast<SourceCppType>(std::numeric_limits<TargetType>::max())) {
                         conversion_ok = false;
                    }
                 }
                 // Double -> Float Check (basic check for large magnitude loss)
                 else if constexpr (std::is_same_v<TargetType, float> && std::is_same_v<SourceCppType, double>) {
                     if (std::fabs(arg) > static_cast<SourceCppType>(std::numeric_limits<TargetType>::max())) {
                         conversion_ok = false; // Would likely become infinity
                     }
                 }
                 if (conversion_ok) {
                     result = static_cast<TargetType>(arg);
                     success = true;
                 } else {
                      throw BDIExecutionError("Unsafe or out-of-range conversion from " +
                           std::string(bdiTypeToString(source_bdi_type)) + " to " +
                           std::string(bdiTypeToString(target_bdi_type)) + " " + context_msg);
                 }
             } else {
                  throw BDIExecutionError("Conversion not allowed by TypeSystem from " +
                       std::string(bdiTypeToString(source_bdi_type)) + " to " +
                       std::string(bdiTypeToString(target_bdi_type)) + " " + context_msg);
             }
        }
    }, value_var);
    if (success) return result;
    // Should have thrown if !success and not monostate
    throw BDIExecutionError("Internal error during variant conversion " + context_msg);
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
    if (result_type == BDIType::UNKNOWN)  throw BDIExecutionError("Invalid type promotion for operation");
    if (requires_integer && !TypeSystem::isInteger(result_type)) throw BDIExecutionError("Operation requires integer types");
        throw std::runtime_error("Operation requires integer types after promotion"); // Throw for type errors
    }
    #define HANDLE_PROMOTED_BINARY_OP(CppType) \
        case core::payload::MapCppTypeToBdiType<CppType>::value: { \
            auto v1 = convertValueOrThrow<CppType>(lhs_var); \
            auto v2 = convertValueOrThrow<CppType>(rhs_var); \
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
    throw BDIExecutionError("Unhandled promoted type in binary op");
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
 BDIValueVariant performNumericBitwiseUnaryOp(const BDIValueVariant& var, Operation op_lambda, bool requires_integer = false) {
     BDIType type = getBDIType(var);
     if (requires_integer && !core::types::TypeSystem::isInteger(type)) throw BDIExecutionError("Unary op requires integer type");
          throw std::runtime_error("Operation requires integer type");
     }
     if (!requires_integer && !core::types::TypeSystem::isNumeric(type)) throw BDIExecutionError("Unary op requires numeric type"); // Adjust if needed
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
            auto v = convertValueOrThrow<CppType>(var); \
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
    throw BDIExecutionError("Unhandled type in unary op");
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
 inline BDIValueVariant performAbsolute(const BDIValueVariant& v) { return performNumericBitwiseUnaryOp(v, [](auto a){ using std::abs; using
 std::fabs; if constexpr (std::is_integral_v<decltype(a)> && std::is_signed_v<decltype(a)>) return abs(a); else if constexpr
 (std::is_floating_point_v<decltype(a)>) return fabs(a); else return a; }); }
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
 // Shift operations (SHL, SHR, ASHR) need careful implementation considering shift amount type and potential UB for large/negative shifts. Left as exercise.
 inline BDIValueVariant performBitwiseSHL(const BDIValueVariant& l, const BDIValueVariant& r) { /* ... Implement using helpers, check shift amount
 range ... */ return std::monostate{}; }
 inline BDIValueVariant performBitwiseSHR(const BDIValueVariant& l, const BDIValueVariant& r) { /* ... Implement using helpers, check shift amount
 range ... */ return std::monostate{}; }
 inline BDIValueVariant performBitwiseASHR(const BDIValueVariant& l, const BDIValueVariant& r) { /* ... Implement using helpers, check shift amount
 range, signedness ... */ return std::monostate{}; }
 // ... Implement OR, XOR, NOT, Shifts ...
 // Shifts
 // Note: C++ shift behavior for negative values or amounts >= width is potentially UB/implementation-defined. 
 // BDI should define precise semantics if needed (e.g., always mask shift amount). 
 inline BDIValueVariant performBitwiseSHL(const BDIValueVariant& l, const BDIValueVariant& r) {
    BDIType type1 = getBDIType(l);
    if (!TypeSystem::isInteger(type1)) throw BDIExecutionError("Shift value must be integer");
    // Use unsigned int for shift amount, check range? 
    auto shift_amount_opt = convertValue<unsigned int>(r); // Convert shift amount to unsigned int
    if (!shift_amount_opt) throw BDIExecutionError("Invalid shift amount type");
    unsigned int shift = shift_amount_opt.value();
    // Check for undefined behavior (shifting >= width)
    // Check shift range? (C++ standard UB if >= width) 
    // size_t width = getBdiTypeSize(type1) * 8; 
    // if (shift >= width) throw BDIExecutionError("Shift amount >= bit width"); 
    if (shift >= getBdiTypeSize(type1) * 8) throw BDIExecutionError("Shift amount greater than or equal to bit width");
    return performNumericBitwiseUnaryOp(l, [shift](auto a){ return a << shift; }, true);
 }
 inline BDIValueVariant performBitwiseSHR(const BDIValueVariant& l, const BDIValueVariant& r) { // Logical Shift Right
    BDIType type1 = getBDIType(l);
    if (!TypeSystem::isInteger(type1)) throw BDIExecutionError("Shift value must be integer");
    auto shift_amount_opt = convertValue<unsigned int>(r);
    if (!shift_amount_opt) throw BDIExecutionError("Invalid shift amount type");
    unsigned int shift = shift_amount_opt.value();
    if (shift >= getBdiTypeSize(type1) * 8) throw BDIExecutionError("Shift amount greater than or equal to bit width");
    // C++ >> is arithmetic for signed, logical for unsigned. Force unsigned cast for logical.
    return performNumericBitwiseUnaryOp(l, [shift](auto a){
        if constexpr (std::is_integral_v<decltype(a)>) {
            // Cast to corresponding unsigned type for logical shift
            using UnsignedT = std::make_unsigned_t<decltype(a)>;
            return static_cast<decltype(a)>( static_cast<UnsignedT>(a) >> shift );
        } else throw BDIExecutionError("SHR requires integers"); }, true);
 }
 inline BDIValueVariant performBitwiseASHR(const BDIValueVariant& l, const BDIValueVariant& r) { // Arithmetic Shift Right
     BDIType type1 = getBDIType(l);
     if (!TypeSystem::isInteger(type1)) throw BDIExecutionError("Shift value must be integer");
     auto shift_amount_opt = convertValue<unsigned int>(r);
     if (!shift_amount_opt) throw BDIExecutionError("Invalid shift amount type");
     unsigned int shift = shift_amount_opt.value();
     if (shift >= getBdiTypeSize(type1) * 8) throw BDIExecutionError("Shift amount greater than or equal to bit width");
     // C++ >> is arithmetic for signed types.
     return performNumericBitwiseUnaryOp(l, [shift](auto a){
            if constexpr (std::is_integral_v<decltype(a)>) { 
            using UnsignedT = std::make_unsigned_t<decltype(a)>; 
            return static_cast<decltype(a)>( static_cast<UnsignedT>(a) >> shift ); 
         } else throw BDIExecutionError("ASHR requires integers"); }, true);
 }  
 inline BDIValueVariant performBitwiseASHR(const BDIValueVariant& l, const BDIValueVariant& r) { // Arithmetic Shift Right 
     BDIType type1 = getBDIType(l); 
     if (!TypeSystem::isInteger(type1)) throw BDIExecutionError("Shift value must be integer"); 
     auto shift_amount_opt = convertValue<unsigned int>(r); 
     if (!shift_amount_opt) throw BDIExecutionError("Invalid shift amount type"); 
     unsigned int shift = shift_amount_opt.value(); 
     // if (shift >= getBdiTypeSize(type1) * 8) throw BDIExecutionError("Shift amount >= bit width"); 
     // C++ >> is arithmetic for signed types, logical for unsigned. 
     // We want arithmetic ONLY for signed types here. 
     return performNumericBitwiseUnaryOp(l, [shift](auto a){ 
         if constexpr (std::is_integral_v<decltype(a)> && std::is_signed_v<decltype(a)>) {
         return a >> shift; // Signed type -> arithmetic shift 
 }   else if constexpr (std::is_integral_v<decltype(a)> && !std::is_signed_v<decltype(a)>) { 
     // Explicitly do logical shift for unsigned if ASHR is called on it? Or error? Error is safer. 
     throw BDIExecutionError("ASHR called on unsigned type"); 
 }   else throw BDIExecutionError("ASHR requires integers"); }, true); 
 // Comparison (returns bool variant)
 template <typename Operation>
BDIValueVariant performComparison(const BDIValueVariant& lhs_var, const BDIValueVariant& rhs_var, Operation op_lambda) {
     // Similar promotion logic as numeric binary ops
     BDIType type1 = getBDIType(lhs_var);
     BDIType type2 = getBDIType(rhs_var);
     BDIType result_type = core::types::TypeSystem::getPromotedType(type1, type2);
     if (promoted_type == BDIType::UNKNOWN) { // Allow bool comparison if types match
     if (type1 == BDIType::BOOL && type2 == BDIType::BOOL) promoted_type = BDIType::BOOL;
         else if (type1 == BDIType::POINTER && type2 == BDIType::POINTER) promoted_type = BDIType::POINTER; // Allow pointer comparison
         else throw BDIExecutionError("Cannot promote types for comparison");
        #define HANDLE_PROMOTED_CMP_OP(CppType) \
        case MapCppTypeToBdiType<CppType>::value: { \
            auto v1 = convertValueOrThrow<CppType>(l); auto v2 = convertValueOrThrow<CppType>(r); \
            return BDIValueVariant{bool(op_lambda(v1, v2))}; \
        }
     switch(promoted_type) { /* Numeric cases */ /* Add bool case */ case BDIType::BOOL: { auto v1=convertValueOrThrow<bool>(l); auto
 v2=convertValueOrThrow<bool>(r); return BDIValueVariant{bool(op_lambda(v1,v2))}; } /* Add pointer case */ case BDIType::POINTER: { auto
 v1=convertValueOrThrow<uintptr_t>(l); auto v2=convertValueOrThrow<uintptr_t>(r); return BDIValueVariant{bool(op_lambda(v1,v2))}; } default:
 break;}
    #undef HANDLE_PROMOTED_CMP_OP
     throw BDIExecutionError("Unhandled promoted type in comparison op");
 }
 // Specific comparison implementations (remain the same, call performComparison)
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
    // Use convertValueOrThrow for robust conversion
    #define HANDLE_CONVERSION_TARGET(CppType) \
        case MapCppTypeToBdiType<CppType>::value: { \
            return BDIValueVariant{convertValueOrThrow<CppType>(input_var)}; \
        }
    switch(target_bdi_type) { /* Cases as before */ }
    #undef HANDLE_CONVERSION_TARGET
     throw BDIExecutionError("Unhandled target type in conversion");
 }
 // Bitcast
 inline BDIValueVariant performBitcast(const BDIValueVariant& input_var, BDIType target_bdi_type) {
    // ... (Implementation remains the same, potentially add try/catch around payload ops) ...
     BDIType source_type = getBDIType(input_var);
     size_t source_size = getBdiTypeSize(source_type);
     size_t target_size = getBdiTypeSize(target_bdi_type);
     if (source_size != target_size || source_size == 0) throw BDIExecutionError("Bitcast requires equal non-zero sizes");
     core::payload::TypedPayload temp_payload = ExecutionContext::variantToPayload(input_var); // Use static method
     temp_payload.type = target_bdi_type;
     return ExecutionContext::payloadToVariant(temp_payload); // Use static method
 }
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
