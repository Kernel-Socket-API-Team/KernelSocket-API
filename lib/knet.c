// common
#include "../src/dispatcher/dispatcher.c"

#ifdef _WIN32
	// Windows
	#include "../src/adapters/windows/windows_adapter.c"
	#include "../src/common/common.c"
	#include "../src/common/windows/windows_common.c"
	#include "../src/common/windows/windows_common_release.c"
	#include "../src/protocols/windows_protocols.c"
#else
	// Linux
	#include "../src/adapters/linux/linux_adapter.c"
	#include "../src/common/linux/linux_common.c"
	#include "../src/common/linux/linux_common_release.c"
	#include "../src/protocols/linux_protocols.c"
#endif