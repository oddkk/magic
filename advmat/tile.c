#include "tile.h"

PortalInfo
portal_get_info(Portal p)
{
	PortalInfo result = {0};
	result.kind      = (p.value >>  1) & 0x1;
	result.side_mask = (p.value >>  2) & 0xfc;
	result.rotation  = (p.value >>  8) & 0x07;
	result.location  = (p.value >> 12) & 0x008fffff;

	return result;
}

Portal
portal_from_info(PortalInfo p)
{
	Portal result = {0};
	result.value |= (p.kind      & 0x00001) <<  1;
	result.value |= (p.side_mask & 0x000fc) <<  2;
	result.value |= (p.rotation  & 0x00007) <<  8;
	result.value |= (p.location  & 0x008fffff) << 12;

	return result;
}

Tile
tileTransition(MaterialTable *mats, Tile *from,
		MaterialId *neighbours, size_t numNeighbours)
{
	for (size_t i = 0; i < mats->numMaterialTransitions; i++) {
		MaterialTransition *trans;
		trans = &mats->materialTransitions[i];

		if (trans->from.val != from->material.val) {
			break;
		}

		if (trans->condition & MATERIAL_TRANS_CONTACT) {
			bool contactFound = false;
			for (size_t i = 0; i < numNeighbours; i++) {
				if (neighbours[i].val == trans->contact.val) {
					contactFound = true;
					break;
				}
			}

			if (!contactFound) {
				break;
			}
		}

		if (trans->condition & MATERIAL_TRANS_TEMPERATURE_LTE) {
			if (!(from->temperature.val <= trans->temperature.val)) {
				break;
			}
		} else if (trans->condition & MATERIAL_TRANS_TEMPERATURE_GTE) {
			if (!(from->temperature.val >= trans->temperature.val)) {
				break;
			}
		}

		if (trans->condition & MATERIAL_TRANS_PRESSURE_LTE) {
			if (!(from->pressure.val <= trans->pressure.val)) {
				break;
			}
		} else if (trans->condition & MATERIAL_TRANS_PRESSURE_GTE) {
			if (!(from->pressure.val >= trans->pressure.val)) {
				break;
			}
		}

		if (trans->condition & MATERIAL_TRANS_MAGIC_LTE) {
			if (!(from->magic.val <= trans->magic.val)) {
				break;
			}
		} else if (trans->condition & MATERIAL_TRANS_MAGIC_GTE) {
			if (!(from->magic.val >= trans->magic.val)) {
				break;
			}
		}

		// Transition match.
		Tile newTile = *from;
		newTile.material = trans->to;
		return newTile;
	}

	return *from;
}
