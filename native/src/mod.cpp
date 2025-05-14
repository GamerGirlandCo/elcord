#include "mod.h"
#include <cstdarg>
#include <string>
#include <vector>
#include <windows.h>
#include "winwrappers.h"

static HANDLE npipe = INVALID_HANDLE_VALUE;
const std::string pipeName = R"(\\.\pipe\discord-ipc-0)";
void msg(emacs_env *env, std::string fmt, ...) {
  std::va_list args;
  va_start(args, fmt);

  std::vector<emacs_value> aargs;
  char* cfmt = fmt.data();
  aargs.push_back(env->make_unibyte_string(env, fmt.c_str(), fmt.size()));
  for (const char *p = cfmt; *p != '\0'; ++p) {
    switch (*p) {
    case '%':
      switch (*++p) {
      case 'd':
        aargs.push_back(env->make_integer(env, va_arg(args, int)));
        continue;
      case 'f':
        aargs.push_back(env->make_float(env, va_arg(args, double)));
        continue;
      case 's': {
        auto str = va_arg(args, const char *);
        aargs.push_back(env->make_unibyte_string(env, str, strlen(str)));
        continue;
      }
      case 'c': {
        aargs.push_back(env->make_integer(env, va_arg(args, int)));
        continue;
      }
      case '%': {
        std::string str = "%";
        aargs.push_back(env->make_string(env, str.c_str(), str.size()));
        continue;
      }
      }
    default:
      continue;
    }
  }
  emacs_value msgFunc = env->intern(env, "message");
  env->funcall(env, msgFunc, aargs.size(), aargs.data());
}

std::string read_message(emacs_env *env) {
  char opcodeBuffer[4];
  DWORD bytesRead;
  msg(env, "about to read");
  bool res = ReadFileWithTimeout(npipe, opcodeBuffer, 4, &bytesRead, 5000);
  if (!res) {
    msg(env, "did not receive anything!");
    return "";
  }

  char lengthBuffer[4];
  res = ReadFileWithTimeout(npipe, lengthBuffer, 4, &bytesRead, 5000);
  if (!res) {
    msg(env, "did not receive anything!");
    return "";
  }
  int payloadLength = *reinterpret_cast<int *>(lengthBuffer);

  std::vector<char> payloadBuffer(payloadLength);
  res = ReadFileWithTimeout(npipe, payloadBuffer.data(), payloadLength,
                            &bytesRead, 5000);
  if (!res) {
    msg(env, "did not receive anything!");
    return "";
  }

  return std::string(payloadBuffer.begin(), payloadBuffer.end());
}

extern "C" {
__declspec(dllexport) int plugin_is_GPL_compatible;
static emacs_value Felcord_send_message(emacs_env *env, ptrdiff_t nargs,
                                        emacs_value args[],
                                        void *data) noexcept {
  auto opcode = env->extract_integer(env, args[0]);
  ptrdiff_t sz = 0;
  env->copy_string_contents(env, args[1], nullptr, &sz);
  char *charPayload = new char[sz];
  env->copy_string_contents(env, args[1], charPayload, &sz);
  std::string payload(charPayload);
  std::vector<char> msg(8 + sz);
  *reinterpret_cast<int *>(msg.data()) = opcode;
  *reinterpret_cast<int *>(msg.data() + 4) = sz - 1;
  std::copy(payload.begin(), payload.end(), msg.begin() + 8);
  DWORD bytesWritten;
  WriteFile(npipe, msg.data(), msg.size() - 1, &bytesWritten, nullptr);
  std::string res = read_message(env);
  return env->make_unibyte_string(env, res.c_str(), res.size());
  // char* str = "Brian Tatler fucked and abused Sean Harris!";
  // return env->make_unibyte_string(env, str, strlen(str));
}
static emacs_value Felcord_connect(emacs_env *env, ptrdiff_t nargs,
                                   emacs_value args[], void *data) noexcept {
  npipe = CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                      nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
  if (npipe == INVALID_HANDLE_VALUE) {
    return env->intern(env, "nil");
  }
  DWORD timeout = 5000;
  DWORD mode = PIPE_READMODE_BYTE;
  BOOL setRes = SetNamedPipeHandleState(npipe, &mode, nullptr, &timeout);
  if (!setRes)
    msg(env, "didn't set the timeout properly?");
  return env->make_integer(env, 1);
}
static emacs_value Felcord_disconnect(emacs_env *env, ptrdiff_t nargs,
                                      emacs_value args[], void *data) noexcept {
  if (npipe == INVALID_HANDLE_VALUE)
    return env->intern(env, "nil");
  CloseHandle(npipe);
  npipe = INVALID_HANDLE_VALUE;
  return env->make_integer(env, 1);
}
}
static void bind_function(emacs_env *env, const char *name, emacs_value Sfun) {
  /* Set the function cell of the symbol named NAME to SFUN using
     the 'fset' function.  */

  /* Convert the strings to symbols by interning them */
  emacs_value Qfset = env->intern(env, "fset");
  emacs_value Qsym = env->intern(env, name);

  /* Prepare the arguments array */
  emacs_value args[] = {Qsym, Sfun};

  /* Make the call (2 == nb of arguments) */
  env->funcall(env, Qfset, 2, args);
}
static void provide(emacs_env *env, const char *feature) {
  /* call 'provide' with FEATURE converted to a symbol */

  emacs_value Qfeat = env->intern(env, feature);
  emacs_value Qprovide = env->intern(env, "provide");
  emacs_value args[] = {Qfeat};

  env->funcall(env, Qprovide, 1, args);
}

extern "C" EXPORTED int emacs_module_init(struct emacs_runtime *ert) noexcept {
  emacs_env *env = ert->get_environment(ert);

  /* create a lambda (returns an emacs_value) */
  emacs_value fun = env->make_function(
      env, 2,                                   /* min. number of arguments */
      2,                                        /* max. number of arguments */
      Felcord_send_message,                     /* actual function pointer */
      "send a message to discord's named pipe", /* docstring */
      nullptr);

  bind_function(env, "elcord--native-send-message", fun);
  bind_function(env, "elcord--native-connect",
                env->make_function(env, 0, 0, Felcord_connect,
                                   "connect to discord's named pipe", nullptr));
  bind_function(env, "elcord--native-disconnect",
                env->make_function(env, 0, 0, Felcord_disconnect,
                                   "disconnect from discord's named pipe",
                                   nullptr));
  provide(env, "elcord-native");

  /* loaded successfully */
  return 0;
}
