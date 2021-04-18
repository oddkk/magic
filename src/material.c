#include "material.h"
#include <assert.h>

TraitType defaultTraits[] = {
	[TRAIT_GAS]        = { STR("gas")        },
	[TRAIT_DUST]       = { STR("dust")       },
	[TRAIT_LIQUID]     = { STR("liquid")     },
	[TRAIT_FLAMABLE]   = { STR("flamable")   },
	[TRAIT_SOAKABLE]   = { STR("soakable")   },
	[TRAIT_CONDUCTIVE] = { STR("conductive") },
};

StatusType defaultStatusTypes[] = {
	[STATUS_WET]     = { STR("wet")     },
	[STATUS_BURNING] = { STR("burning") },
	[STATUS_SHOCKED] = { STR("shocked") },
};

ActionType defaultActionTypes[] = {
	[ACTION_IGNITE] = { STR("ignite") },
	[ACTION_SHOCK]  = { STR("shock")  },
};

Material defaultMaterials[] = {
	[MAT_AIR]   = { STR("air"),   COLOR_HEX(0x000000), false },
	[MAT_WATER] = { STR("water"), COLOR_HEX(0x0000ff), true },
	[MAT_WOOD]  = { STR("wood"),  COLOR_HEX(0x804000), true },
	[MAT_METAL] = { STR("metal"), COLOR_HEX(0xd3d3d3), true },
};

MaterialTrait defaultMaterialTraits[] = {
	{ MAT_AIR, TRAIT_GAS },

	{ MAT_WATER, TRAIT_LIQUID },
	{ MAT_WATER, TRAIT_CONDUCTIVE },

	{ MAT_WATER_VAPOUR, TRAIT_LIQUID },
	{ MAT_WATER_VAPOUR, TRAIT_CONDUCTIVE },

	{ MAT_WOOD, TRAIT_FLAMABLE },

	{ MAT_METAL, TRAIT_CONDUCTIVE },
};

MaterialTransition defaultTraitTransitions[] = {
	{ TRAIT_FLAMABLE, ACTION_IGNITE, TRANS_APPLY_STATUS, .materialTo = STATUS_BURNING }
};

void
mgc_material_table_init(struct mgc_material_table *mats)
{
	mats->num_materials = sizeof(defaultMaterials) / sizeof(Material);
	mats->materials = defaultMaterials;
}

Material *
mgc_mat_get(struct mgc_material_table *mats, mgc_material_id id)
{
	assert(id < mats->num_materials);
	return &mats->materials[id];
}

bool
mgc_material_lookup(struct mgc_material_table *mats, struct string name, mgc_material_id *out_id)
{
	for (size_t i = 0; i < mats->num_materials; i++) {
		if (string_equal(mats->materials[i].name, name)) {
			*out_id = i;
			return true;
		}
	}

	return false;
}
