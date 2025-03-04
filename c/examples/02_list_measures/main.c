#include <tirex_tracker.h>

#include <stdio.h>

static const char* datatypeToStr[] = {
		[TIREX_STRING] = "string",
		[TIREX_INTEGER] = "integer",
		[TIREX_FLOATING] = "float",
};

int main(int argc, char* argv[]) {
	puts("[DATA PROVIDERS]");
	size_t num = tirexDataProviderGetAll(NULL, 0);
	// Bless you ISO C99 (https://gcc.gnu.org/onlinedocs/gcc/Variable-Length.html)
	tirexDataProvider buf[num];
	tirexDataProviderGetAll(buf, num);
	for (size_t i = 0; i < num; ++i)
		printf("== %s ==\nVersion:     %s\nDescription: %s\n\n", buf[i].name, buf[i].version, buf[i].description);

	puts("[MEASURES]");
	const tirexMeasureInfo* info;
	for (tirexMeasure measure = 0; measure < TIREX_MEASURE_COUNT; ++measure) {
		tirexMeasureInfoGet(measure, &info);
		printf("(0x%02x; %-7s)   %s\n", measure, datatypeToStr[info->datatype], info->description);
	}
	return 0;
}