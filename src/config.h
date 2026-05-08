/*
 * ClanBomber for Nintendo Switch — minimal config.h replacement.
 *
 * Copyright (C) 2026 ClanBomber Switch port contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the LICENSE file for details.
 */

#ifndef CLANBOMBER_SWITCH_CONFIG_H
#define CLANBOMBER_SWITCH_CONFIG_H

/* Replaces the autotools-generated config.h that the upstream sources include
 * with `#include <config.h>` (angle brackets — picked up via -Isrc).
 *
 * CB_DATADIR / CB_LOCALEDIR / PACKAGE come from the Makefile via -D flags.
 * Switch (ARM64) is little-endian, so WORDS_BIGENDIAN is intentionally undefined.
 */

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "ClanBomber"
#endif

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "2.3.0-switch"
#endif

#ifndef PACKAGE_STRING
#define PACKAGE_STRING PACKAGE_NAME " " PACKAGE_VERSION
#endif

#ifndef PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT ""
#endif

#ifndef PACKAGE_TARNAME
#define PACKAGE_TARNAME "clanbomber2"
#endif

#endif
