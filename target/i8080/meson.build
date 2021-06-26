i8080_ss = ss.source_set()
i8080_ss.add(files(
  'cpu.c',
  'helper.c',
  'op_helper.c',
  'translate.c',
  'gdbstub.c',
))

i8080_softmmu_ss = ss.source_set()

target_arch += {'i8080': i8080_ss}
target_softmmu_arch += {'i8080': i8080_softmmu_ss}
