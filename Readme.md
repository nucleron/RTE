# YAPLC/RTE
This is YAPLC runtime environment project.

# Using stm32f4-discovery board
Target "yaplc" can be used to build and upload aplication to the board.

  If you want to run it, then folow these steps:
1. Build yaplc-runtime project and load it to the board (I do it with
st-util on debug session start, I think OpenOCD may also be used).
2. Create a project with "yaplc" target,
3. build it,
4. connect to your target (see
https://github.com/nucleron/RTE/blob/master/src/bsp/nuc-227-dev/plc_config.h#L27
for details)
5. and transfer aplication.
6. Now you can program your discovery board with YAPLC/IDE.

 And yes, to use YAPLC/IDE with stock discovery you should connect Boot0 and
VDD pins with jumper just before pressing "Transfer PLC" button in
YAPLC/IDE.

When aplication has been loaded - disconnect Boot0 from VDD.

Or alternatively, you can add "mosfet with memory capasitor" circuit switch
Boot0 automatically (see
https://github.com/nucleron/RTE/blob/master/src/bsp/nuc-227-dev/plc_hw.c#L54
and
https://github.com/nucleron/RTE/blob/master/src/bsp/nuc-227-dev/plc_config.h#L44
for details)
