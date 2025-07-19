#include "gtest.h" 
#include "VMTypeOperations.hpp" 
#include <limits> 
using namespace bdi::runtime; 
using namespace bdi::runtime::vm_ops; 
using namespace bdi::core::types; 
// --- VMTypeOperations Tests --- 
// Test convertValueOrThrow explicitly 
TEST(VMTypeOperationsTest, ConvertValueBasic) { 
    BDIValueVariant i32_val{int32_t{10}}; 
    BDIValueVariant f32_val{float{25.5f}}; 
    BDIValueVariant u64_val{uint64_t{1000}}; 
    BDIValueVariant bool_val{true}; 
    EXPECT_EQ(convertValueOrThrow<int32_t>(i32_val), 10); 
    EXPECT_FLOAT_EQ(convertValueOrThrow<float>(f32_val), 25.5f); 
    EXPECT_EQ(convertValueOrThrow<uint64_t>(u64_val), 1000); 
    EXPECT_EQ(convertValueOrThrow<bool>(bool_val), true); 
// Implicit Conversions allowed by TypeSystem 
    EXPECT_EQ(convertValueOrThrow<int64_t>(i32_val), 10L); 
    EXPECT_FLOAT_EQ(convertValueOrThrow<double>(f32_val), 25.5); 
    EXPECT_EQ(convertValueOrThrow<int32_t>(bool_val), 1); // bool -> int 
} 
TEST(VMTypeOperationsTest, ConvertValueFailure) { 
    BDIValueVariant i64_large{std::numeric_limits<int64_t>::max()}; 
    BDIValueVariant f64_large{std::numeric_limits<double>::max()}; 
    BDIValueVariant f32_nan{std::numeric_limits<float>::quiet_NaN()}; 
    BDIValueVariant i32_val{int32_t{10}}; 
    BDIValueVariant void_val{std::monostate{}}; 
// Overflow/Narrowing 
    EXPECT_THROW(convertValueOrThrow<int32_t>(i64_large), BDIExecutionError); 
    EXPECT_THROW(convertValueOrThrow<int32_t>(f64_large), BDIExecutionError); 
// Invalid Conversion 
    EXPECT_THROW(convertValueOrThrow<float>(i32_val), BDIExecutionError); // Assuming int->float is not implicit *for direct conversion helper
// Invalid Input 
    EXPECT_THROW(convertValueOrThrow<int32_t>(f32_nan), BDIExecutionError); // Should fail on non-finite 
    EXPECT_THROW(convertValueOrThrow<int32_t>(void_val), BDIExecutionError); 
} 
TEST(VMTypeOperationsTest, ShiftOperations) { 
    BDIValueVariant i32_val{int32_t{0b1010}}; // 10 
    BDIValueVariant u32_val{uint32_t{0xF0000001}}; 
    BDIValueVariant shift_amt{uint32_t{2}}; 
    BDIValueVariant shift_large{uint32_t{32}}; 
    BDIValueVariant shift_neg{int32_t{-1}}; // Should likely be disallowed or cast to unsigned 
// SHL
    BDIValueVariant shl_res = performBitwiseSHL(i32_val, shift_amt); 
    ASSERT_TRUE(std::holds_alternative<int32_t>(shl_res)); 
    EXPECT_EQ(std::get<int32_t>(shl_res), 0b101000); // 40 
// SHR (Logical) 
    BDIValueVariant shr_res_u = performBitwiseSHR(u32_val, shift_amt); 
    ASSERT_TRUE(std::holds_alternative<uint32_t>(shr_res_u)); 
    EXPECT_EQ(std::get<uint32_t>(shr_res_u), 0xF0000001 >> 2); // Logical shift 
    BDIValueVariant shr_res_i = performBitwiseSHR(i32_val, shift_amt); // SHR on signed -> logical 
    ASSERT_TRUE(std::holds_alternative<int32_t>(shr_res_i)); 
    EXPECT_EQ(std::get<int32_t>(shr_res_i), int32_t(uint32_t(10) >> 2)); // 2 
// ASHR (Arithmetic) 
    BDIValueVariant ashr_res_i = performBitwiseASHR(i32_val, shift_amt); // ASHR on positive signed 
    ASSERT_TRUE(std::holds_alternative<int32_t>(ashr_res_i)); 
    EXPECT_EQ(std::get<int32_t>(ashr_res_i), 10 >> 2); // 2 
    BDIValueVariant i32_neg{int32_t{-10}}; // 0xFFFFFFF6 
    BDIValueVariant ashr_res_neg = performBitwiseASHR(i32_neg, shift_amt); // ASHR on negative signed 
    ASSERT_TRUE(std::holds_alternative<int32_t>(ashr_res_neg)); 
    EXPECT_EQ(std::get<int32_t>(ashr_res_neg), -10 >> 2); // Sign extension, result depends on compiler but usually -3 
// Error cases 
    EXPECT_THROW(performBitwiseSHL(i32_val, shift_large), BDIExecutionError); // Shift >= width 
    EXPECT_THROW(performBitwiseASHR(u32_val, shift_amt), BDIExecutionError); // ASHR on unsigned 
TEST(VMTypeOperationsTest, ConvertValueOverflowIntToInt) { 
    BDIValueVariant i64_max{std::numeric_limits<int64_t>::max()}; 
    BDIValueVariant i64_min{std::numeric_limits<int64_t>::min()}; 
    BDIValueVariant i32_max{std::numeric_limits<int32_t>::max()}; 
    BDIValueVariant i32_min{std::numeric_limits<int32_t>::min()}; 
// Valid narrowing 
    EXPECT_EQ(convertValueOrThrow<int32_t>(BDIValueVariant{int64_t{100}}), 100); 
    EXPECT_EQ(convertValueOrThrow<int16_t>(BDIValueVariant{int32_t{-10}}), -10); 
// Invalid narrowing (Overflow/Underflow) 
    EXPECT_THROW(convertValueOrThrow<int32_t>(i64_max), BDIExecutionError); 
    EXPECT_THROW(convertValueOrThrow<int32_t>(i64_min), BDIExecutionError); 
} 
    EXPECT_THROW(convertValueOrThrow<uint32_t>(BDIValueVariant{int64_t{-1}}), BDIExecutionError); // Signed to smaller unsigned 
    EXPECT_THROW(convertValueOrThrow<int8_t>(i32_max), BDIExecutionError); 
TEST(VMTypeOperationsTest, ConvertValueOverflowFloatToInt) { 
    BDIValueVariant f64_way_too_big = BDIValueVariant{std::numeric_limits<double>::max()}; 
    BDIValueVariant f64_too_big = BDIValueVariant{static_cast<double>(std::numeric_limits<int32_t>::max()) + 100.0}; 
    BDIValueVariant f64_too_small = BDIValueVariant{static_cast<double>(std::numeric_limits<int32_t>::min()) - 100.0}; 
    BDIValueVariant f64_nan{std::numeric_limits<double>::quiet_NaN()}; 
    BDIValueVariant f64_inf{std::numeric_limits<double>::infinity()}; 
    BDIValueVariant f64_neg_inf{-std::numeric_limits<double>::infinity()}; 
// Valid conversion (truncation expected) 
    EXPECT_EQ(convertValueOrThrow<int32_t>(BDIValueVariant{double{123.75}}), 123); 
    EXPECT_EQ(convertValueOrThrow<int32_t>(BDIValueVariant{double{-456.99}}), -456); 
    EXPECT_EQ(convertValueOrThrow<uint32_t>(BDIValueVariant{double{789.0}}), 789); 
// Invalid conversions 
    EXPECT_THROW(convertValueOrThrow<int32_t>(f64_way_too_big), BDIExecutionError); 
    EXPECT_THROW(convertValueOrThrow<int32_t>(f64_too_big), BDIExecutionError); 
    EXPECT_THROW(convertValueOrThrow<int32_t>(f64_too_small), BDIExecutionError); 
    EXPECT_THROW(convertValueOrThrow<uint32_t>(BDIValueVariant{double{-1.0}}), BDIExecutionError); // Negative to unsigned 
    EXPECT_THROW(convertValueOrThrow<int32_t>(f64_nan), BDIExecutionError); 
    EXPECT_THROW(convertValueOrThrow<int32_t>(f64_inf), BDIExecutionError); 
    EXPECT_THROW(convertValueOrThrow<int32_t>(f64_neg_inf), BDIExecutionError); 
} 
TEST(VMTypeOperationsTest, ConvertValuePrecisionLossFloat) { 
// Double -> Float 
    BDIValueVariant d_precise{1.2345678901234567}; // Precision likely lost in float 
    BDIValueVariant d_ok{1.5}; 
    BDIValueVariant d_large{3.5e38}; // Too large for float max 
    EXPECT_NO_THROW(convertValueOrThrow<float>(d_precise)); // Conversion allowed, loss expected 
    EXPECT_FLOAT_EQ(convertValueOrThrow<float>(d_precise).value_or(0.0f), 1.2345678901234567f); // Compare as float 
    EXPECT_FLOAT_EQ(convertValueOrThrow<float>(d_ok).value_or(0.0f), 1.5f); 
//EXPECT_THROW(convertValueOrThrow<float>(d_large), BDIExecutionError); // Overflow check might be needed 
// Float -> Double (Generally safe) 
    BDIValueVariant f_val{1.23f}; 
    EXPECT_NO_THROW(convertValueOrThrow<double>(f_val)); 
    EXPECT_DOUBLE_EQ(convertValueOrThrow<double>(f_val).value_or(0.0), 1.23); // Allow small tolerance 
} 
TEST(VMTypeOperationsTest, DivisionByZeroFloat) { 
    BDIValueVariant f10{float{10.0f}}; 
    BDIValueVariant f0{float{0.0f}}; 
    BDIValueVariant fnan{std::numeric_limits<float>::quiet_NaN()}; 
// Standard IEEE 754 behavior for division by zero (produces Inf) - our op throws 
    EXPECT_THROW(vm_ops::performDivision(f10, f0), BDIExecutionError); 
    EXPECT_THROW(vm_ops::performDivision(f0, f0), BDIExecutionError); // 0/0 is NaN -> error 
// Standard behavior for NaN (any op with NaN -> NaN) - our op should throw if conversion fails 
// EXPECT_THROW(vm_ops::performAddition(f10, fnan), BDIExecutionError); // Throws if NaN cannot be converted/handled 
} 
TEST(VMTypeOperationsTest, ComparisonNaN) { 
    BDIValueVariant f_nan1{std::numeric_limits<float>::quiet_NaN()}; 
    BDIValueVariant f_nan2{std::numeric_limits<float>::quiet_NaN()}; 
    BDIValueVariant f_val{5.0f}; 
// NaN compares false to everything, including itself (except !=) 
    EXPECT_EQ(std::get<bool>(vm_ops::performComparisonEQ(f_nan1, f_nan1)), false); 
    EXPECT_EQ(std::get<bool>(vm_ops::performComparisonNE(f_nan1, f_nan1)), true); // NaN != NaN is true 
    EXPECT_EQ(std::get<bool>(vm_ops::performComparisonLT(f_nan1, f_val)), false); 
    EXPECT_EQ(std::get<bool>(vm_ops::performComparisonGT(f_nan1, f_val)), false); 
    EXPECT_EQ(std::get<bool>(vm_ops::performComparisonLE(f_nan1, f_val)), false); 
    EXPECT_EQ(std::get<bool>(vm_ops::performComparisonGE(f_nan1, f_val)), false); 
    EXPECT_EQ(std::get<bool>(vm_ops::performComparisonEQ(f_val, f_nan1)), false); 
} 
TEST(VMTypeOperationsTest, ShiftEdgeCases) { 
    BDIValueVariant i32_val{int32_t{0x12345678}}; 
    BDIValueVariant shift0{uint32_t{0}}; 
    BDIValueVariant shift31{uint32_t{31}}; 
    BDIValueVariant shift32{uint32_t{32}}; // Invalid shift amount 
    BDIValueVariant shift_neg{int32_t{-1}}; // Invalid shift amount type (should convert/error) 
// Shift by 0 
    EXPECT_NO_THROW(vm_ops::performBitwiseSHL(i32_val, shift0)); 
    EXPECT_EQ(std::get<int32_t>(vm_ops::performBitwiseSHL(i32_val, shift0)), 0x12345678); 
// Shift by max valid amount (31 for 32-bit) 
    EXPECT_NO_THROW(vm_ops::performBitwiseSHL(i32_val, shift31)); 
    EXPECT_EQ(std::get<int32_t>(vm_ops::performBitwiseSHL(i32_val, shift31)), int32_t(0x12345678 << 31)); 
// Shift >= width (expect throw) 
    EXPECT_THROW(vm_ops::performBitwiseSHL(i32_val, shift32), BDIExecutionError); 
    EXPECT_THROW(vm_ops::performBitwiseSHR(i32_val, shift32), BDIExecutionError); 
// Invalid shift amount type (expect throw during conversion in helper) 
    EXPECT_THROW(vm_ops::performBitwiseSHL(i32_val, shift_neg), BDIExecutionError); 
} 
// Add similar explicit tests for all other VMTypeOperations helpers... 
