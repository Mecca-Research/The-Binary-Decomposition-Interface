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
// Add similar explicit tests for all other VMTypeOperations helpers... 
