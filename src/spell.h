#ifndef MAGIC_SPELL_H
#define MAGIC_SPELL_H

enum {
	SPELL_NOP = 0,
	SPELL_ACTION,
} SpellOp;

typedef struct {
	SpellOp op;
	union {
		struct {
		} action;
	};
} SpellInstruction;

typedef struct {
	int _dc;
} SpellParameter;

typedef struct {
	int _dc;
} SpellRegistryDef;

typedef struct {
	SpellParameter *params;
	size_t num_params;

	SpellRegistryDef *regs;
	size_t num_regs;

	SpellInstruction *instrs;
	size_t num_instrs;
} Spell;

#endif
