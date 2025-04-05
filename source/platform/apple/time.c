#include <mach/mach_time.h>
#include <sippet/core/message.h>
#include "time.h"

static mach_timebase_info_data_t sippetAppleTimebaseInformationData;

void sippetAppleInitializeTime(void) {
  mach_timebase_info(&sippetAppleTimebaseInformationData);
  sippetSendMessage(SIPPET_MESSAGE_SEVERITY_NOTICE, "Time module for Apple initialized");
}

double sippetAppleGetCurrentTime(void) {
  return mach_absolute_time() * sippetAppleTimebaseInformationData.numer / (double)sippetAppleTimebaseInformationData.denom / 1e9;
}