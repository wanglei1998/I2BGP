#include "hook-manager.h"
#include "context.h"
#include <iostream>
#include <dlfcn.h>

using namespace aspcpp;

static void sink (void)
{
  std::cout << "sink called" << std::endl;
}

static void source (void)
{
  std::cout << "source called" << std::endl;
}

int main (int argc, char *argv[])
{
  HookManager *manager = new HookManager ();

  //manager->AddHookBySourceAndFunction ("test.cc", "source", &sink);
  manager->AddHookByFunction ("::source", &sink);
  manager->AddHookByFunction ("::foo", &sink);
 
  void *h = dlopen ("libfoo.so", RTLD_LAZY);
  void (*foo) (void) = (void (*)(void))dlsym (h, "foo");
  foo ();
  dlclose (h);

  source ();

  delete manager;
  return 0;
}
