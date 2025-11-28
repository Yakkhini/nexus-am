#include "common.h"
#include "am.h"
#include "klib.h"

__attribute__((optimize("O0")))
void resolve_test(int cnt) {
  volatile int A = 0;
  volatile int B = 1;
  volatile int C = 0;
  volatile int tmp = 0;

  while (cnt--) {
    if (A == B) {
      if (C > 0) {
        A = 0;
      }

      tmp = C;
      C = 0;
    } else {
      C = tmp + 1;
      A = 1;
    }
  }

  // asm volatile(
  //     // 初始化计数器
  //     "li t0, 0\n\t"      // 循环计数器
  //     "li t2, 0\n\t"      // 用于产生交替模式的计数器
  //     ".align 4\n\t"
  //     "1:\n\t"
  //     // 重复执行分支指令
  //     "li t3, 0\n\t"         // A = 0
  //     "li t4, 1\n\t"         // B = 1
  //     "beq t3, t4, 2f\n\t"   // if (A == B)
  //     "nop\n\t"              // 延迟槽
  //     // else 分支
  //     "li t5, 1\n\t"         // C = 1
  //     "j 3f\n\t"
  //     "nop\n\t"
  //     // if 分支
  //     "2:\n\t"
  //     "li t6, 0\n\t"         // C = 0
  //     "3:\n\t"
  //     "addi t0, t0, 1\n\t"   // 增加循环计数器
  //     "blt t0, %0, 1b\n\t"   // 循环控制
  //     :
  //     : "r"(cnt)
  //     : "t0", "t2", "t3", "t4", "t5", "t6", "memory"
  // );
}

int main() {
  resolve_test(100);
  return 0;
}
