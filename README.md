# TXV_HedgehogFirmware

Firmware for the Hedgehog Educational Robotics Controller, modified for the TU Wien Space Team small scale rocket engine test stand

### Importing Project in Atollic TrueSTUDIO
1. Copy/clone project into Workspace
2. File > New > Makefile Project with existing code:
    * Browse, select HedgehogFirmware folder in Workspace
    * Toolchain for Indexer Settings = Atollic ARM Tools
3. Project Properties > C/C++ Build > Tool Chain Editor:
    * Current Builder = GNU Make Builder
4. Project Properties > C/C++ Build:
    * deselect "Generate Makefiles automatically"
    * Build directory = ${workspace_loc:/HedgehogFirmware}/
5. Project Properties > C/C++ Build > Settings > Target Settings:
    * Target = STM32F401VB
6. Create Debug Config
