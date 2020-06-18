/*
** Interfaces for opening a shared library, finding entry points
** within the shared library, and closing the shared library.
*/

#ifdef _WIN32

static void *dylib_open(const char *zFilename){
  HANDLE h;
  h = LoadLibraryA(zFilename);
  OSTRACE(("DLOPEN name=%s, handle=%p\n", zFilename, (void*)h));
  return (void*)h;
}

static void (*dylib_sym(void *pH,const char *zSym))(void){
  FARPROC proc;
  proc = GetProcAddress((HANDLE)pH, zSym);
  OSTRACE(("DLSYM handle=%p, symbol=%s, address=%p\n",
           (void*)pH, zSym, (void*)proc));
  return (void(*)(void))proc;
}

static void dylib_error(int nBuf, char *zBufOut){
  winGetLastErrorMsg(osGetLastError(), nBuf, zBufOut);
}

static void dylib_close(void *pHandle){
  FreeLibrary((HANDLE)pHandle);
  OSTRACE(("DLCLOSE handle=%p\n", (void*)pHandle));
}


#else  /* Linux, BSD, Mac, iOS, Android */


#include <dlfcn.h>

static void *dylib_open(const char *zFilename){
  return dlopen(zFilename, RTLD_NOW | RTLD_GLOBAL);
}

static void (*dylib_sym(void *p, const char*zSym))(void){
  /*
  ** GCC with -pedantic-errors says that C90 does not allow a void* to be
  ** cast into a pointer to a function.  And yet the library dlsym() routine
  ** returns a void* which is really a pointer to a function.  So how do we
  ** use dlsym() with -pedantic-errors?
  **
  ** Variable x below is defined to be a pointer to a function taking
  ** parameters void* and const char* and returning a pointer to a function.
  ** We initialize x by assigning it a pointer to the dlsym() function.
  ** (That assignment requires a cast.)  Then we call the function that
  ** x points to.
  **
  ** This work-around is unlikely to work correctly on any system where
  ** you really cannot cast a function pointer into void*.  But then, on the
  ** other hand, dlsym() will not work on such a system either, so we have
  ** not really lost anything.
  */
  void (*(*x)(void*,const char*))(void);
  x = (void(*(*)(void*,const char*))(void))dlsym;
  return (*x)(p, zSym);
}

static void dylib_error(int nBuf, char *zBufOut){
  const char *zErr;
  zErr = dlerror();
  if( zErr ){
    snprintf(zBufOut, nBuf, "%s", zErr);
  } else {
    zBufOut[0] = 0;
  }
}

static void dylib_close(void *pHandle){
  dlclose(pHandle);
}

#endif
