#include "Director/DirectiveHeadshots.h"

float UDirectiveHeadshots::EvaluateKill(const FKillInfo& Kill) const
{
	return Judge(Kill.bHeadshot);
}
