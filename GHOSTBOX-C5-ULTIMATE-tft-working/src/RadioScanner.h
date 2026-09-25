#ifndef RADIOSCANNER_H

#define RADIOSCANNER_H



#include <Arduino.h>

#include <SPI.h>

#include <RF24.h>

#include "PepeDraw.h"



// Main function prototypes

void runRadioScanner();
void runRfBaseline();

#endif
