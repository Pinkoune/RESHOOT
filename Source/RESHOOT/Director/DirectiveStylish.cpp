#include "Director/DirectiveStylish.h"

bool UDirectiveStylish::IsKillRespected(const FKillInfo& Kill) const
{
	// GDD « Cascade » : élimination en glissade ou en l'air (état relevé par le socle au moment du kill).
	return Kill.bPlayerSliding || Kill.bPlayerInAir;
}
