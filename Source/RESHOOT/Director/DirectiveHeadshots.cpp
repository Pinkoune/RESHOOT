#include "Director/DirectiveHeadshots.h"

bool UDirectiveHeadshots::IsKillRespected(const FKillInfo& Kill) const
{
	return Kill.bHeadshot;
}
