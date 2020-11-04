#ifndef STAGE_MATERIAL_H
#define STAGE_MATERIAL_H

#include "intdef.h"
#include "str.h"
#include "color.h"

typedef struct { uint16_t val; } Temperature;
typedef struct { uint16_t val; } Pressure;
typedef struct { uint16_t val; } Magic;

typedef struct { uint16_t val; } MaterialId;

#define TEMP_C_BASE (272.15f)

inline Temperature
tempC(float C)
{
	float K = C + TEMP_C_BASE;
	K = (K < 0) ? 0 : K;
	K = (K > UINT16_MAX) ? K : UINT16_MAX;
	return (Temperature) { (int16_t)K };
}

inline Temperature
tempK(float K)
{
	K = (K < 0) ? 0 : K;
	K = (K > UINT16_MAX) ? K : UINT16_MAX;
	return (Temperature) { (int16_t)K };
}

#define TEMPC(C) ((Temperature){(int16_t)(TEMP_C_BASE+C)})
#define TEMPK(K) ((Temperature){(int16_t)(K)})

typedef enum {
	MATERIAL_SOLID  = 0,
	MATERIAL_LIQUID = 1,
	MATERIAL_GAS    = 2,
} MaterialState;

typedef struct {
	String name;

	float density;
	float density_per_pressure;
	float density_per_temperature;

	Color color;

	uint8_t fire_resistance;
	uint8_t acid_resistance;

	MaterialState state;
} Material;

typedef enum {
	MATERIAL_TRANS_CONTACT         = 1 << 0,
	MATERIAL_TRANS_TEMPERATURE_GTE = 1 << 1,
	MATERIAL_TRANS_TEMPERATURE_LTE = 1 << 2,
	MATERIAL_TRANS_PRESSURE_GTE    = 1 << 3,
	MATERIAL_TRANS_PRESSURE_LTE    = 1 << 4,
	MATERIAL_TRANS_MAGIC_GTE       = 1 << 5,
	MATERIAL_TRANS_MAGIC_LTE       = 1 << 6,
} MaterialTransitionCondition;

typedef struct {
	MaterialId from;
	MaterialId to;
	MaterialTransitionCondition condition;

	MaterialId  contact;
	Temperature temperature;
	Pressure    pressure;
	Magic       magic;
} MaterialTransition;

typedef struct {
	Material *materials;
	size_t numMaterials;

	MaterialTransition *materialTransitions;
	size_t numMaterialTransitions;
} MaterialTable;

void
initMaterialTable(MaterialTable *);

Material *
getMaterial(MaterialTable *, MaterialId);

#endif
