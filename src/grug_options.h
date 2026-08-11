#pragma once

#ifdef GRUG_MALLOC_HEADER
	#include GRUG_MALLOC_HEADER
#else
	#include <malloc.h> //NOLINT: need malloc
#endif

#ifndef GRUG_MALLOC
	#define GRUG_MALLOC(_size) malloc(_size)
#endif

#ifndef GRUG_FREE
	#define GRUG_FREE(_ptr, _len) ((void)(_len), free(_ptr))
#endif
