#pragma once

#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
 #define mu_run_test(test) do { const char *message = test(); \
                                if (message) return message; } while (0)
