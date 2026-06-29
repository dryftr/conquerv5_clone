/* ai_integration.h - Wire honest AI into Conquer's NPC update loop
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2025-2026 Ambitions Research, LLC
 */
#ifndef AI_INTEGRATION_H
#define AI_INTEGRATION_H

#include "ai/personality.h"

/* Get the global AI personality registry.
 * Returns pointer to the singleton registry.
 * Only valid after ai_ensure_initialized() has been called. */
PERSONALITY_REGISTRY_PTR ai_get_registry(void);

#endif /* AI_INTEGRATION_H */