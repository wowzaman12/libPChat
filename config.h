/* PChat
 * Copyright (c) 2009-2010, Zach Thibeau
 * Copyright (c) 2010-2011, XhmikosR
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PCHAT_CONFIG_H
#define PCHAT_CONFIG_H

#include "src/include/version.h"

#if !defined(TWO_DIGIT_VER) && !defined(THREE_DIGIT_VER)
	#error "Define TWO_DIGIT_VER or THREE_DIGIT_VER in version.h"
#endif

#if defined(TWO_DIGIT_VER) && defined(THREE_DIGIT_VER)
	#error "You can't define both TWO_DIGIT_VER and THREE_DIGIT_VER at the same time"
#endif

#if defined(TWO_DIGIT_VER)
	#define PACKAGE_TARNAME "PChat-"STRINGIFY(VERSION_MAJOR)"."STRINGIFY(VERSION_MINOR)""
#elif defined(THREE_DIGIT_VER)
	#define PACKAGE_TARNAME "PChat-"STRINGIFY(VERSION_MAJOR)"."STRINGIFY(VERSION_MINOR)"."STRINGIFY(VERSION_BUILD)""
#endif

#if defined(SVNCOMPILE)
	#if defined(TWO_DIGIT_VER)
		#define PACKAGE_VERSION ""STRINGIFY(VERSION_MAJOR)"."STRINGIFY(VERSION_MINOR)" rev. "STRINGIFY(VERSION_REV)""
	#elif defined(THREE_DIGIT_VER)
		#define PACKAGE_VERSION ""STRINGIFY(VERSION_MAJOR)"."STRINGIFY(VERSION_MINOR)"."STRINGIFY(VERSION_BUILD)" rev. "STRINGIFY(VERSION_REV)""
	#endif
#else
	#if defined(TWO_DIGIT_VER)
		#define PACKAGE_VERSION STRINGIFY(VERSION_MAJOR)"."STRINGIFY(VERSION_MINOR)
	#elif defined(THREE_DIGIT_VER)
		#define PACKAGE_VERSION STRINGIFY(VERSION_MAJOR)"."STRINGIFY(VERSION_MINOR)"."STRINGIFY(VERSION_BUILD)
	#endif
#endif

#define GETTEXT_PACKAGE "PChat"
#define PACKAGE_NAME    "PChat"
#define LOCALEDIR "./locale"
#define XCHATLIBDIR "."
#define XCHATSHAREDIR "./"

#define ENABLE_NLS
#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1
#define HAVE_STRTOULL
#define USE_DCC64
#define USE_GMODULE
//#define USE_IPV6
#define USE_LIBSEXY
#ifndef _MSC_VER
//#define USE_MMX
#endif
#define USE_LIBCO
#define USE_OPENSSL
#define USE_PLUGIN
#define USE_WIN32_THREADS
#define G_DISABLE_CAST_CHECKS
//#define G_DISABLE_DEPRECATED
//#define GDK_DISABLE_DEPRECATED
#define GDK_PIXBUF_DISABLE_DEPRECATED
//#define GTK_DISABLE_DEPRECATED

#ifndef USE_IPV6
#define socklen_t int
#endif


#endif // PCHAT_CONFIG_H
