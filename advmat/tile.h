#ifndef STAGE_TILE_H
#define STAGE_TILE_H

#include "material.h"
#include "intdef.h"

typedef enum {
	PORTAL_NEAR = 0,
	PORTAL_FAR  = 1,
} PortalKind;

typedef struct {
	PortalKind kind;
	uint8_t    side_mask;
	uint8_t    rotation;
	uint32_t   location;
} PortalInfo;

/*
 * struct {
 * 	uint8_t              : 1;
 * 	PortalKind kind      : 1;
 * 	uint8_t    side_mask : 6;
 * 	uint8_t    rotation  : 3;
 * 	uint32_t   location  : 21;
 * };
*/
typedef struct {
	uint32_t value;
} Portal;

PortalInfo
portal_get_info(Portal p);

Portal
portal_from_info(PortalInfo p);

typedef struct {
	MaterialId  material;
	Portal      portal;
	Temperature temperature;
	Pressure    pressure;
	Magic       magic;
} Tile;

Tile
tileTransition(MaterialTable *mats, Tile *from,
		MaterialId *neighbours, size_t numNeighbours);

#endif
