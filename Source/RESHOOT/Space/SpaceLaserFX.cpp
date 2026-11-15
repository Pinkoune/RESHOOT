#include "Space/SpaceLaserFX.h"
#include "RESHOOT.h"

#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName ColorParam(TEXT("Color"));
	const FName IntensityParam(TEXT("Intensity"));
	const FName FadeParam(TEXT("Fade"));

	// Les formes de base font 100 cm.
	constexpr float BasicShapeSize = 100.f;

	float EaseOut(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		return 1.f - (1.f - T) * (1.f - T);
	}
}

USpaceLaserFXComponent::USpaceLaserFXComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	CylinderMesh = Cylinder.Object;
	SphereMesh = Sphere.Object;
}

void USpaceLaserFXComponent::BeginPlay()
{
	Super::BeginPlay();

	Shots.SetNum(PoolSize);
	Impacts.SetNum(PoolSize);
	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		Cores.Add(MakeMesh(CylinderMesh, CoreIntensity, CoreMids));
		Glows.Add(MakeMesh(CylinderMesh, GlowIntensity, GlowMids));
		Bolts.Add(MakeMesh(CylinderMesh, CoreIntensity, BoltMids));
		ImpactGlows.Add(MakeMesh(SphereMesh, GlowIntensity, ImpactMids));
		ImpactLights.Add(MakeLight());
	}
	MuzzleFlash = MakeMesh(SphereMesh, CoreIntensity, MuzzleMids);
	MuzzleLight = MakeLight();
}

UStaticMeshComponent* USpaceLaserFXComponent::MakeMesh(UStaticMesh* InMesh, float Intensity, TArray<TObjectPtr<UMaterialInstanceDynamic>>& OutMids)
{
	AActor* Owner = GetOwner();
	UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(Owner);
	Comp->SetStaticMesh(InMesh);
	Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Comp->SetCastShadow(false);
	Comp->SetReceivesDecals(false);
	Comp->SetUsingAbsoluteLocation(true);
	Comp->SetUsingAbsoluteRotation(true);
	Comp->SetUsingAbsoluteScale(true);
	Comp->SetHiddenInGame(true);
	Comp->SetupAttachment(Owner->GetRootComponent());
	Comp->RegisterComponent();

	UMaterialInstanceDynamic* Mid = BeamMaterial
		? UMaterialInstanceDynamic::Create(BeamMaterial, Comp)
		: Comp->CreateAndSetMaterialInstanceDynamic(0);
	if (Mid)
	{
		Comp->SetMaterial(0, Mid);
		SetMid(Mid, Intensity, 1.f, false);
	}
	OutMids.Add(Mid);
	return Comp;
}

UPointLightComponent* USpaceLaserFXComponent::MakeLight()
{
	AActor* Owner = GetOwner();
	UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetLightColor(BeamColor);
	Light->SetCastShadows(false);
	Light->SetUsingAbsoluteLocation(true);
	Light->SetVisibility(false);
	Light->SetupAttachment(Owner->GetRootComponent());
	Light->RegisterComponent();
	return Light;
}

void USpaceLaserFXComponent::SetMid(UMaterialInstanceDynamic* Mid, float Intensity, float Fade, bool bWhiteCore)
{
	if (!Mid)
	{
		return;
	}
	// Le cœur tire vers le blanc : un laser saturé paraît plat, le centre brûlé lui donne de l'énergie.
	const FLinearColor Color = bWhiteCore ? FMath::Lerp(BeamColor, FLinearColor::White, 0.55f) : BeamColor;
	Mid->SetVectorParameterValue(ColorParam, Color);
	Mid->SetScalarParameterValue(IntensityParam, Intensity);
	Mid->SetScalarParameterValue(FadeParam, Fade);
}

void USpaceLaserFXComponent::PlaceBeam(UStaticMeshComponent* Comp, const FVector& A, const FVector& B, float Diameter)
{
	const FVector Delta = B - A;
	const float Length = Delta.Size();
	if (Length < KINDA_SMALL_NUMBER || Diameter <= 0.f)
	{
		Comp->SetHiddenInGame(true);
		return;
	}
	// Cylindre de base : 100 cm de haut, axe Z, centré.
	Comp->SetWorldLocationAndRotation(A + Delta * 0.5f, FRotationMatrix::MakeFromZ(Delta / Length).Rotator());
	Comp->SetWorldScale3D(FVector(Diameter / BasicShapeSize, Diameter / BasicShapeSize, Length / BasicShapeSize));
	Comp->SetHiddenInGame(false);
}

// ---------------------------------------------------------------------------

void USpaceLaserFXComponent::FireBeam(const FVector& From, const FVector& To, bool bHit, const FVector& HitNormal, bool bScorch)
{
	if (Shots.Num() == 0)
	{
		return;   // pas encore BeginPlay
	}

	FShot& Shot = Shots[NextShot];
	Shot.From = From;
	Shot.To = To;
	Shot.Normal = HitNormal.IsNearlyZero() ? (From - To).GetSafeNormal() : HitNormal;
	Shot.Age = 0.f;
	Shot.TravelTime = FVector::Distance(From, To) / FMath::Max(BoltSpeed, 1.f);
	Shot.bHit = bHit;
	Shot.bImpactDone = false;
	Shot.bScorch = bHit && bScorch;
	NextShot = (NextShot + 1) % Shots.Num();

	MuzzleAge = 0.f;
	MuzzleLocation = From;
	MuzzleLight->SetLightColor(BeamColor);

	UpdateTickState();
}

void USpaceLaserFXComponent::StartImpact(const FShot& Shot)
{
	// Rien à l'impact si le tir se perd dans le vide ou touche le joueur (pas de boule de lumière devant la caméra).
	if (!Shot.bHit)
	{
		return;
	}

	FImpact& Impact = Impacts[NextImpact];
	Impact.Location = Shot.To + Shot.Normal * 2.f;
	Impact.Age = 0.f;
	ImpactLights[NextImpact]->SetLightColor(BeamColor);
	NextImpact = (NextImpact + 1) % Impacts.Num();
	if (ImpactSparks)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactSparks, Shot.To, Shot.Normal.Rotation(), FVector(0.3f), true, true, ENCPoolMethod::AutoRelease);
	}
	if (ScorchMaterial && Shot.bScorch)
	{
		// Le décal projette le long de son X : on l'oriente contre la surface.
		const FRotator DecalRot = (-Shot.Normal).Rotation() + FRotator(0.f, 0.f, FMath::FRandRange(0.f, 360.f));
		if (UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(this, ScorchMaterial, FVector(25.f, ScorchSize, ScorchSize), Shot.To, DecalRot, ScorchLifetime))
		{
			// DecalLifetimeOpacity descend de 1 à 0 : le point chaud refroidit, puis la brûlure s'efface.
			Decal->SetFadeOut(0.f, ScorchLifetime, false);
			Decal->SetFadeScreenSize(0.f);
		}
	}
}

void USpaceLaserFXComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// --- Rayons et traits ---
	for (int32 Index = 0; Index < Shots.Num(); ++Index)
	{
		FShot& Shot = Shots[Index];
		if (Shot.Age < 0.f)
		{
			continue;
		}
		Shot.Age += DeltaTime;

		if (!Shot.bImpactDone && Shot.Age >= Shot.TravelTime)
		{
			Shot.bImpactDone = true;
			StartImpact(Shot);
		}

		const float Life = FMath::Max(BeamLifetime, Shot.TravelTime);
		const float T = Shot.Age / Life;
		if (T >= 1.f)
		{
			Shot.Age = -1.f;
			Cores[Index]->SetHiddenInGame(true);
			Glows[Index]->SetHiddenInGame(true);
			Bolts[Index]->SetHiddenInGame(true);
			continue;
		}

		// Le rayon s'allume d'un coup, puis s'amincit et s'éteint ; il ne dépasse jamais le trait.
		const float Fade = FMath::Square(1.f - T);
		const FVector Dir = (Shot.To - Shot.From).GetSafeNormal();
		const float Length = FVector::Distance(Shot.From, Shot.To);
		const float Head = FMath::Min(Shot.Age * BoltSpeed, Length);
		const FVector HeadPos = Shot.From + Dir * Head;

		PlaceBeam(Cores[Index], Shot.From, HeadPos, CoreThickness * FMath::Lerp(0.4f, 1.f, Fade));
		PlaceBeam(Glows[Index], Shot.From, HeadPos, GlowThickness * FMath::Lerp(0.6f, 1.f, Fade));
		SetMid(CoreMids[Index], CoreIntensity, Fade, true);
		SetMid(GlowMids[Index], GlowIntensity, Fade, false);

		if (Head < Length)
		{
			PlaceBeam(Bolts[Index], Shot.From + Dir * FMath::Max(Head - BoltLength, 0.f), HeadPos, BoltThickness);
			SetMid(BoltMids[Index], CoreIntensity * 1.5f, 1.f, true);
		}
		else
		{
			Bolts[Index]->SetHiddenInGame(true);
		}
	}

	// --- Éclair de bouche ---
	if (MuzzleAge >= 0.f)
	{
		MuzzleAge += DeltaTime;
		const float T = MuzzleAge / MuzzleFlashTime;
		if (T >= 1.f)
		{
			MuzzleAge = -1.f;
			MuzzleFlash->SetHiddenInGame(true);
			MuzzleLight->SetVisibility(false);
		}
		else
		{
			const float Fade = 1.f - T;
			const float Size = MuzzleFlashSize * FMath::Lerp(0.6f, 1.2f, EaseOut(T)) / BasicShapeSize;
			MuzzleFlash->SetWorldLocation(MuzzleLocation);
			MuzzleFlash->SetWorldScale3D(FVector(Size));
			MuzzleFlash->SetHiddenInGame(false);
			SetMid(MuzzleMids[0], CoreIntensity * 0.5f, Fade, true);
			MuzzleLight->SetWorldLocation(MuzzleLocation);
			MuzzleLight->SetAttenuationRadius(MuzzleLightRadius);
			MuzzleLight->SetIntensity(MuzzleLightIntensity * Fade);
			MuzzleLight->SetVisibility(true);
		}
	}

	// --- Impacts ---
	for (int32 Index = 0; Index < Impacts.Num(); ++Index)
	{
		FImpact& Impact = Impacts[Index];
		if (Impact.Age < 0.f)
		{
			continue;
		}
		Impact.Age += DeltaTime;
		const float T = Impact.Age / ImpactTime;
		if (T >= 1.f)
		{
			Impact.Age = -1.f;
			ImpactGlows[Index]->SetHiddenInGame(true);
			ImpactLights[Index]->SetVisibility(false);
			continue;
		}
		const float Fade = FMath::Square(1.f - T);
		const float Size = ImpactGlowSize * FMath::Lerp(0.4f, 1.3f, EaseOut(T)) / BasicShapeSize;
		ImpactGlows[Index]->SetWorldLocation(Impact.Location);
		ImpactGlows[Index]->SetWorldScale3D(FVector(Size));
		ImpactGlows[Index]->SetHiddenInGame(false);
		SetMid(ImpactMids[Index], GlowIntensity * 1.5f, Fade, false);
		ImpactLights[Index]->SetWorldLocation(Impact.Location);
		ImpactLights[Index]->SetAttenuationRadius(ImpactLightRadius);
		ImpactLights[Index]->SetIntensity(ImpactLightIntensity * Fade);
		ImpactLights[Index]->SetVisibility(true);
	}

	UpdateTickState();
}

void USpaceLaserFXComponent::UpdateTickState()
{
	bool bActive = MuzzleAge >= 0.f;
	for (const FShot& Shot : Shots)
	{
		bActive |= Shot.Age >= 0.f;
	}
	for (const FImpact& Impact : Impacts)
	{
		bActive |= Impact.Age >= 0.f;
	}
	SetComponentTickEnabled(bActive);
}

void USpaceLaserFXComponent::HideAll()
{
	for (FShot& Shot : Shots)
	{
		Shot.Age = -1.f;
	}
	for (FImpact& Impact : Impacts)
	{
		Impact.Age = -1.f;
	}
	MuzzleAge = -1.f;
	auto HideMeshes = [](const TArray<TObjectPtr<UStaticMeshComponent>>& Comps)
	{
		for (UStaticMeshComponent* Comp : Comps)
		{
			if (Comp)
			{
				Comp->SetHiddenInGame(true);
			}
		}
	};
	HideMeshes(Cores);
	HideMeshes(Glows);
	HideMeshes(Bolts);
	HideMeshes(ImpactGlows);
	for (UPointLightComponent* Light : ImpactLights)
	{
		if (Light)
		{
			Light->SetVisibility(false);
		}
	}
	if (MuzzleFlash)
	{
		MuzzleFlash->SetHiddenInGame(true);
	}
	if (MuzzleLight)
	{
		MuzzleLight->SetVisibility(false);
	}
	SetComponentTickEnabled(false);
}
