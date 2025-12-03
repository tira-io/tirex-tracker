#include "abort.hpp"
#include "logging.hpp"

#include <cstdlib>

static void defaultabort(const char* msg) {
	tirex::log::critical("tracker", "Aborting with Error: {}", msg);
	std::abort();
}

static tirexLogLevel abortlevel = tirexLogLevel::CRITICAL;
static tirexAbortCallback abortfn = defaultabort;

void tirexSetAbortCallback(tirexAbortCallback callback) { abortfn = (callback == nullptr) ? defaultabort : callback; }
void tirexSetAbortLevel(tirexLogLevel level) { abortlevel = level; }

void tirex::abort(tirexLogLevel level, const char* message) {
	if (level >= abortlevel)
		abortfn(message);
}