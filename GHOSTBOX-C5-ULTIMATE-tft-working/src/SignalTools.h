#ifndef SIGNAL_TOOLS_H
#define SIGNAL_TOOLS_H

#include <Arduino.h>

void runSignalTools();
uint8_t signalToolsSavedIrMax();
bool signalToolsLoadSavedIrInfo(uint8_t slot, String* name, uint16_t* count);
bool signalToolsReplaySavedIrSlot(uint8_t slot);
bool signalToolsRenameSavedIrSlot(uint8_t slot, const String& name);
bool signalToolsDeleteSavedIrSlot(uint8_t slot);

#endif
