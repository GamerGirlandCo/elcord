#ifndef ELCORD_MOD_H
#define ELCORD_MOD_H
#include "emacs-module.h"
#define EXPORTED __declspec(dllexport)
extern int emacs_module_init (struct emacs_runtime *runtime) noexcept;
#endif