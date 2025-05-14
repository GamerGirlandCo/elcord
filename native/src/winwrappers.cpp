#include "winwrappers.h"

bool ReadFileWithTimeout(HANDLE hFile, LPVOID lpBuffer,
                                DWORD nNumberOfBytesToRead, LPDWORD nBytesRead,
                                DWORD timeoutMs) {
  if (hFile == INVALID_HANDLE_VALUE) {
    return false;
  }

  OVERLAPPED overlapped = {};
  overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  if (!overlapped.hEvent) {
    return false;
  }

  bool result = false;

    if (!ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, nullptr,
                  &overlapped)) {
      DWORD error = GetLastError();
      if (error != ERROR_IO_PENDING) {
        return false;
      }
    }

    DWORD waitResult = WaitForSingleObject(overlapped.hEvent, timeoutMs);
    switch (waitResult) {
    case WAIT_OBJECT_0:
      if (!GetOverlappedResult(hFile, &overlapped, nBytesRead, FALSE)) {
        return false;
      }
      result = true;
      break;

    case WAIT_TIMEOUT:
      CancelIo(hFile);
      return false;
      break;

    default:
      return false;
    }

  CloseHandle(overlapped.hEvent);
  return result;
}
