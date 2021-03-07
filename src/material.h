#ifndef MAGIC_MATERIAL_H
#define MAGIC_MATERIAL_H

#include "intdef.h"
#include "str.h"
#include "color.h"

typedef uint16_t TraitId;
typedef uint16_t ActionId;
typedef uint16_t StatusId;
typedef uint16_t mgc_material_id;

typedef enum {
	TRAIT_GAS,
	TRAIT_DUST,
	TRAIT_LIQUID,
	TRAIT_FLAMABLE,
	TRAIT_SOAKABLE,
	TRAIT_CONDUCTIVE,
} DeafultTraitTypes;

typedef struct {
	struct string name;
} TraitType;

typedef enum {
	STATUS_WET,
	STATUS_BURNING,
	STATUS_SHOCKED,
} DeafultStatusTypes;

typedef struct {
	struct string name;
} StatusType;

typedef struct {
	StatusId status;
	int duration;
} Status;

typedef enum {
	ACTION_IGNITE,
	ACTION_SHOCK,
} DeafultActionTypes;

typedef struct {
	struct string name;
} ActionType;

typedef enum {
	MAT_AIR,
	MAT_ICE,
	MAT_WATER,
	MAT_WATER_VAPOUR,
	MAT_WOOD,
	MAT_METAL,
} DeafultMaterialTypes;

typedef struct mgc_material {
	struct string name;
	Color color;
	bool solid;
} Material;

typedef struct {
	mgc_material_id material;
	TraitId trait;
} MaterialTrait;


typedef enum {
	TRANS_APPLY_STATUS,
	TRANS_REMOVE_STATUS,
	TRANS_SET_MATERIAL,
} TransitionType;

typedef struct {
	TraitId trait;
	ActionId action;

	TransitionType type;

	union {
		mgc_material_id materialTo;
		StatusId status;
	};

} MaterialTransition;


typedef struct mgc_material_table {
	struct mgc_material *materials;
	size_t numMaterials;
} MaterialTable;

void
initMaterialTable(MaterialTable *);

Material *
getMaterial(MaterialTable *, mgc_material_id);

#endif
