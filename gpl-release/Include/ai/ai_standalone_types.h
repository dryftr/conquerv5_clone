// SPDX-License-Identifier: GPL-3.0-or-later
/* ai_standalone_types.h - Type definitions for standalone AI module testing */
/*
 * When building with the full Conquer codebase, ntntype and Diplotype
 * are defined in sysconf.h and dstatusX.h. This header provides
 * compatible definitions for standalone unit testing of AI modules.
 *
 * Include this BEFORE the ai/ headers in standalone test files.
 * NEVER include this when building with the full Conquer codebase
 * (the MEMORYH macro is defined in the Conquer build).
 */

#ifndef AI_STANDALONE_TYPES_H
#define AI_STANDALONE_TYPES_H

#ifdef MEMORYH
/* Building with Conquer - types already defined */
#else
/* Standalone testing - provide compatible types */
typedef short ntntype;

typedef enum diplotype Diplotype;
enum diplotype {
  DIP_UNMET, DIP_ALLIED, DIP_TREATY, DIP_FRIENDLY, DIP_PEACEFUL,
  DIP_NEUTRAL, DIP_HOSTILE, DIP_BELLICOSE, DIP_WAR, DIP_JIHAD
};
#endif

#endif /* AI_STANDALONE_TYPES_H */