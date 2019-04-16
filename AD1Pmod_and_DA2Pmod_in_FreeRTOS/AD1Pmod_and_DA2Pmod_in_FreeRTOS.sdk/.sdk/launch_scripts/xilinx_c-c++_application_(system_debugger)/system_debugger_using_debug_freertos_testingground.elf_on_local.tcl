connect -url tcp:127.0.0.1:3121
source C:/Zynq_Book/AD1Pmod_and_DA2Pmod_in_FreeRTOS/AD1Pmod_and_DA2Pmod_in_FreeRTOS.sdk/design_1_wrapper_hw_platform_0/ps7_init.tcl
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Zybo 210279771849A"} -index 0
loadhw -hw C:/Zynq_Book/AD1Pmod_and_DA2Pmod_in_FreeRTOS/AD1Pmod_and_DA2Pmod_in_FreeRTOS.sdk/design_1_wrapper_hw_platform_0/system.hdf -mem-ranges [list {0x40000000 0xbfffffff}]
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Zybo 210279771849A"} -index 0
stop
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "ARM*#0" && jtag_cable_name =~ "Digilent Zybo 210279771849A"} -index 0
rst -processor
targets -set -nocase -filter {name =~ "ARM*#0" && jtag_cable_name =~ "Digilent Zybo 210279771849A"} -index 0
dow C:/Zynq_Book/AD1Pmod_and_DA2Pmod_in_FreeRTOS/AD1Pmod_and_DA2Pmod_in_FreeRTOS.sdk/freeRTOS_testingground/Debug/freeRTOS_testingground.elf
configparams force-mem-access 0
targets -set -nocase -filter {name =~ "ARM*#0" && jtag_cable_name =~ "Digilent Zybo 210279771849A"} -index 0
con
