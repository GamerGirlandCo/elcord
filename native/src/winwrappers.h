#include <windows.h>
bool ReadFileWithTimeout(HANDLE hFile, LPVOID lpBuffer,
                                DWORD nNumberOfBytesToRead, LPDWORD nBytesRead,
                                DWORD timeoutMs);
