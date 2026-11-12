#include "Director/DirectiveMeleeOnly.h"

float UDirectiveMeleeOnly::EvaluateKill(const FKillInfo& Kill) const
{
	return Judge(Kill.bMelee);
}
