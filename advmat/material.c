#include "material.h"
#include <assert.h>

typedef enum {
	MAT_AIR = 0,
	MAT_WATER,
	MAT_ICE,
	MAT_STEAM,
} MaterialDef;

static Material materials[] = {
	[MAT_AIR]   = {STR("air"),   0.0f, 0.0f, 0.0f, {  0,   0,   0}, 0, 0, MATERIAL_GAS},
	[MAT_WATER] = {STR("water"), 0.0f, 0.0f, 0.0f, {  0,   0, 255}, 0, 0, MATERIAL_LIQUID},
	[MAT_ICE]   = {STR("ice"),   0.0f, 0.0f, 0.0f, {153, 255, 255}, 0, 0, MATERIAL_SOLID },
	[MAT_STEAM] = {STR("steam"), 0.0f, 0.0f, 0.0f, {240, 248, 255}, 0, 0, MATERIAL_GAS   },
};

static MaterialTransition materialTransitions[] = {
	{{MAT_WATER}, {MAT_ICE  }, MATERIAL_TRANS_TEMPERATURE_LTE, .temperature=TEMPC(0.0f)  },
	{{MAT_ICE  }, {MAT_WATER}, MATERIAL_TRANS_TEMPERATURE_GTE, .temperature=TEMPC(0.0f)  },
	{{MAT_STEAM}, {MAT_WATER}, MATERIAL_TRANS_TEMPERATURE_LTE, .temperature=TEMPC(100.0f)},
	{{MAT_WATER}, {MAT_STEAM}, MATERIAL_TRANS_TEMPERATURE_GTE, .temperature=TEMPC(100.0f)},
};

void
initMaterialTable(MaterialTable *table)
{
	table->materials = materials;
	table->numMaterials =
		sizeof(materials) / sizeof(Material);

	table->materialTransitions = materialTransitions;
	table->numMaterialTransitions =
		sizeof(materialTransitions) / sizeof(MaterialTransition);
}

Material *
getMaterial(MaterialTable *table, MaterialId id)
{
	assert(id.val < table->numMaterials);
	return &table->materials[id.val];
}
