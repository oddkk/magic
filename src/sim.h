#ifndef MAGIC_SIM_H
#define MAGIC_SIM_H

#include "chunk_cache.h"
#include "registry.h"
#include "types.h"

void
mgc_sim_tick(v3i sim_center, struct mgc_chunk_cache *cache, struct mgc_registry *reg);

#endif
