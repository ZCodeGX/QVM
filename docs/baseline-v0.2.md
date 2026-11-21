-- The C compiler identification is GNU 15.2.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin/cc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Configuring done (0.2s)
-- Generating done (0.0s)
-- Build files have been written to: /home/amine/Documentos/QVM/QVM/build
[  3%] Building C object CMakeFiles/qvm_core.dir/src/core/types.c.o
[  3%] Building C object CMakeFiles/qvm_core.dir/src/core/value.c.o
[  3%] Building C object CMakeFiles/qvm_core.dir/src/core/limits.c.o
[  4%] Building C object CMakeFiles/qvm_core.dir/src/core/error.c.o
[  5%] Building C object CMakeFiles/qvm_core.dir/src/core/config.c.o
[  7%] Building C object CMakeFiles/qvm_core.dir/src/core/context.c.o
[  8%] Building C object CMakeFiles/qvm_core.dir/src/core/lifecycle.c.o
[  9%] Building C object CMakeFiles/qvm_core.dir/src/core/type_table.c.o
[ 10%] Building C object CMakeFiles/qvm_core.dir/src/core/version.c.o
[ 11%] Building C object CMakeFiles/qvm_core.dir/src/frames/frames.c.o
[ 12%] Building C object CMakeFiles/qvm_core.dir/src/registers/registers.c.o
[ 14%] Building C object CMakeFiles/qvm_core.dir/src/instruction/instruction.c.o
[ 15%] Building C object CMakeFiles/qvm_core.dir/src/instruction/decode.c.o
[ 16%] Building C object CMakeFiles/qvm_core.dir/src/instruction/execute.c.o
[ 17%] Building C object CMakeFiles/qvm_core.dir/src/memory/gcq/gcq.c.o
[ 18%] Building C object CMakeFiles/qvm_core.dir/src/runtime/runtime.c.o
[ 20%] Building C object CMakeFiles/qvm_core.dir/src/runtime/runtime_fast.c.o
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c: In function ‘qvm_runtime_execute_fast’:
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:23:9: warning: taking the address of a label is non-standard [-Wpedantic]
   23 |         [QVM_OP_ARITH_ADD] = &&LABEL_ADD,
      |         ^
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:24:9: warning: taking the address of a label is non-standard [-Wpedantic]
   24 |         [QVM_OP_CMP_LT] = &&LABEL_CMP_LT,
      |         ^
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:25:9: warning: taking the address of a label is non-standard [-Wpedantic]
   25 |         [QVM_OP_JMP_IF] = &&LABEL_JMP_IF,
      |         ^
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:26:9: warning: taking the address of a label is non-standard [-Wpedantic]
   26 |         [QVM_OP_LOAD_PTR] = &&LABEL_LOAD_PTR,
      |         ^
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:27:9: warning: taking the address of a label is non-standard [-Wpedantic]
   27 |         [QVM_OP_STORE_PTR] = &&LABEL_STORE_PTR,
      |         ^
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:28:9: warning: taking the address of a label is non-standard [-Wpedantic]
   28 |         [QVM_OP_RETURN] = &&LABEL_RETURN
      |         ^
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:31:24: warning: ISO C forbids ‘goto *expr;’ [-Wpedantic]
   31 |     #define DISPATCH() goto *dispatch_table[bc[pc]]
      |                        ^~~~
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:33:5: note: in expansion of macro ‘DISPATCH’
   33 |     DISPATCH();
      |     ^~~~~~~~
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:31:24: warning: ISO C forbids ‘goto *expr;’ [-Wpedantic]
   31 |     #define DISPATCH() goto *dispatch_table[bc[pc]]
      |                        ^~~~
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:41:9: note: in expansion of macro ‘DISPATCH’
   41 |         DISPATCH();
      |         ^~~~~~~~
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:31:24: warning: ISO C forbids ‘goto *expr;’ [-Wpedantic]
   31 |     #define DISPATCH() goto *dispatch_table[bc[pc]]
      |                        ^~~~
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:50:9: note: in expansion of macro ‘DISPATCH’
   50 |         DISPATCH();
      |         ^~~~~~~~
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:31:24: warning: ISO C forbids ‘goto *expr;’ [-Wpedantic]
   31 |     #define DISPATCH() goto *dispatch_table[bc[pc]]
      |                        ^~~~
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:61:9: note: in expansion of macro ‘DISPATCH’
   61 |         DISPATCH();
      |         ^~~~~~~~
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:31:24: warning: ISO C forbids ‘goto *expr;’ [-Wpedantic]
   31 |     #define DISPATCH() goto *dispatch_table[bc[pc]]
      |                        ^~~~
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:73:9: note: in expansion of macro ‘DISPATCH’
   73 |         DISPATCH();
      |         ^~~~~~~~
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:31:24: warning: ISO C forbids ‘goto *expr;’ [-Wpedantic]
   31 |     #define DISPATCH() goto *dispatch_table[bc[pc]]
      |                        ^~~~
/home/amine/Documentos/QVM/QVM/src/runtime/runtime_fast.c:85:9: note: in expansion of macro ‘DISPATCH’
   85 |         DISPATCH();
      |         ^~~~~~~~
[ 21%] Building C object CMakeFiles/qvm_core.dir/src/loader/loader.c.o
[ 22%] Building C object CMakeFiles/qvm_core.dir/src/verifier/verifier.c.o
[ 23%] Building C object CMakeFiles/qvm_core.dir/src/verifier/verifier_qms.c.o
[ 24%] Building C object CMakeFiles/qvm_core.dir/src/string/string.c.o
[ 25%] Building C object CMakeFiles/qvm_core.dir/src/ffi/ffi.c.o
[ 27%] Building C object CMakeFiles/qvm_core.dir/src/jit/jit.c.o
[ 28%] Building C object CMakeFiles/qvm_core.dir/src/qit/qit.c.o
/home/amine/Documentos/QVM/QVM/src/qit/qit.c: In function ‘compile_jmp_if’:
/home/amine/Documentos/QVM/QVM/src/qit/qit.c:65:73: warning: unused parameter ‘pc’ [-Wunused-parameter]
   65 | static void compile_jmp_if(qit_code *qit, const uint8_t *insn, uint32_t pc) {
      |                                                                ~~~~~~~~~^~
[ 29%] Linking C static library libqvm_core.a
[ 29%] Built target qvm_core
[ 34%] Building C object CMakeFiles/test_lifecycle.dir/tests/core/test_lifecycle.c.o
[ 34%] Building C object CMakeFiles/test_type_table.dir/tests/core/test_type_table.c.o
[ 34%] Building C object CMakeFiles/test_frames.dir/tests/frames/test_frames.c.o
[ 34%] Building C object CMakeFiles/test_registers.dir/tests/registers/test_registers.c.o
[ 36%] Linking C executable test_lifecycle
[ 36%] Linking C executable test_frames
[ 37%] Linking C executable test_registers
[ 38%] Linking C executable test_type_table
[ 38%] Built target test_lifecycle
[ 38%] Built target test_frames
[ 38%] Built target test_registers
[ 38%] Built target test_type_table
[ 41%] Building C object CMakeFiles/test_decode.dir/tests/instruction/test_decode.c.o
[ 41%] Building C object CMakeFiles/test_instruction.dir/tests/instruction/test_instruction.c.o
[ 42%] Building C object CMakeFiles/test_execute.dir/tests/instruction/test_execute.c.o
[ 43%] Building C object CMakeFiles/test_string.dir/tests/string/test_string.c.o
[ 44%] Linking C executable test_decode
[ 45%] Linking C executable test_instruction
[ 47%] Linking C executable test_string
[ 48%] Linking C executable test_execute
[ 48%] Built target test_decode
[ 48%] Built target test_instruction
[ 48%] Built target test_string
[ 49%] Building C object CMakeFiles/test_ffi.dir/tests/ffi/test_ffi.c.o
[ 50%] Building C object CMakeFiles/test_gcq.dir/tests/memory/test_gcq.c.o
[ 50%] Built target test_execute
[ 51%] Building C object CMakeFiles/test_runtime.dir/tests/runtime/test_runtime.c.o
[ 52%] Building C object CMakeFiles/test_call_return.dir/tests/runtime/test_call_return.c.o
[ 54%] Linking C executable test_ffi
[ 55%] Linking C executable test_gcq
[ 56%] Linking C executable test_runtime
[ 56%] Built target test_ffi
[ 56%] Built target test_gcq
[ 56%] Built target test_runtime
[ 57%] Building C object CMakeFiles/test_qms_runtime.dir/tests/runtime/test_qms_runtime.c.o
[ 58%] Building C object CMakeFiles/test_loader.dir/tests/loader/test_loader.c.o
[ 60%] Building C object CMakeFiles/test_verifier.dir/tests/verifier/test_verifier.c.o
[ 61%] Linking C executable test_call_return
[ 62%] Linking C executable test_qms_runtime
[ 63%] Linking C executable test_verifier
[ 64%] Linking C executable test_loader
[ 64%] Built target test_qms_runtime
[ 64%] Built target test_call_return
[ 64%] Built target test_verifier
[ 64%] Built target test_loader
[ 65%] Building C object CMakeFiles/test_qit.dir/tests/qit/test_qit.c.o
[ 67%] Building C object CMakeFiles/test_qms_zero.dir/tests/verifier/test_qms_zero.c.o
[ 68%] Building C object CMakeFiles/demo.dir/examples/demo.c.o
[ 69%] Building C object CMakeFiles/demo_ffi.dir/examples/demo_ffi.c.o
/home/amine/Documentos/QVM/QVM/tests/qit/test_qit.c: In function ‘test_qit_execute_arith’:
/home/amine/Documentos/QVM/QVM/tests/qit/test_qit.c:71:5: error: unknown type name ‘qit_func_t’
   71 |     qit_func_t func = (qit_func_t)qit_get_entry(code);
      |     ^~~~~~~~~~
/home/amine/Documentos/QVM/QVM/tests/qit/test_qit.c:71:24: error: ‘qit_func_t’ undeclared (first use in this function)
   71 |     qit_func_t func = (qit_func_t)qit_get_entry(code);
      |                        ^~~~~~~~~~
/home/amine/Documentos/QVM/QVM/tests/qit/test_qit.c:71:24: note: each undeclared identifier is reported only once for each function it appears in
/home/amine/Documentos/QVM/QVM/tests/qit/test_qit.c:71:35: error: expected ‘,’ or ‘;’ before ‘qit_get_entry’
   71 |     qit_func_t func = (qit_func_t)qit_get_entry(code);
      |                                   ^~~~~~~~~~~~~
/home/amine/Documentos/QVM/QVM/tests/qit/test_qit.c:73:22: error: called object ‘func’ is not a function or function pointer
   73 |     int64_t result = func(frame);
      |                      ^~~~
/home/amine/Documentos/QVM/QVM/tests/qit/test_qit.c:71:16: note: declared here
   71 |     qit_func_t func = (qit_func_t)qit_get_entry(code);
      |                ^~~~
gmake[2]: *** [CMakeFiles/test_qit.dir/build.make:79: CMakeFiles/test_qit.dir/tests/qit/test_qit.c.o] Error 1
gmake[1]: *** [CMakeFiles/Makefile2:721: CMakeFiles/test_qit.dir/all] Error 2
gmake[1]: *** Se espera a que terminen otras tareas....
[ 70%] Linking C executable test_qms_zero
[ 71%] Linking C executable demo
[ 71%] Built target test_qms_zero
[ 72%] Linking C executable demo_ffi
[ 72%] Built target demo
[ 72%] Built target demo_ffi
gmake: *** [Makefile:101: all] Error 2
