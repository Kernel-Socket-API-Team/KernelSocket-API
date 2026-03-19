# 0 "/workspace/compileLibraryForLinux/build/src/common/linux/linux_common.c"
# 1 "/usr/src/linux-headers-6.8.0-101-generic//"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "././include/linux/compiler-version.h" 1
# 0 "<command-line>" 2
# 1 "././include/linux/kconfig.h" 1




# 1 "./include/generated/autoconf.h" 1
# 6 "././include/linux/kconfig.h" 2
# 0 "<command-line>" 2
# 1 "././include/linux/compiler_types.h" 1
# 89 "././include/linux/compiler_types.h"
# 1 "./include/linux/compiler_attributes.h" 1
# 90 "././include/linux/compiler_types.h" 2
# 151 "././include/linux/compiler_types.h"
# 1 "./include/linux/compiler-gcc.h" 1
# 152 "././include/linux/compiler_types.h" 2
# 168 "././include/linux/compiler_types.h"
struct ftrace_branch_data {
 const char *func;
 const char *file;
 unsigned line;
 union {
  struct {
   unsigned long correct;
   unsigned long incorrect;
  };
  struct {
   unsigned long miss;
   unsigned long hit;
  };
  unsigned long miss_hit[2];
 };
};

struct ftrace_likely_data {
 struct ftrace_branch_data data;
 unsigned long constant;
};
# 0 "<command-line>" 2
# 1 "/workspace/compileLibraryForLinux/build/src/common/linux/linux_common.c"
