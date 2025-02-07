#include <measure.h>

#include "measure/stats/provider.hpp"

msrError msrResultEntryGetByIndex(const msrResult* result, size_t index, msrResultEntry* entry) {
	if (result == nullptr || index >= result->value.size())
		return msrError::MSR_INVALID_ARGUMENT;
	const auto& [source, value] = result->value.at(index);
	*entry = {.source = source, .value = value.c_str(), .type = msrResultType::MSR_STRING};
	return msrError::MSR_SUCCESS;
}

msrError msrResultEntryNum(const msrResult* result, size_t* num) {
	if (result == nullptr)
		return msrError::MSR_INVALID_ARGUMENT;
	*num = result->value.size();
	return msrError::MSR_SUCCESS;
}

void msrResultFree(msrResult* result) { delete result; }