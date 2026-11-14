#include "Director/DirectorVoice.h"
#include "RESHOOT.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UDirectorVoice::UDirectorVoice()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDirectorVoice::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearLines();
	Super::EndPlay(EndPlayReason);
}

double UDirectorVoice::Now() const
{
	// Même horloge que le gestionnaire de timers (les gels du CUT! ne ralentissent pas le monde).
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

const FDirectorLine* UDirectorVoice::FindLine(FName LineId) const
{
	if (LineId.IsNone() || !LinesTable)
	{
		return nullptr;
	}
	const FDirectorLine* Line = LinesTable->FindRow<FDirectorLine>(LineId, TEXT("UDirectorVoice"), false);
	if (!Line)
	{
		UE_LOG(LogReshoot, Warning, TEXT("Director line '%s' not found in %s"), *LineId.ToString(), *GetNameSafe(LinesTable));
	}
	return Line;
}

float UDirectorVoice::GetLineDuration(const FDirectorLine& Line) const
{
	return FMath::Max(Line.SubtitleDuration, Line.Sound ? Line.Sound->GetDuration() : 0.f);
}

float UDirectorVoice::PlayLine(FName LineId)
{
	const FDirectorLine* Line = FindLine(LineId);
	if (!Line)
	{
		return 0.f;
	}
	const float Duration = GetLineDuration(*Line);
	const double Time = Now();
	const double Wait = FMath::Max(ChannelFreeTime - Time, 0.0);

	if (Wait <= KINDA_SMALL_NUMBER)
	{
		StartLine(LineId);
		return Duration;
	}
	if (Wait > MaxQueueWait)
	{
		// Elle passerait trop tard pour avoir encore un sens.
		UE_LOG(LogReshoot, Verbose, TEXT("Director line '%s' dropped (would wait %.1f s)."), *LineId.ToString(), Wait);
		return 0.f;
	}

	QueuedLines.Add(LineId);
	ChannelFreeTime = Time + Wait + Duration;
	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(QueueTimer))
		{
			World->GetTimerManager().SetTimer(QueueTimer, this, &UDirectorVoice::PlayNextQueued, static_cast<float>(Wait), false);
		}
	}
	return static_cast<float>(Wait) + Duration;
}

void UDirectorVoice::StartLine(FName LineId)
{
	const FDirectorLine* Line = FindLine(LineId);
	if (!Line)
	{
		return;
	}
	const float Duration = GetLineDuration(*Line);
	ChannelFreeTime = FMath::Max(ChannelFreeTime, Now() + Duration);
	if (Line->Sound)
	{
		CurrentSound = UGameplayStatics::SpawnSound2D(this, Line->Sound);
	}
	OnSubtitle.Broadcast(LineId, Line->Subtitle, Duration);
}

void UDirectorVoice::PlayNextQueued()
{
	if (QueuedLines.Num() == 0)
	{
		return;
	}
	const FName LineId = QueuedLines[0];
	QueuedLines.RemoveAt(0);

	const FDirectorLine* Line = FindLine(LineId);
	const float Duration = Line ? GetLineDuration(*Line) : 0.f;
	if (Line)
	{
		if (Line->Sound)
		{
			CurrentSound = UGameplayStatics::SpawnSound2D(this, Line->Sound);
		}
		OnSubtitle.Broadcast(LineId, Line->Subtitle, Duration);
	}
	if (QueuedLines.Num() > 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(QueueTimer, this, &UDirectorVoice::PlayNextQueued, FMath::Max(Duration, 0.01f), false);
		}
	}
}

void UDirectorVoice::ClearLines()
{
	QueuedLines.Reset();
	ChannelFreeTime = 0.0;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(QueueTimer);
	}
	if (CurrentSound)
	{
		CurrentSound->Stop();
		CurrentSound = nullptr;
	}
}
