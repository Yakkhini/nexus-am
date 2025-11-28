#include "am.h"
#include "klib.h"

/*
 * ======================================================================================
 * TAGE Microbenchmark (RISC-V)
 * ======================================================================================
 * Platform: RISC-V (RV64GC or RV64IMAC)
 * Compiler: Nexus AM Build System (based on GCC)
 * ======================================================================================
 */

// 强制生成 RISC-V 分支指令
// bnez: Branch if Not Equal to Zero
#define FORCE_BRANCH(cond)                                                     \
  __asm__ volatile("bnez %0, 1f \n\t" /* Taken jump to label 1 */              \
                   "nop         \n\t" /* Not Taken path */                     \
                   "1:          \n\t" /* Target */                             \
                   "nop         \n\t" /* Padding */                            \
                   :                                                           \
                   : "r"(cond)                                                 \
                   : "memory")

// --- 辅助函数：在波形中制造显著的间隔 ---
// 执行 50 次 Always Taken，方便在波形中看到一个"高电平"长条，分割不同测试
void run_separator() {
  for (int i = 0; i < 50; i++) {
    FORCE_BRANCH(1);
  }
}

// ---------------------------------------------------------
// Test 1: Bimodal / Base Predictor (1-0-1-0)
// ---------------------------------------------------------
void test_bimodal_toggle() {
  int iterations = 4000;
  for (int i = 0; i < iterations; i++) {
    int cond = (i % 2 != 0);
    FORCE_BRANCH(cond);
  }
}

// ---------------------------------------------------------
// Test 2: Geometric History (Len=20)
// ---------------------------------------------------------
void test_long_history() {
  int pattern[] = {1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0};
  int len = 20;
  int iterations = 10000;

  for (int i = 0; i < iterations; i++) {
    int cond = pattern[i % len];
    FORCE_BRANCH(cond);
  }
}

// ---------------------------------------------------------
// Test 3: Path History Recursion
// ---------------------------------------------------------
volatile int g_dummy = 0;
void recursive_brancher(int depth) {
  if (depth == 0)
    return;
  int cond1 = (depth % 3 == 0);
  FORCE_BRANCH(cond1);
  recursive_brancher(depth - 1);
  int cond2 = (depth % 2 == 0);
  FORCE_BRANCH(cond2);
}

void test_path_interference() {
  int iterations = 500; // 递归产生的分支数很多，循环次数减少
  for (int i = 0; i < iterations; i++) {
    recursive_brancher(10);
  }
}

// ---------------------------------------------------------
// Test 4: Useful Bit Set (Provider wins, Alt fails)
// ---------------------------------------------------------
void test_ubit_set() {
  int pattern[] = {1, 1, 0};
  int len = 3;
  int iterations = 3000;

  for (int i = 0; i < iterations; i++) {
    int cond = pattern[i % len];
    FORCE_BRANCH(cond);
  }
}

// ---------------------------------------------------------
// Test 5: Useful Bit Decay
// ---------------------------------------------------------
void test_ubit_decay() {
  int pattern[] = {1, 1, 1, 0, 0};
  int len = 5;

  // 1. Train
  for (int i = 0; i < 2000; i++) {
    int cond = pattern[i % len];
    FORCE_BRANCH(cond);
  }

  // 2. Aging (触发 Reset)
  // 根据你的 TAGE 参数，这里的次数可能需要调整 (e.g., 256K)
  int age_cycles = 100000;
  for (int i = 0; i < age_cycles; i++) {
    FORCE_BRANCH(1);
  }

  // 3. Check
  for (int i = 0; i < 20; i++) {
    int cond = pattern[i % len];
    FORCE_BRANCH(cond);
  }
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------
int main(const char *args) {

  const char *setting_name = args;
  if (args == NULL || strcmp(args, "") == 0) {
    setting_name = "batch";
  }

  // 顺序执行所有测试
  if (strcmp(setting_name, "batch") != 0) {
    test_bimodal_toggle();
    run_separator();

    test_long_history();
    run_separator();

    test_path_interference();
    run_separator();

    test_ubit_set();
    run_separator();

    test_ubit_decay();
  }

  if (strcmp(setting_name, "bimodal") == 0) {
    test_bimodal_toggle();
  }

  if (strcmp(setting_name, "longhist") == 0) {
    test_long_history();
  }

  if (strcmp(setting_name, "path") == 0) {
    test_path_interference();
  }

  if (strcmp(setting_name, "ubit") == 0) {
    test_ubit_set();
    run_separator();
    test_ubit_decay();
  }

  return 0;
}
