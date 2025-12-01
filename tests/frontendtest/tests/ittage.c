/*
 * ======================================================================================
 * ITTAGE Microbenchmark (RISC-V)
 * ======================================================================================
 * Platform: RISC-V (RV64GC or RV64IMAC)
 * Compiler: Nexus AM Build System (based on GCC)
 * ======================================================================================
 */

// Minimal definitions to replace am.h/klib.h for this test
#define NULL ((void *)0)

// 强制生成 RISC-V 分支指令，用于构造全局历史 (GHR)
#define FORCE_BRANCH(cond)                                                     \
  __asm__ volatile("bnez %0, 1f \n\t" /* Taken jump to label 1 */              \
                   "nop         \n\t" /* Not Taken path */                     \
                   "1:          \n\t" /* Target */                             \
                   "nop         \n\t" /* Padding */                            \
                   :                                                           \
                   : "r"(cond)                                                 \
                   : "memory")

// 强制生成间接跳转指令 (jalr)
// 使用 x1 (ra) 作为链接寄存器，模拟函数调用
// 从 clobber list 中移除 "ra" 以避免非 RISC-V 环境下的诊断错误
#define FORCE_INDIRECT_CALL(target_addr)                                       \
  __asm__ volatile("jalr x1, 0(%0) \n\t" : : "r"(target_addr) : "memory", "c"  \
                                                                          "c")

// 定义几个简单的目标函数
// 使用 noinline 防止内联，确保有独立的地址
void __attribute__((noinline)) target_1() { __asm__ volatile("nop"); }
void __attribute__((noinline)) target_2() { __asm__ volatile("nop"); }
void __attribute__((noinline)) target_3() { __asm__ volatile("nop"); }
void __attribute__((noinline)) target_4() { __asm__ volatile("nop"); }

typedef void (*func_ptr_t)();

// --- 辅助函数：在波形中制造显著的间隔 ---
void run_separator() {
  for (int i = 0; i < 50; i++) {
    FORCE_BRANCH(1);
  }
}

// ---------------------------------------------------------
// Test 1: Simple Correlation (1 Branch -> 2 Targets)
// History: T -> Target 1, NT -> Target 2
// ---------------------------------------------------------
void test_simple_correlation() {
  int iterations = 2000;
  func_ptr_t t1 = target_1;
  func_ptr_t t2 = target_2;

  for (int i = 0; i < iterations; i++) {
    // 产生一个随机模式，或者简单的交替模式
    // 这里使用交替模式：T, NT, T, NT...
    int cond = (i % 2 != 0);

    // 1. 更新 GHR
    FORCE_BRANCH(cond);

    // 2. 根据条件选择目标
    // ITTAGE 应该能学会：当 GHR 最新一位是 1 时跳转到 t1，为 0 时跳转到 t2
    func_ptr_t target = cond ? t1 : t2;

    // 3. 执行间接跳转
    FORCE_INDIRECT_CALL(target);
  }
}

// ---------------------------------------------------------
// Test 2: Complex Correlation (2 Branches -> 4 Targets)
// History: TT -> T1, TN -> T2, NT -> T3, NN -> T4
// ---------------------------------------------------------
void test_complex_correlation() {
  int iterations = 4000;
  func_ptr_t targets[4] = {target_1, target_2, target_3, target_4};

  for (int i = 0; i < iterations; i++) {
    // 产生 0-3 的模式
    int pattern = i % 4;
    // pattern 0: 00 (NN)
    // pattern 1: 01 (NT)
    // pattern 2: 10 (TN)
    // pattern 3: 11 (TT)

    int cond1 = (pattern >> 1) & 1;
    int cond2 = (pattern & 1);

    // 1. 更新 GHR (推入两位历史)
    FORCE_BRANCH(cond1);
    FORCE_BRANCH(cond2);

    // 2. 选择目标
    // 注意：pattern 与 targets 的映射关系
    // pattern 0 (00) -> targets[0]
    // pattern 1 (01) -> targets[1]
    // ...
    func_ptr_t target = targets[pattern];

    // 3. 执行间接跳转
    FORCE_INDIRECT_CALL(target);
  }
}

// ---------------------------------------------------------
// Test 3: Long History / Path Correlation
// Pattern: A sequence of branches determines the target
// ---------------------------------------------------------
void test_path_correlation() {
  int iterations = 2000;
  func_ptr_t t1 = target_1;
  func_ptr_t t2 = target_2;

  // 模式：
  // Path A: 1, 1, 1, 0 -> Target 1
  // Path B: 0, 0, 0, 1 -> Target 2

  for (int i = 0; i < iterations; i++) {
    int is_path_a = (i % 2 == 0);

    if (is_path_a) {
      FORCE_BRANCH(1);
      FORCE_BRANCH(1);
      FORCE_BRANCH(1);
      FORCE_BRANCH(0);
      FORCE_INDIRECT_CALL(t1);
    } else {
      FORCE_BRANCH(0);
      FORCE_BRANCH(0);
      FORCE_BRANCH(0);
      FORCE_BRANCH(1);
      FORCE_INDIRECT_CALL(t2);
    }
  }
}

// ---------------------------------------------------------
// Test 4: Stability Test (Same Target, Varying History)
// 测试 ITTAGE
// 是否会在历史变化但目标不变时保持稳定（或者是否会因为历史变化而产生不必要的别名/冲突）
// ---------------------------------------------------------
void test_stability() {
  int iterations = 2000;
  func_ptr_t t1 = target_1;

  for (int i = 0; i < iterations; i++) {
    // 随机或变化的条件，但目标始终是 t1
    int cond = (i % 3 == 0);
    FORCE_BRANCH(cond);

    FORCE_INDIRECT_CALL(t1);
  }
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------
int main() {
  test_simple_correlation();
  run_separator();

  test_complex_correlation();
  run_separator();

  test_path_correlation();
  run_separator();

  test_stability();

  return 0;
}
