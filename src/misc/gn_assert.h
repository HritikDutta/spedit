#pragma once

#ifdef DEBUG

bool DebugMessage(const char* message, const char* file, int line);

#define ASSERT(x) (void)(!!(x) || (DebugMessage(#x, __FILE__, __LINE__)))
#define ASSERT_NOT_IMPLEMENTED() DebugMessage(__FUNCTION__" not implemented!", __FILE__, __LINE__)
#define ASSERT_NOT_VALID(msg) DebugMessage(msg, __FILE__, __LINE__)

#else

#define ASSERT(x)
#define ASSERT_NOT_IMPLEMENTED()
#define ASSERT_NOT_VALID(msg)

#endif
