#include "Medieval/MedievalHorseConvoyEvent.h"
#include "Core/HealthComponent.h"
#include "Core/ReshootCharacter.h"
#include "Core/ReshootEnemy.h"
#include "Director/ShootDirectorSubsystem.h"
#include "RESHOOT.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Convoi : 3 chevaux de bois à la suite, puis 2 techniciens qui poussent. Axe de marche = +Y local du convoi.
	constexpr int32 NumHorses = 3;
	constexpr float HorseSpacing = 260.f;
	constexpr float ConvoyFront = 130.f;                                        // nez du premier cheval
	constexpr float ConvoyBack = -(NumHorses - 1) * HorseSpacing - 170.f;      // dos des techniciens
	constexpr float RailGauge = 45.f;
	constexpr float SleeperStep = 150.f;
}

AMedievalHorseConvoyEvent::AMedievalHorseConvoyEvent()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Convoy = CreateDefaultSubobject<USceneComponent>(TEXT("Convoy"));
	Convoy->SetupAttachment(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WoodMat(TEXT("/Game/RESHOOT/Medieval/Materials/MI_Medieval_Bois.MI_Medieval_Bois"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PaintMat(TEXT("/Game/RESHOOT/Medieval/Materials/MI_Medieval_Paille.MI_Medieval_Paille"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ClothMat(TEXT("/Game/RESHOOT/Medieval/Materials/MI_Medieval_Toile.MI_Medieval_Toile"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BlackMat(TEXT("/Game/RESHOOT/Medieval/Materials/MI_Studio_Noir.MI_Studio_Noir"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MetalMat(TEXT("/Game/RESHOOT/Medieval/Materials/MI_Studio_Metal.MI_Studio_Metal"));

	int32 PartIndex = 0;
	auto AddPart = [&](UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& Location, const FVector& Scale, const FRotator& Rotation)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ConvoyPart_%02d"), PartIndex++));
		Part->SetupAttachment(Convoy);
		Part->SetStaticMesh(Mesh);
		if (Material)
		{
			Part->SetMaterial(0, Material);
		}
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetRelativeLocationAndRotation(Location, Rotation);
		Part->SetRelativeScale3D(Scale);
		ConvoyParts.Add(Part);
	};

	UStaticMesh* Cube = CubeMesh.Object;
	UStaticMesh* Cylinder = CylinderMesh.Object;
	UMaterialInterface* Wood = WoodMat.Object;
	UMaterialInterface* Paint = PaintMat.Object;
	UMaterialInterface* Cloth = ClothMat.Object;
	UMaterialInterface* Black = BlackMat.Object;

	for (int32 i = 0; i < NumHorses; ++i)
	{
		const float Y = -i * HorseSpacing;
		// Plateforme à roulettes
		AddPart(Cube, Wood, FVector(0.f, Y, 30.f), FVector(1.0f, 1.9f, 0.15f), FRotator::ZeroRotator);
		for (int32 w = 0; w < 4; ++w)
		{
			const float WX = (w % 2 == 0) ? -55.f : 55.f;
			const float WY = Y + ((w < 2) ? 70.f : -70.f);
			// Axe de la roue le long de X (perpendiculaire à la marche).
			AddPart(Cylinder, Black, FVector(WX, WY, 18.f), FVector(0.32f, 0.32f, 0.08f), FRotator(90.f, 0.f, 0.f));
		}
		// Cheval de bois peint : pattes, corps, encolure, tête, selle rouge
		for (int32 l = 0; l < 4; ++l)
		{
			const float LX = (l % 2 == 0) ? -18.f : 18.f;
			const float LY = Y + ((l < 2) ? 50.f : -50.f);
			AddPart(Cube, Wood, FVector(LX, LY, 75.f), FVector(0.12f, 0.12f, 0.75f), FRotator::ZeroRotator);
		}
		AddPart(Cube, Paint, FVector(0.f, Y, 130.f), FVector(0.45f, 1.35f, 0.5f), FRotator::ZeroRotator);
		AddPart(Cube, Cloth, FVector(0.f, Y - 5.f, 158.f), FVector(0.5f, 0.5f, 0.08f), FRotator::ZeroRotator);
		AddPart(Cube, Paint, FVector(0.f, Y + 70.f, 175.f), FVector(0.25f, 0.28f, 0.65f), FRotator(0.f, 0.f, -25.f));
		AddPart(Cube, Paint, FVector(0.f, Y + 100.f, 210.f), FVector(0.25f, 0.55f, 0.25f), FRotator::ZeroRotator);
		AddPart(Cube, Black, FVector(0.f, Y - 75.f, 125.f), FVector(0.08f, 0.08f, 0.5f), FRotator(0.f, 0.f, 30.f));
	}
	// Deux techniciens en noir qui poussent le dernier cheval
	const float TechY = -(NumHorses - 1) * HorseSpacing - 140.f;
	for (int32 t = 0; t < 2; ++t)
	{
		const float TX = t == 0 ? -35.f : 35.f;
		AddPart(Cube, Black, FVector(TX, TechY, 85.f), FVector(0.4f, 0.3f, 1.5f), FRotator(0.f, 0.f, -12.f));
		AddPart(Cube, Black, FVector(TX, TechY + 15.f, 175.f), FVector(0.25f, 0.25f, 0.25f), FRotator::ZeroRotator);
	}

	// Zone de choc : tout le convoi.
	Hitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	Hitbox->SetupAttachment(Convoy);
	Hitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hitbox->SetBoxExtent(FVector(80.f, (ConvoyFront - ConvoyBack) * 0.5f, 120.f));
	Hitbox->SetRelativeLocation(FVector(0.f, (ConvoyFront + ConvoyBack) * 0.5f, 120.f));

	// Rails et traverses, toujours visibles : ils font partie du décor.
	auto MakeRail = [&](const TCHAR* Name, float X)
	{
		UStaticMeshComponent* Rail = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Rail->SetupAttachment(Root);
		Rail->SetStaticMesh(Cube);
		if (MetalMat.Object)
		{
			Rail->SetMaterial(0, MetalMat.Object);
		}
		Rail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Rail->SetRelativeLocation(FVector(X, 0.f, 3.f));
		return Rail;
	};
	RailLeft = MakeRail(TEXT("RailLeft"), -RailGauge);
	RailRight = MakeRail(TEXT("RailRight"), RailGauge);

	Sleepers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Sleepers"));
	Sleepers->SetupAttachment(Root);
	Sleepers->SetStaticMesh(Cube);
	if (Wood)
	{
		Sleepers->SetMaterial(0, Wood);
	}
	Sleepers->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Convoy->SetVisibility(false, true);
}

void AMedievalHorseConvoyEvent::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateRailsAndSleepers();
	PlaceConvoyAtStart();
}

void AMedievalHorseConvoyEvent::UpdateRailsAndSleepers()
{
	const float Length = RailHalfLength * 2.f;
	RailLeft->SetRelativeScale3D(FVector(0.06f, Length / 100.f, 0.06f));
	RailRight->SetRelativeScale3D(FVector(0.06f, Length / 100.f, 0.06f));

	Sleepers->ClearInstances();
	for (float Y = -RailHalfLength; Y <= RailHalfLength; Y += SleeperStep)
	{
		Sleepers->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.f, Y, 1.f), FVector(1.3f, 0.2f, 0.03f)));
	}
}

void AMedievalHorseConvoyEvent::BeginPlay()
{
	Super::BeginPlay();
	UpdateRailsAndSleepers();
	PlaceConvoyAtStart();

	if (UShootDirectorSubsystem* Director = GetWorld()->GetSubsystem<UShootDirectorSubsystem>())
	{
		Director->OnCut.AddDynamic(this, &AMedievalHorseConvoyEvent::HandleCut);
		Director->OnAction.AddDynamic(this, &AMedievalHorseConvoyEvent::HandleAction);
	}

	if (AutoStartInterval > 0.f)
	{
		GetWorldTimerManager().SetTimer(AutoStartTimer, this, &AMedievalHorseConvoyEvent::StartEvent, AutoStartInterval, true, AutoStartInterval);
	}
}

void AMedievalHorseConvoyEvent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (UWorld* World = GetWorld())
	{
		if (UShootDirectorSubsystem* Director = World->GetSubsystem<UShootDirectorSubsystem>())
		{
			Director->OnCut.RemoveDynamic(this, &AMedievalHorseConvoyEvent::HandleCut);
			Director->OnAction.RemoveDynamic(this, &AMedievalHorseConvoyEvent::HandleAction);
		}
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------

void AMedievalHorseConvoyEvent::PlaceConvoyAtStart()
{
	// Le convoi entre par le bout de départ, tête en avant.
	ConvoyY = -Direction * RailHalfLength;
	Convoy->SetRelativeLocationAndRotation(FVector(0.f, ConvoyY, 0.f), FRotator(0.f, Direction > 0.f ? 0.f : 180.f, 0.f));
}

void AMedievalHorseConvoyEvent::StartEvent()
{
	if (IsEventActive())
	{
		return;
	}
	Super::StartEvent();

	HitThisPass.Reset();
	bRolling = false;
	WarningElapsed = 0.f;
	PlaceConvoyAtStart();
	Convoy->SetVisibility(true, true);
	SetActorTickEnabled(true);

	if (WarningSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WarningSound, Convoy->GetComponentLocation());
	}
	UE_LOG(LogReshoot, Log, TEXT("Medieval convoy: warning (%s)"), Direction > 0.f ? TEXT("+Y") : TEXT("-Y"));

	GetWorldTimerManager().SetTimer(WarningTimer, this, &AMedievalHorseConvoyEvent::BeginRolling, FMath::Max(WarningDelay, 0.01f), false);
	if (CustomTimeDilation == 0.f)
	{
		GetWorldTimerManager().PauseTimer(WarningTimer);
	}
}

void AMedievalHorseConvoyEvent::BeginRolling()
{
	bRolling = true;
	if (RollingSound)
	{
		UGameplayStatics::SpawnSoundAttached(RollingSound, Convoy);
	}
}

void AMedievalHorseConvoyEvent::StopEvent()
{
	if (!IsEventActive())
	{
		return;
	}
	Super::StopEvent();

	GetWorldTimerManager().ClearTimer(WarningTimer);
	bRolling = false;
	SetActorTickEnabled(false);
	Convoy->SetVisibility(false, true);

	// Prochain passage dans l'autre sens.
	Direction = -Direction;
	PlaceConvoyAtStart();
	UE_LOG(LogReshoot, Log, TEXT("Medieval convoy: done"));
}

void AMedievalHorseConvoyEvent::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bRolling)
	{
		// En attente : le convoi trépigne en bout de rail.
		WarningElapsed += DeltaSeconds;
		const float Shake = FMath::Sin(WarningElapsed * 40.f) * 4.f;
		Convoy->SetRelativeLocation(FVector(Shake, ConvoyY, FMath::Abs(Shake) * 0.5f));
		return;
	}

	ConvoyY += Direction * Speed * DeltaSeconds;
	Convoy->SetRelativeLocation(FVector(0.f, ConvoyY, 0.f));
	HitPawnsInPath();

	// Fin de course : la queue du convoi a dépassé le bout des rails.
	if (ConvoyY * Direction > RailHalfLength + (ConvoyFront - ConvoyBack))
	{
		StopEvent();
	}
}

void AMedievalHorseConvoyEvent::HitPawnsInPath()
{
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MedievalConvoy), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Hitbox->GetComponentLocation(), Hitbox->GetComponentQuat(), ObjectParams,
		FCollisionShape::MakeBox(Hitbox->GetScaledBoxExtent()), Params);

	const FVector RailSide = GetActorForwardVector().GetSafeNormal2D(); // perpendiculaire aux rails (qui suivent l'axe Y)

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || HitThisPass.Contains(Target))
		{
			continue;
		}
		UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
		if (!Health || Health->IsDead())
		{
			continue;
		}
		HitThisPass.Add(Target);

		// Projeté sur le côté où il se trouve, et vers le haut.
		const float Side = FVector::DotProduct(Target->GetActorLocation() - GetActorLocation(), RailSide) >= 0.f ? 1.f : -1.f;
		const FVector Launch = RailSide * Side * KnockbackSpeed + GetActorRightVector() * Direction * KnockbackSpeed * 0.4f + FVector(0.f, 0.f, KnockbackSpeed * 0.5f);
		if (ACharacter* Character = Cast<ACharacter>(Target))
		{
			Character->LaunchCharacter(Launch, true, true);
		}

		const bool bPlayer = Target->IsA<AReshootCharacter>();
		Health->ApplyDamage(bPlayer ? PlayerDamage : EnemyDamage, this);

		if (ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Target->GetActorLocation());
		}
		UE_LOG(LogReshoot, Log, TEXT("Medieval convoy: knocked over %s"), *GetNameSafe(Target));
	}
}

void AMedievalHorseConvoyEvent::HandleCut()
{
	CustomTimeDilation = 0.f;
	GetWorldTimerManager().PauseTimer(WarningTimer);
	GetWorldTimerManager().PauseTimer(AutoStartTimer);
}

void AMedievalHorseConvoyEvent::HandleAction()
{
	CustomTimeDilation = 1.f;
	GetWorldTimerManager().UnPauseTimer(WarningTimer);
	GetWorldTimerManager().UnPauseTimer(AutoStartTimer);
}
