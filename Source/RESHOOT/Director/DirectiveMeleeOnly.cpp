#include "Director/DirectiveMeleeOnly.h"

bool UDirectiveMeleeOnly::IsKillRespected(const FKillInfo& Kill) const
{
	return Kill.bMelee;
}
