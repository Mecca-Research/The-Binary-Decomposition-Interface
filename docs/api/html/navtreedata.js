/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "BDI Kernel", "index.html", [
    [ "BDI Automatic Differentiation", "index.html", [
      [ "Components", "index.html#autotoc_md609", [
        [ "Forward Mode AD (<tt>forward_ad.h/c</tt>)", "index.html#autotoc_md610", null ],
        [ "Reverse Mode AD (<tt>reverse_ad.h/c</tt>)", "index.html#autotoc_md611", null ],
        [ "Gradient Utilities (<tt>gradient.h/c</tt>)", "index.html#autotoc_md612", null ]
      ] ],
      [ "Performance Considerations", "index.html#autotoc_md613", null ],
      [ "Testing", "index.html#autotoc_md614", null ],
      [ "Integration with BDI", "index.html#autotoc_md615", null ]
    ] ],
    [ "P1 Bug Fixes Summary - Bytecode Generation", "md_C_BUG_FIX_SUMMARY.html", [
      [ "Overview", "md_C_BUG_FIX_SUMMARY.html#autotoc_md1", null ],
      [ "Bugs Fixed", "md_C_BUG_FIX_SUMMARY.html#autotoc_md2", [
        [ "Bug 1: Short-circuit Evaluation for && and || Operators", "md_C_BUG_FIX_SUMMARY.html#autotoc_md3", null ],
        [ "Bug 2: Out-of-bounds Read in Peephole Optimization", "md_C_BUG_FIX_SUMMARY.html#autotoc_md4", null ]
      ] ],
      [ "Tests Added", "md_C_BUG_FIX_SUMMARY.html#autotoc_md5", [
        [ "Short-circuit Evaluation Tests (4 tests)", "md_C_BUG_FIX_SUMMARY.html#autotoc_md6", null ],
        [ "Peephole Optimization Safety Tests (3 tests)", "md_C_BUG_FIX_SUMMARY.html#autotoc_md7", null ]
      ] ],
      [ "Test Results", "md_C_BUG_FIX_SUMMARY.html#autotoc_md8", [
        [ "Standard Build", "md_C_BUG_FIX_SUMMARY.html#autotoc_md9", null ],
        [ "AddressSanitizer Build", "md_C_BUG_FIX_SUMMARY.html#autotoc_md10", null ]
      ] ],
      [ "Files Changed", "md_C_BUG_FIX_SUMMARY.html#autotoc_md11", null ],
      [ "Pull Request", "md_C_BUG_FIX_SUMMARY.html#autotoc_md12", null ],
      [ "Verification Commands", "md_C_BUG_FIX_SUMMARY.html#autotoc_md13", null ],
      [ "Success Criteria - All Met ✅", "md_C_BUG_FIX_SUMMARY.html#autotoc_md14", null ],
      [ "Next Steps", "md_C_BUG_FIX_SUMMARY.html#autotoc_md15", null ],
      [ "Related Work", "md_C_BUG_FIX_SUMMARY.html#autotoc_md16", null ]
    ] ],
    [ "BDI \"C\" Folder: Comprehensive Analysis & Phased Refactoring Plan", "md_C_C_analysis_plan.html", [
      [ "Executive Summary", "md_C_C_analysis_plan.html#autotoc_md20", null ],
      [ "Table of Contents", "md_C_C_analysis_plan.html#autotoc_md22", null ],
      [ "1. Current Architecture Overview", "md_C_C_analysis_plan.html#autotoc_md24", [
        [ "1.1 Directory Structure", "md_C_C_analysis_plan.html#autotoc_md25", null ],
        [ "1.2 Architectural Philosophy", "md_C_C_analysis_plan.html#autotoc_md26", null ],
        [ "1.3 Current Build System", "md_C_C_analysis_plan.html#autotoc_md27", null ]
      ] ],
      [ "2. Component-by-Component Analysis", "md_C_C_analysis_plan.html#autotoc_md29", [
        [ "2.1 Binary Counting Interface (BCI)", "md_C_C_analysis_plan.html#autotoc_md30", null ],
        [ "2.2 Boolean Translation Layer (BTL)", "md_C_C_analysis_plan.html#autotoc_md32", null ],
        [ "2.3 Code Generator (Codegen)", "md_C_C_analysis_plan.html#autotoc_md34", null ],
        [ "2.4 Compiler Infrastructure", "md_C_C_analysis_plan.html#autotoc_md36", [
          [ "2.4.1 Lexer (220 lines)", "md_C_C_analysis_plan.html#autotoc_md37", null ],
          [ "2.4.2 Parser (246 lines)", "md_C_C_analysis_plan.html#autotoc_md38", null ],
          [ "2.4.3 AST (94 lines)", "md_C_C_analysis_plan.html#autotoc_md39", null ],
          [ "2.4.4 Semantic Analyzer (165 lines)", "md_C_C_analysis_plan.html#autotoc_md40", null ],
          [ "2.4.5 Type System (56 lines)", "md_C_C_analysis_plan.html#autotoc_md41", null ]
        ] ],
        [ "2.5 AI Trainer", "md_C_C_analysis_plan.html#autotoc_md43", null ],
        [ "2.6 Kernel (Aeon-0)", "md_C_C_analysis_plan.html#autotoc_md45", [
          [ "2.6.1 Graph Core (graph.h, graph.c)", "md_C_C_analysis_plan.html#autotoc_md46", null ],
          [ "2.6.2 HAM (Hierarchical Access Memory)", "md_C_C_analysis_plan.html#autotoc_md47", null ],
          [ "2.6.3 Device Abstraction", "md_C_C_analysis_plan.html#autotoc_md48", null ],
          [ "2.6.4 Scheduler", "md_C_C_analysis_plan.html#autotoc_md49", null ],
          [ "2.6.5 File System (xv6-inspired)", "md_C_C_analysis_plan.html#autotoc_md50", null ],
          [ "2.6.6 Process Management", "md_C_C_analysis_plan.html#autotoc_md51", null ]
        ] ],
        [ "2.7 Virtual Machine (VM)", "md_C_C_analysis_plan.html#autotoc_md53", null ],
        [ "2.8 Root Files", "md_C_C_analysis_plan.html#autotoc_md55", null ]
      ] ],
      [ "3. C23 Modernization Opportunities", "md_C_C_analysis_plan.html#autotoc_md57", [
        [ "3.1 Current C Standard Usage", "md_C_C_analysis_plan.html#autotoc_md58", null ],
        [ "3.2 C23 Features to Adopt", "md_C_C_analysis_plan.html#autotoc_md59", [
          [ "3.2.1 <tt>nullptr</tt> (Critical Priority)", "md_C_C_analysis_plan.html#autotoc_md60", null ],
          [ "3.2.2 Attributes <tt>[[...]]</tt> (High Priority)", "md_C_C_analysis_plan.html#autotoc_md62", null ],
          [ "3.2.3 <tt>typeof</tt> and <tt>auto</tt> (High Priority)", "md_C_C_analysis_plan.html#autotoc_md64", null ],
          [ "3.2.4 <tt>constexpr</tt> (Medium Priority)", "md_C_C_analysis_plan.html#autotoc_md66", null ],
          [ "3.2.5 <tt>_Static_assert</tt> (High Priority)", "md_C_C_analysis_plan.html#autotoc_md68", null ],
          [ "3.2.6 <tt>_BitInt(N)</tt> (Medium Priority)", "md_C_C_analysis_plan.html#autotoc_md70", null ],
          [ "3.2.7 <tt>_Generic</tt> (Medium Priority)", "md_C_C_analysis_plan.html#autotoc_md72", null ],
          [ "3.2.8 <tt>_Alignas</tt> (Low Priority)", "md_C_C_analysis_plan.html#autotoc_md74", null ],
          [ "3.2.9 Improved Enums (Low Priority)", "md_C_C_analysis_plan.html#autotoc_md76", null ]
        ] ],
        [ "3.3 C23 Adoption Priority Matrix", "md_C_C_analysis_plan.html#autotoc_md78", null ]
      ] ],
      [ "4. Systems Approach Improvements", "md_C_C_analysis_plan.html#autotoc_md80", [
        [ "4.1 Memory Management", "md_C_C_analysis_plan.html#autotoc_md81", [
          [ "4.1.1 Smart Memory Allocators", "md_C_C_analysis_plan.html#autotoc_md82", null ],
          [ "4.1.2 Memory Profiling", "md_C_C_analysis_plan.html#autotoc_md83", null ],
          [ "4.1.3 NUMA-Aware Allocation", "md_C_C_analysis_plan.html#autotoc_md84", null ]
        ] ],
        [ "4.2 Concurrency & Parallelism", "md_C_C_analysis_plan.html#autotoc_md86", [
          [ "4.2.1 Thread Pool", "md_C_C_analysis_plan.html#autotoc_md87", null ],
          [ "4.2.2 Lock-Free Data Structures", "md_C_C_analysis_plan.html#autotoc_md88", null ],
          [ "4.2.3 Parallel Graph Execution", "md_C_C_analysis_plan.html#autotoc_md89", null ]
        ] ],
        [ "4.3 Performance Optimization", "md_C_C_analysis_plan.html#autotoc_md91", [
          [ "4.3.1 Profiling Infrastructure", "md_C_C_analysis_plan.html#autotoc_md92", null ],
          [ "4.3.2 Cache Optimization", "md_C_C_analysis_plan.html#autotoc_md93", null ],
          [ "4.3.3 SIMD Optimization", "md_C_C_analysis_plan.html#autotoc_md94", null ]
        ] ],
        [ "4.4 Error Handling & Diagnostics", "md_C_C_analysis_plan.html#autotoc_md96", [
          [ "4.4.1 Error Code System", "md_C_C_analysis_plan.html#autotoc_md97", null ],
          [ "4.4.2 Logging System", "md_C_C_analysis_plan.html#autotoc_md98", null ],
          [ "4.4.3 Assertion System", "md_C_C_analysis_plan.html#autotoc_md99", null ]
        ] ],
        [ "4.5 Testing Infrastructure", "md_C_C_analysis_plan.html#autotoc_md101", [
          [ "4.5.1 Unit Testing Framework", "md_C_C_analysis_plan.html#autotoc_md102", null ],
          [ "4.5.2 Benchmark Framework", "md_C_C_analysis_plan.html#autotoc_md103", null ]
        ] ],
        [ "4.6 Build System Enhancements", "md_C_C_analysis_plan.html#autotoc_md105", [
          [ "4.6.1 CMake Migration", "md_C_C_analysis_plan.html#autotoc_md106", null ],
          [ "4.6.2 Continuous Integration", "md_C_C_analysis_plan.html#autotoc_md107", null ]
        ] ]
      ] ],
      [ "5. Phased Refactoring Plan", "md_C_C_analysis_plan.html#autotoc_md109", [
        [ "Phase 0: Foundation & Preparation (Weeks 1-2)", "md_C_C_analysis_plan.html#autotoc_md110", null ],
        [ "Phase 1: C23 Core Modernization (Weeks 3-6)", "md_C_C_analysis_plan.html#autotoc_md112", [
          [ "Phase 1.1: <tt>nullptr</tt> Migration (Week 3)", "md_C_C_analysis_plan.html#autotoc_md113", null ],
          [ "Phase 1.2: Attribute Annotations (Week 4)", "md_C_C_analysis_plan.html#autotoc_md115", null ],
          [ "Phase 1.3: Static Assertions (Week 5)", "md_C_C_analysis_plan.html#autotoc_md117", null ],
          [ "Phase 1.4: Type Inference (<tt>typeof</tt>, <tt>auto</tt>) (Week 6)", "md_C_C_analysis_plan.html#autotoc_md119", null ]
        ] ],
        [ "Phase 2: Component Expansion - BCI & BTL (Weeks 7-10)", "md_C_C_analysis_plan.html#autotoc_md121", [
          [ "Phase 2.1: BCI Expansion (Weeks 7-8)", "md_C_C_analysis_plan.html#autotoc_md122", null ],
          [ "Phase 2.2: BTL Expansion (Weeks 9-10)", "md_C_C_analysis_plan.html#autotoc_md124", null ]
        ] ],
        [ "Phase 3: Compiler Infrastructure Enhancement (Weeks 11-16)", "md_C_C_analysis_plan.html#autotoc_md126", [
          [ "Phase 3.1: Type System Expansion (Weeks 11-12)", "md_C_C_analysis_plan.html#autotoc_md127", null ],
          [ "Phase 3.2: Parser & AST Enhancement (Weeks 13-14)", "md_C_C_analysis_plan.html#autotoc_md129", null ],
          [ "Phase 3.3: Semantic Analysis Enhancement (Weeks 15-16)", "md_C_C_analysis_plan.html#autotoc_md131", null ]
        ] ],
        [ "Phase 4: Code Generation & Optimization (Weeks 17-22)", "md_C_C_analysis_plan.html#autotoc_md133", [
          [ "Phase 4.1: SSA Construction (Weeks 17-18)", "md_C_C_analysis_plan.html#autotoc_md134", null ],
          [ "Phase 4.2: Optimization Passes (Weeks 19-20)", "md_C_C_analysis_plan.html#autotoc_md136", null ],
          [ "Phase 4.3: Backend Code Generation (Weeks 21-22)", "md_C_C_analysis_plan.html#autotoc_md138", null ]
        ] ],
        [ "Phase 5: Kernel Enhancement (Weeks 23-30)", "md_C_C_analysis_plan.html#autotoc_md140", [
          [ "Phase 5.1: Graph Optimization (Weeks 23-24)", "md_C_C_analysis_plan.html#autotoc_md141", null ],
          [ "Phase 5.2: Device Backend Implementation (Weeks 25-26)", "md_C_C_analysis_plan.html#autotoc_md143", null ],
          [ "Phase 5.3: Scheduler Implementation (Weeks 27-28)", "md_C_C_analysis_plan.html#autotoc_md145", null ],
          [ "Phase 5.4: HAM Intelligence (Weeks 29-30)", "md_C_C_analysis_plan.html#autotoc_md147", null ]
        ] ],
        [ "Phase 6: AI Trainer Implementation (Weeks 31-36)", "md_C_C_analysis_plan.html#autotoc_md149", [
          [ "Phase 6.1: Automatic Differentiation (Weeks 31-32)", "md_C_C_analysis_plan.html#autotoc_md150", null ],
          [ "Phase 6.2: Optimizer Suite (Weeks 33-34)", "md_C_C_analysis_plan.html#autotoc_md152", null ],
          [ "Phase 6.3: Loss Functions & Metrics (Weeks 35-36)", "md_C_C_analysis_plan.html#autotoc_md154", null ]
        ] ],
        [ "Phase 7: VM Enhancement & JIT (Weeks 37-42)", "md_C_C_analysis_plan.html#autotoc_md156", [
          [ "Phase 7.1: JIT Compiler (Weeks 37-40)", "md_C_C_analysis_plan.html#autotoc_md157", null ],
          [ "Phase 7.2: Garbage Collection (Weeks 41-42)", "md_C_C_analysis_plan.html#autotoc_md159", null ]
        ] ],
        [ "Phase 8: Integration & Testing (Weeks 43-48)", "md_C_C_analysis_plan.html#autotoc_md161", [
          [ "Phase 8.1: System Integration (Weeks 43-44)", "md_C_C_analysis_plan.html#autotoc_md162", null ],
          [ "Phase 8.2: Comprehensive Testing (Weeks 45-46)", "md_C_C_analysis_plan.html#autotoc_md164", null ],
          [ "Phase 8.3: Documentation (Weeks 47-48)", "md_C_C_analysis_plan.html#autotoc_md166", null ]
        ] ]
      ] ],
      [ "6. Risk Assessment & Mitigation", "md_C_C_analysis_plan.html#autotoc_md168", [
        [ "6.1 Technical Risks", "md_C_C_analysis_plan.html#autotoc_md169", [
          [ "Risk 1: C23 Compiler Support", "md_C_C_analysis_plan.html#autotoc_md170", null ],
          [ "Risk 2: Performance Regression", "md_C_C_analysis_plan.html#autotoc_md172", null ],
          [ "Risk 3: Integration Complexity", "md_C_C_analysis_plan.html#autotoc_md174", null ],
          [ "Risk 4: Memory Safety Issues", "md_C_C_analysis_plan.html#autotoc_md176", null ],
          [ "Risk 5: Scope Creep", "md_C_C_analysis_plan.html#autotoc_md178", null ]
        ] ],
        [ "6.2 Resource Risks", "md_C_C_analysis_plan.html#autotoc_md180", [
          [ "Risk 6: Developer Availability", "md_C_C_analysis_plan.html#autotoc_md181", null ],
          [ "Risk 7: Hardware Requirements", "md_C_C_analysis_plan.html#autotoc_md183", null ]
        ] ],
        [ "6.3 Risk Matrix", "md_C_C_analysis_plan.html#autotoc_md185", null ]
      ] ],
      [ "7. Success Metrics & Validation", "md_C_C_analysis_plan.html#autotoc_md187", [
        [ "7.1 Code Quality Metrics", "md_C_C_analysis_plan.html#autotoc_md188", null ],
        [ "7.2 Performance Metrics", "md_C_C_analysis_plan.html#autotoc_md190", null ],
        [ "7.3 Feature Completeness", "md_C_C_analysis_plan.html#autotoc_md192", null ],
        [ "7.4 Documentation Metrics", "md_C_C_analysis_plan.html#autotoc_md194", null ]
      ] ],
      [ "8. Appendices", "md_C_C_analysis_plan.html#autotoc_md196", [
        [ "Appendix A: C23 Feature Reference", "md_C_C_analysis_plan.html#autotoc_md197", null ],
        [ "Appendix B: Build System Reference", "md_C_C_analysis_plan.html#autotoc_md199", null ],
        [ "Appendix C: Testing Strategy", "md_C_C_analysis_plan.html#autotoc_md201", null ],
        [ "Appendix D: Coding Standards", "md_C_C_analysis_plan.html#autotoc_md203", null ],
        [ "Appendix E: Timeline Summary", "md_C_C_analysis_plan.html#autotoc_md205", null ],
        [ "Appendix F: Resource Requirements", "md_C_C_analysis_plan.html#autotoc_md207", null ],
        [ "Appendix G: References", "md_C_C_analysis_plan.html#autotoc_md209", null ]
      ] ],
      [ "Conclusion", "md_C_C_analysis_plan.html#autotoc_md211", null ]
    ] ],
    [ "Phase 2: BCI & BTL API Documentation", "md_C_docs_PHASE2_API_DOCUMENTATION.html", [
      [ "Table of Contents", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md215", null ],
      [ "BCI (Binary Counting Interface)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md217", [
        [ "Binary Arithmetic Library (<tt>bci_arithmetic.h</tt>)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md218", [
          [ "Data Structures", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md219", null ],
          [ "Functions", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md220", null ],
          [ "Example Usage", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md221", null ]
        ] ],
        [ "Bit Manipulation Utilities (<tt>bci_bitops.h</tt>)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md223", [
          [ "Bit Operations", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md224", null ],
          [ "Bit Counting", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md225", null ],
          [ "Bit Manipulation", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md226", null ],
          [ "Example Usage", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md227", null ]
        ] ],
        [ "Conversion Utilities (<tt>bci_conversion.h</tt>)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md229", [
          [ "Functions", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md230", null ],
          [ "Example Usage", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md231", null ]
        ] ],
        [ "SIMD Operations (<tt>bci_simd.h</tt>)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md233", [
          [ "Feature Detection", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md234", null ],
          [ "AVX2 Operations (if <tt>BCI_HAS_AVX2</tt> is defined)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md235", null ],
          [ "Scalar Fallbacks (always available)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md236", null ],
          [ "Example Usage", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md237", null ]
        ] ]
      ] ],
      [ "BTL (Boolean Translation Layer)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md239", [
        [ "ISA Support (<tt>btl_isa.h</tt>)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md240", [
          [ "Enumerations", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md241", null ],
          [ "Data Structures", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md242", null ],
          [ "Functions", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md243", null ],
          [ "Example Usage", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md244", null ]
        ] ],
        [ "Register Allocator (<tt>btl_regalloc.h</tt>)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md246", [
          [ "Data Structures", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md247", null ],
          [ "Functions", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md248", null ],
          [ "Example Usage", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md249", null ]
        ] ],
        [ "Instruction Scheduler (<tt>btl_scheduler.h</tt>)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md251", [
          [ "Functions", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md252", null ],
          [ "Example Usage", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md253", null ]
        ] ],
        [ "Peephole Optimizer (<tt>btl_peephole.h</tt>)", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md255", [
          [ "Data Structures", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md256", null ],
          [ "Functions", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md257", null ],
          [ "Predefined Rules", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md258", null ],
          [ "Example Usage", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md259", null ]
        ] ]
      ] ],
      [ "Build System", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md261", [
        [ "CMake Configuration", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md262", null ],
        [ "Build Options", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md263", null ],
        [ "Feature Detection", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md264", null ]
      ] ],
      [ "Testing", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md266", [
        [ "Running Tests", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md267", null ],
        [ "Test Coverage", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md268", null ]
      ] ],
      [ "Performance", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md270", [
        [ "Running Benchmarks", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md271", null ],
        [ "Expected Performance", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md272", null ]
      ] ],
      [ "Risk Mitigation", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md274", [
        [ "Feature Detection Macros", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md275", null ],
        [ "Sanitizer Support", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md276", null ],
        [ "API Contracts", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md277", null ]
      ] ],
      [ "License", "md_C_docs_PHASE2_API_DOCUMENTATION.html#autotoc_md279", null ]
    ] ],
    [ "Phase 2 Implementation Notes", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html", [
      [ "Overview", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md281", null ],
      [ "Implementation Details", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md282", [
        [ "BCI Components", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md283", [
          [ "1. Binary Arithmetic (<tt>bci_arithmetic.c</tt>)", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md284", null ],
          [ "2. Bit Operations (<tt>bci_bitops.c</tt>)", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md285", null ],
          [ "3. SIMD Operations (<tt>bci_simd.c</tt>)", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md286", null ]
        ] ],
        [ "BTL Components", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md287", [
          [ "1. ISA Support (<tt>btl_isa.c</tt>)", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md288", null ],
          [ "2. Register Allocator (<tt>btl_regalloc.c</tt>)", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md289", null ],
          [ "3. Instruction Scheduler (<tt>btl_scheduler.c</tt>)", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md290", null ],
          [ "4. Peephole Optimizer (<tt>btl_peephole.c</tt>)", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md291", null ]
        ] ]
      ] ],
      [ "Testing Strategy", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md292", [
        [ "Unit Tests", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md293", null ],
        [ "Integration Tests", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md294", null ],
        [ "Sanitizer Testing", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md295", null ]
      ] ],
      [ "Performance Benchmarks", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md296", [
        [ "BCI Performance", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md297", null ],
        [ "BTL Performance", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md298", null ]
      ] ],
      [ "Known Limitations", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md299", [
        [ "BCI", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md300", null ],
        [ "BTL", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md301", null ]
      ] ],
      [ "Future Work", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md302", [
        [ "Phase 3 Enhancements", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md303", null ]
      ] ],
      [ "References", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md304", [
        [ "Academic Papers", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md305", null ],
        [ "Technical Documentation", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md306", null ],
        [ "Standards", "md_C_docs_PHASE2_IMPLEMENTATION_NOTES.html#autotoc_md307", null ]
      ] ]
    ] ],
    [ "Phase 3 API Reference", "md_C_docs_PHASE3_API_REFERENCE.html", [
      [ "Type System Extended API", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md310", [
        [ "Struct Types", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md311", [
          [ "<tt>bci_type_struct_create</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md312", null ],
          [ "<tt>bci_type_struct_add_field</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md313", null ],
          [ "<tt>bci_type_struct_finalize</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md314", null ],
          [ "<tt>bci_type_struct_get_field</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md315", null ]
        ] ],
        [ "Union Types", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md316", [
          [ "<tt>bci_type_union_create</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md317", null ],
          [ "<tt>bci_type_union_add_variant</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md318", null ]
        ] ],
        [ "Enum Types", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md319", [
          [ "<tt>bci_type_enum_create</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md320", null ],
          [ "<tt>bci_type_enum_add_variant</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md321", null ],
          [ "<tt>bci_type_enum_get_value</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md322", null ]
        ] ],
        [ "Function Types", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md323", [
          [ "<tt>bci_type_function_create</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md324", null ],
          [ "<tt>bci_type_function_add_param</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md325", null ],
          [ "<tt>bci_type_function_matches</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md326", null ]
        ] ],
        [ "Generic Types", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md327", [
          [ "<tt>bci_type_generic_create</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md328", null ],
          [ "<tt>bci_type_generic_add_param</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md329", null ],
          [ "<tt>bci_type_generic_instantiate</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md330", null ]
        ] ],
        [ "Type Checking", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md331", [
          [ "<tt>bci_type_ext_equals</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md332", null ],
          [ "<tt>bci_type_ext_is_assignable</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md333", null ]
        ] ]
      ] ],
      [ "Parser Extended API", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md334", [
        [ "Parser Initialization", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md335", [
          [ "<tt>parser_extended_init</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md336", null ],
          [ "<tt>parser_extended_parse</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md337", null ]
        ] ],
        [ "Error Handling", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md338", [
          [ "<tt>parser_error_at</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md339", null ],
          [ "<tt>parser_synchronize</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md340", null ]
        ] ],
        [ "Pattern Matching", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md341", [
          [ "<tt>ast_new_match_expr</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md342", null ],
          [ "<tt>ast_new_pattern_wildcard</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md343", null ],
          [ "<tt>ast_new_pattern_binding</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md344", null ]
        ] ],
        [ "Lambda Expressions", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md345", [
          [ "<tt>ast_new_lambda</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md346", null ],
          [ "<tt>ast_lambda_add_param</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md347", null ],
          [ "<tt>ast_lambda_add_capture</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md348", null ]
        ] ]
      ] ],
      [ "Semantic Analysis API", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md349", [
        [ "Type Inference", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md350", [
          [ "<tt>type_inference_init</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md351", null ],
          [ "<tt>type_inference_new_var</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md352", null ],
          [ "<tt>type_inference_solve</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md353", null ]
        ] ],
        [ "Lifetime Analysis", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md354", [
          [ "<tt>lifetime_analyzer_init</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md355", null ],
          [ "<tt>lifetime_analyze_program</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md356", null ],
          [ "<tt>lifetime_is_live</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md357", null ]
        ] ],
        [ "Escape Analysis", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md358", [
          [ "<tt>escape_analyzer_init</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md359", null ],
          [ "<tt>escape_analyze_function</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md360", null ],
          [ "<tt>escape_can_stack_allocate</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md361", null ]
        ] ],
        [ "Control Flow Graph", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md362", [
          [ "<tt>cfg_init</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md363", null ],
          [ "<tt>cfg_build_from_ast</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md364", null ],
          [ "<tt>cfg_is_reachable</tt>", "md_C_docs_PHASE3_API_REFERENCE.html#autotoc_md365", null ]
        ] ]
      ] ]
    ] ],
    [ "Phase 3: Compiler Infrastructure Enhancement", "md_C_docs_PHASE3_IMPLEMENTATION.html", [
      [ "Overview", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md367", null ],
      [ "Phase 3.1: Type System Expansion", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md368", [
        [ "Struct Types", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md369", null ],
        [ "Union Types", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md370", null ],
        [ "Enum Types", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md371", null ],
        [ "Function Types", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md372", null ],
        [ "Generic Types", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md373", null ],
        [ "Array Types", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md374", null ],
        [ "Type Checking Utilities", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md375", null ]
      ] ],
      [ "Phase 3.2: Parser & AST Enhancement", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md376", [
        [ "Operator Precedence Table", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md377", null ],
        [ "Enhanced Error Recovery", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md378", null ],
        [ "Pattern Matching Support", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md379", null ],
        [ "Lambda Expressions", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md380", null ]
      ] ],
      [ "Phase 3.3: Semantic Analysis Enhancement", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md381", [
        [ "Type Inference Engine", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md382", null ],
        [ "Lifetime Analysis", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md383", null ],
        [ "Escape Analysis", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md384", null ],
        [ "Control Flow Graph", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md385", null ]
      ] ],
      [ "Testing", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md386", [
        [ "Test Coverage", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md387", null ],
        [ "Running Tests", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md388", null ]
      ] ],
      [ "Risk Mitigation", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md389", [
        [ "Feature Detection", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md390", null ],
        [ "Sanitizer Support", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md391", null ],
        [ "Performance Baselines", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md392", null ]
      ] ],
      [ "API Contracts", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md393", [
        [ "Memory Management", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md394", null ],
        [ "Error Handling", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md395", null ],
        [ "Thread Safety", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md396", null ]
      ] ],
      [ "Integration", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md397", [
        [ "With Existing Compiler", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md398", null ],
        [ "Build System Integration", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md399", null ]
      ] ],
      [ "Future Work", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md400", [
        [ "Phase 4 Considerations", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md401", null ],
        [ "Potential Enhancements", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md402", null ]
      ] ],
      [ "References", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md403", null ],
      [ "Conclusion", "md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md404", null ]
    ] ],
    [ "Device Backend API Documentation", "md_C_docs_phase5_DEVICE_BACKEND_API.html", [
      [ "Overview", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md406", null ],
      [ "Architecture", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md407", null ],
      [ "Device VTable Interface", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md408", [
        [ "Methods", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md409", null ]
      ] ],
      [ "CPU Backend", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md410", [
        [ "Features", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md411", null ],
        [ "API", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md412", null ],
        [ "Supported Operations", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md413", null ],
        [ "Example", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md414", null ]
      ] ],
      [ "GPU Backend (OpenCL)", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md415", [
        [ "Features", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md416", null ],
        [ "API", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md417", null ],
        [ "Buffer Management", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md418", null ],
        [ "OpenCL Kernel Templates", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md419", null ],
        [ "Example", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md420", null ]
      ] ],
      [ "FPGA Backend (Verilog)", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md421", [
        [ "Features", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md422", null ],
        [ "API", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md423", null ],
        [ "Verilog Generation", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md424", null ],
        [ "Resource Estimation", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md425", null ],
        [ "Example", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md426", null ]
      ] ],
      [ "Multi-Device Execution", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md427", [
        [ "Device Selection Strategy", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md428", null ],
        [ "Heterogeneous Execution", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md429", null ]
      ] ],
      [ "Performance Tuning", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md430", [
        [ "CPU Optimization", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md431", null ],
        [ "GPU Optimization", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md432", null ],
        [ "FPGA Optimization", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md433", null ]
      ] ],
      [ "Error Handling", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md434", null ],
      [ "See Also", "md_C_docs_phase5_DEVICE_BACKEND_API.html#autotoc_md435", null ]
    ] ],
    [ "Graph Optimization Guide", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html", [
      [ "Overview", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md437", null ],
      [ "Core Concepts", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md438", [
        [ "Dead Node Elimination", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md439", null ],
        [ "Constant Folding", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md440", null ],
        [ "Subgraph Fusion", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md441", null ]
      ] ],
      [ "API Reference", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md442", [
        [ "Graph Simplification", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md443", null ],
        [ "Dead Node Removal", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md444", null ],
        [ "Constant Merging", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md445", null ],
        [ "Subgraph Identification", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md446", null ],
        [ "Subgraph Fusion", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md447", null ]
      ] ],
      [ "Serialization Format", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md448", [
        [ "Binary Format Specification", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md449", null ],
        [ "Serialization API", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md450", null ],
        [ "Deserialization API", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md451", null ]
      ] ],
      [ "Optimization Strategies", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md452", [
        [ "When to Optimize", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md453", null ],
        [ "Optimization Pipeline", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md454", null ]
      ] ],
      [ "Performance Considerations", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md455", [
        [ "Time Complexity", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md456", null ],
        [ "Space Complexity", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md457", null ]
      ] ],
      [ "Best Practices", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md458", [
        [ "1. Optimize Early", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md459", null ],
        [ "2. Cache Optimized Graphs", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md460", null ],
        [ "3. Profile Before Fusion", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md461", null ]
      ] ],
      [ "Debugging", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md462", [
        [ "Validation", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md463", null ],
        [ "Visualization", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md464", null ]
      ] ],
      [ "Troubleshooting", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md465", [
        [ "Common Issues", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md466", null ]
      ] ],
      [ "Examples", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md467", [
        [ "Complete Optimization Pipeline", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md468", null ]
      ] ],
      [ "See Also", "md_C_docs_phase5_GRAPH_OPTIMIZATION_GUIDE.html#autotoc_md469", null ]
    ] ],
    [ "HAM Intelligence Guide", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html", [
      [ "Overview", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md471", null ],
      [ "Core Concepts", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md472", [
        [ "Memory Tiers", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md473", null ],
        [ "Entropy Scoring", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md474", null ]
      ] ],
      [ "Entropy-Based Scoring", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md475", [
        [ "Shannon Entropy", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md476", null ],
        [ "Access Pattern Entropy", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md477", null ],
        [ "API", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md478", null ]
      ] ],
      [ "Automatic Tier Management", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md479", [
        [ "Policy Configuration", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md480", null ],
        [ "Tier Manager", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md481", null ],
        [ "Promotion Logic", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md482", null ],
        [ "Demotion Logic", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md483", null ]
      ] ],
      [ "Compression via Motif Interning", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md484", [
        [ "Concept", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md485", null ],
        [ "API", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md486", null ],
        [ "Motif Extraction", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md487", null ],
        [ "Compression Statistics", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md488", null ]
      ] ],
      [ "NUMA Awareness", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md489", [
        [ "Concept", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md490", null ],
        [ "Affinity Computation", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md491", null ],
        [ "NUMA Manager", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md492", null ],
        [ "Migration Decision", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md493", null ]
      ] ],
      [ "Integration Example", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md494", [
        [ "Complete HAM Intelligence Pipeline", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md495", null ]
      ] ],
      [ "Performance Metrics", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md496", [
        [ "Entropy Computation", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md497", null ],
        [ "Tier Management", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md498", null ],
        [ "Compression", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md499", null ],
        [ "NUMA Optimization", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md500", null ]
      ] ],
      [ "Best Practices", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md501", [
        [ "1. Tune Policy Thresholds", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md502", null ],
        [ "2. Monitor Compression Ratios", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md503", null ],
        [ "3. Balance NUMA Migration", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md504", null ]
      ] ],
      [ "See Also", "md_C_docs_phase5_HAM_INTELLIGENCE_GUIDE.html#autotoc_md505", null ]
    ] ],
    [ "Phase 5: Kernel Enhancement - Implementation Overview", "md_C_docs_phase5_PHASE5_OVERVIEW.html", [
      [ "Executive Summary", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md507", null ],
      [ "Architecture Overview", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md508", null ],
      [ "Implementation Status", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md509", null ],
      [ "Key Features", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md510", [
        [ "Phase 5.1: Graph Optimization", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md511", null ],
        [ "Phase 5.2: Device Backend Implementation", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md512", null ],
        [ "Phase 5.3: Scheduler Implementation", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md513", null ],
        [ "Phase 5.4: HAM Intelligence", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md514", null ]
      ] ],
      [ "C23 Features Used", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md515", null ],
      [ "Performance Characteristics", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md516", null ],
      [ "Integration Points", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md517", [
        [ "With Existing BDI Components", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md518", null ],
        [ "External Dependencies", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md519", null ]
      ] ],
      [ "Testing Strategy", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md520", [
        [ "Test Coverage", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md521", null ],
        [ "Test Organization", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md522", null ]
      ] ],
      [ "Build Instructions", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md523", null ],
      [ "API Examples", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md524", [
        [ "Graph Optimization", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md525", null ],
        [ "Device Backend", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md526", null ],
        [ "Scheduler", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md527", null ],
        [ "HAM Intelligence", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md528", null ]
      ] ],
      [ "Future Enhancements", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md529", [
        [ "Short Term", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md530", null ],
        [ "Long Term", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md531", null ]
      ] ],
      [ "References", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md532", null ],
      [ "Contributors", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md533", null ],
      [ "License", "md_C_docs_phase5_PHASE5_OVERVIEW.html#autotoc_md534", null ]
    ] ],
    [ "Scheduler Implementation Guide", "md_C_docs_phase5_SCHEDULER_GUIDE.html", [
      [ "Overview", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md536", null ],
      [ "Scheduler Comparison", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md537", null ],
      [ "Wavefront Scheduler", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md538", [
        [ "Concept", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md539", null ],
        [ "API", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md540", null ],
        [ "Example", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md541", null ],
        [ "Performance Characteristics", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md542", null ]
      ] ],
      [ "Work Stealing Scheduler", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md543", [
        [ "Concept", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md544", null ],
        [ "Lock-Free Queue", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md545", null ],
        [ "API", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md546", null ],
        [ "Example", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md547", null ],
        [ "Performance Characteristics", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md548", null ]
      ] ],
      [ "Priority Scheduler", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md549", [
        [ "Concept", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md550", null ],
        [ "API", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md551", null ],
        [ "Example", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md552", null ],
        [ "Priority Calculation", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md553", null ],
        [ "Performance Characteristics", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md554", null ]
      ] ],
      [ "Scheduler Selection Guide", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md555", [
        [ "Use Wavefront When:", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md556", null ],
        [ "Use Work Stealing When:", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md557", null ],
        [ "Use Priority When:", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md558", null ]
      ] ],
      [ "Advanced Topics", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md559", [
        [ "Hybrid Scheduling", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md560", null ],
        [ "Dynamic Scheduling", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md561", null ]
      ] ],
      [ "Debugging", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md562", [
        [ "Execution Trace", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md563", null ],
        [ "Deadlock Detection", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md564", null ]
      ] ],
      [ "See Also", "md_C_docs_phase5_SCHEDULER_GUIDE.html#autotoc_md565", null ]
    ] ],
    [ "Generational GC Fix Summary - PR #103", "md_C_GC_FIX_SUMMARY.html", [
      [ "✅ Status: COMPLETE", "md_C_GC_FIX_SUMMARY.html#autotoc_md567", null ],
      [ "🔧 Merge Conflicts Resolved", "md_C_GC_FIX_SUMMARY.html#autotoc_md569", [
        [ "1. <tt>add_forwarding_entry</tt> Function Signature", "md_C_GC_FIX_SUMMARY.html#autotoc_md570", null ],
        [ "2. Algorithm Structure", "md_C_GC_FIX_SUMMARY.html#autotoc_md571", null ]
      ] ],
      [ "🐛 Three Critical Bugs Fixed", "md_C_GC_FIX_SUMMARY.html#autotoc_md573", [
        [ "Bug 1 (P0): Record forwarding when promoting in overflow fallback", "md_C_GC_FIX_SUMMARY.html#autotoc_md574", null ],
        [ "Bug 2 (P1): Do not advance compaction pointer for promoted objects", "md_C_GC_FIX_SUMMARY.html#autotoc_md576", null ],
        [ "Bug 3 (P1): Update actual pointer fields in old objects", "md_C_GC_FIX_SUMMARY.html#autotoc_md578", null ]
      ] ],
      [ "🏗️ New Algorithm: 4-Pass Promotion-First Strategy", "md_C_GC_FIX_SUMMARY.html#autotoc_md580", [
        [ "PASS 1: PROMOTION PHASE", "md_C_GC_FIX_SUMMARY.html#autotoc_md581", null ],
        [ "PASS 2: COMPACTION FEASIBILITY CHECK", "md_C_GC_FIX_SUMMARY.html#autotoc_md582", null ],
        [ "PASS 3: COMPACTION PHASE (if enabled)", "md_C_GC_FIX_SUMMARY.html#autotoc_md583", null ],
        [ "PASS 4: REFERENCE UPDATE PHASE", "md_C_GC_FIX_SUMMARY.html#autotoc_md584", null ]
      ] ],
      [ "✨ Key Improvements", "md_C_GC_FIX_SUMMARY.html#autotoc_md586", null ],
      [ "📊 Changes Summary", "md_C_GC_FIX_SUMMARY.html#autotoc_md588", null ],
      [ "✅ Verification Checklist", "md_C_GC_FIX_SUMMARY.html#autotoc_md590", null ],
      [ "🧪 Testing Recommendations", "md_C_GC_FIX_SUMMARY.html#autotoc_md592", null ],
      [ "⚠️ Known Limitations", "md_C_GC_FIX_SUMMARY.html#autotoc_md594", [
        [ "Bug 3 (P1) - Partial Fix", "md_C_GC_FIX_SUMMARY.html#autotoc_md595", null ]
      ] ],
      [ "📝 Next Steps", "md_C_GC_FIX_SUMMARY.html#autotoc_md597", null ],
      [ "🔗 Important Links", "md_C_GC_FIX_SUMMARY.html#autotoc_md599", null ],
      [ "📞 GitHub App Permissions", "md_C_GC_FIX_SUMMARY.html#autotoc_md601", null ]
    ] ],
    [ "blueprint", "md_C_kernel_blueprint.html", null ],
    [ "readme", "md_C_readme.html", null ],
    [ "Data Structures", "annotated.html", [
      [ "Data Structures", "annotated.html", "annotated_dup" ],
      [ "Data Structure Index", "classes.html", null ],
      [ "Data Fields", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Variables", "functions_vars.html", "functions_vars" ],
        [ "Enumerator", "functions_eval.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "Globals", "globals.html", [
        [ "All", "globals.html", "globals_dup" ],
        [ "Functions", "globals_func.html", "globals_func" ],
        [ "Variables", "globals_vars.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", "globals_eval" ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"adam_8c.html",
"bci__conversion_8c.html#ac10f30924d7883a2a0f9c2fd4ccd6e00",
"bci__types__extended_8c.html#a648b5dd47b3c369e362245ea1995cd94",
"btl__scheduler_8h.html#a744948ef5e28062be0cdb6fc6c11c9b4",
"cpu__backend_8h.html#ac9476b902b35f0cae846956a082fd9fe",
"generational__gc_8h.html#a0d105a85c95fd4ce55cf35bc1ca7c0be",
"graph__builder_8h.html#a8abc7e1be6674e9010940c10664e405a",
"graph__optimizer_8h.html#a41d0dfc7deacf0315e2a01a5e66fc4a5",
"kernel_2graph_2graph_8h.html#a4aa45ef1b682c42293aeb6983f38da22",
"md_C_C_analysis_plan.html#autotoc_md187",
"md_C_docs_PHASE3_IMPLEMENTATION.html#autotoc_md385",
"process_8_8h.html#a373a58178f69d5e3e1de7516d105675eab3e832a1cd148e87b95a720e6d38a5a7",
"structBTL__InstructionNode.html#a7a78cb30e53ae2bbfa831db78a330a55",
"structConfusionMatrix.html#ab8e749eba11371ff7d441d4a4a5f4a46",
"structGraphExecutionResult.html#a424117473ccd06819ca7950a898b164e",
"structGraphVMStats.html#a050ea68086c4d5ec1ae9f0872a361755",
"structParseError.html",
"syscalls_8h.html#ac74eaabd7e98c91769333101f50d0458",
"vm_8c.html#a86cb8f97e15803db82ea6990e251fe64"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';