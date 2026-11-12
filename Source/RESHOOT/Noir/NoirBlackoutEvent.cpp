#include "Noir/NoirBlackoutEvent.h"
#include "RESHOOT.h"

#include "Components/LightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ANoirBlackoutEvent::ANoirBlackoutEvent()
{
	ExposureLock = CreateDefaultSubobject<UPostProcessComponent>(TEXT("ExposureLock"));
	SetRootComponent(ExposureLock);
	ExposureLock->bUnbound = true;
	ExposureLock->Priority = 100.f;
	ExposureLock->BlendWeight = 0.f;
}

void ANoirBlackoutEvent::StartEvent()
{
	if (IsEventActive())
	{
		return;
	}
	Super::StartEvent();

	SavedIntensities.Reset();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<ULightComponentBase*> Lights(*It);
		for (ULightComponentBase* Light : Lights)
		{
			// Les éclairs de tir sont créés après : ils ne sont pas touchés. Les lumières statiques ne peuvent pas changer.
			if (!Light || Light->Mobility == EComponentMobility::Static)
			{
				continue;
			}
			if (ULightComponent* Local = Cast<ULightComponent>(Light))
			{
				SavedIntensities.Emplace(Light, Local->Intensity);
				Local->SetIntensity(Local->Intensity * LightScale);
			}
			else if (USkyLightComponent* Sky = Cast<USkyLightComponent>(Light))
			{
				SavedIntensities.Emplace(Light, Sky->Intensity);
				Sky->SetIntensity(Sky->Intensity * LightScale);
			}
		}
	}

	FPostProcessSettings& Settings = ExposureLock->Settings;
	Settings.bOverride_AutoExposureMethod = true;
	Settings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Settings.AutoExposureApplyPhysicalCameraExposure = false;
	Settings.bOverride_AutoExposureBias = true;
	Settings.AutoExposureBias = BlackoutExposureBias;
	ExposureLock->BlendWeight = 1.f;

	if (PowerDownSound)
	{
		UGameplayStatics::PlaySound2D(this, PowerDownSound);
	}

	UE_LOG(LogReshoot, Log, TEXT("Noir blackout: %d lights dimmed"), SavedIntensities.Num());

	if (Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(StopTimer, this, &ANoirBlackoutEvent::StopEvent, Duration, false);
	}
}

void ANoirBlackoutEvent::StopEvent()
{
	if (!IsEventActive())
	{
		return;
	}
	Super::StopEvent();
	GetWorldTimerManager().ClearTimer(StopTimer);
	RestoreLights();

	if (PowerUpSound)
	{
		UGameplayStatics::PlaySound2D(this, PowerUpSound);
	}
}

void ANoirBlackoutEvent::RestoreLights()
{
	for (const TPair<TWeakObjectPtr<ULightComponentBase>, float>& Saved : SavedIntensities)
	{
		if (ULightComponent* Local = Cast<ULightComponent>(Saved.Key.Get()))
		{
			Local->SetIntensity(Saved.Value);
		}
		else if (USkyLightComponent* Sky = Cast<USkyLightComponent>(Saved.Key.Get()))
		{
			Sky->SetIntensity(Saved.Value);
		}
	}
	SavedIntensities.Reset();
	ExposureLock->BlendWeight = 0.f;
}

void ANoirBlackoutEvent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsEventActive())
	{
		RestoreLights();
	}
	Super::EndPlay(EndPlayReason);
}
