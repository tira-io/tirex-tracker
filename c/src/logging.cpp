#include "logging.hpp"

static void nulllogger(tirexLogLevel, const char*, const char*) { return; }

tirexLogCallback tirex::logCallback = nulllogger;

void tirexSetLogCallback(tirexLogCallback callback) { tirex::logCallback = callback; }