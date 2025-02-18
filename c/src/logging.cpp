#include "logging.hpp"

static void nulllogger(msrLogLevel, const char*, const char*) { return; }

msrLogCallback msr::logCallback = nulllogger;

void msrSetLogCallback(msrLogCallback callback) { msr::logCallback = callback; }