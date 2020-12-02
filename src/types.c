#include "types.h"

struct mgc_aabbi
mgc_aabbi_from_extents(v3i p0, v3i p1)
{
	struct mgc_aabbi result = {0};
	result.min.x = min(p0.x, p1.x);
	result.min.y = min(p0.y, p1.y);
	result.min.z = min(p0.z, p1.z);

	result.max.x = max(p0.x, p1.x) + 1;
	result.max.y = max(p0.y, p1.y) + 1;
	result.max.z = max(p0.z, p1.z) + 1;

	return result;
}

struct mgc_aabbi
mgc_aabbi_from_point(v3i p)
{
	struct mgc_aabbi result = {0};
	result.min.x = p.x;
	result.min.y = p.y;
	result.min.z = p.z;

	result.max.x = p.x + 1;
	result.max.y = p.y + 1;
	result.max.z = p.z + 1;

	return result;
}

struct mgc_aabbi
mgc_aabbi_extend_bounds(struct mgc_aabbi lhs, struct mgc_aabbi rhs)
{
	struct mgc_aabbi result = {0};
	if (lhs.min.x >= lhs.max.x ||
		lhs.min.y >= lhs.max.y ||
		lhs.min.z >= lhs.max.z) {
		return rhs;
	}

	if (rhs.min.x >= rhs.max.x ||
		rhs.min.y >= rhs.max.y ||
		rhs.min.z >= rhs.max.z) {
		return lhs;
	}

	result.min.x = min(lhs.min.x, rhs.min.x);
	result.min.y = min(lhs.min.y, rhs.min.y);
	result.min.z = min(lhs.min.z, rhs.min.z);

	result.max.x = max(lhs.max.x, rhs.max.x);
	result.max.y = max(lhs.max.y, rhs.max.y);
	result.max.z = max(lhs.max.z, rhs.max.z);

	return result;
}

struct mgc_aabbi
mgc_aabbi_intersect_bounds(struct mgc_aabbi lhs, struct mgc_aabbi rhs)
{
	struct mgc_aabbi result = {0};
	result.min.x = max(lhs.min.x, rhs.min.x);
	result.min.y = max(lhs.min.y, rhs.min.y);
	result.min.z = max(lhs.min.z, rhs.min.z);

	result.max.x = min(lhs.max.x, rhs.max.x);
	result.max.y = min(lhs.max.y, rhs.max.y);
	result.max.z = min(lhs.max.z, rhs.max.z);

	return result;
}
