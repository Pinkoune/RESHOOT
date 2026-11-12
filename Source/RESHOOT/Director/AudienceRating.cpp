#include "Director/AudienceRating.h"

UAudienceRating::UAudienceRating()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAudienceRating::Add(float Amount)
{
	const float Old = Rating;
	Rating = FMath::Clamp(Rating + Amount, 0.f, MaxRating);
	if (Rating != Old)
	{
		OnRatingChanged.Broadcast(Rating, Rating - Old);
	}
}

void UAudienceRating::ResetRating()
{
	const float Old = Rating;
	Rating = 0.f;
	OnRatingChanged.Broadcast(Rating, Rating - Old);
}

float UAudienceRating::GetRatingPct() const
{
	return MaxRating > 0.f ? Rating / MaxRating : 0.f;
}

int32 UAudienceRating::GetStars() const
{
	int32 Stars = 0;
	for (const float Threshold : StarThresholds)
	{
		if (Rating >= Threshold)
		{
			++Stars;
		}
	}
	return Stars;
}
