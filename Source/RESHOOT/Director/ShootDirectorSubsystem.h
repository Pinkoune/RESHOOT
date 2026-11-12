// RESHOOT — machine à états de la partie : scènes, prises, « CUT! », final, fin.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/ReshootTypes.h"
#include "ShootDirectorSubsystem.generated.h"

class UShootScript;
class UDirective;
class UUniverseDefinition;
class AWaveSpawner;
class UAudienceRating;
class UDirectorVoice;
class AUniverseEvent;
class AReshootCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShootPhaseChanged, EShootPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShootUniverseChanged, EUniverse, NewUniverse);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShootCut);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShootAction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDirectiveChanged, UDirective*, NewDirective);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnKillScored, const FKillInfo&, Kill, float, Points, bool, bDirectiveRespected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUniverseEventAnnounced, AUniverseEvent*, Event);

UCLASS()
class RESHOOT_API UShootDirectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- API publique (docs/ARCHITECTURE.md) ---

	/** Lance le tournage depuis la première scène du script. Appelé par le menu. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void StartShoot();

	UFUNCTION(BlueprintPure, Category = "Director")
	EShootPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "Director")
	EUniverse GetCurrentUniverse() const { return CurrentUniverse; }

	/** Appelé par AReshootEnemy à sa mort. Met l'audimat à jour selon la directive. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void NotifyKill(const FKillInfo& Kill);

	/** Appelé par AReshootCharacter à sa mort : « on la refait », la prise recommence. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void NotifyPlayerDeath();

	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnShootPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnShootUniverseChanged OnUniverseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnShootCut OnCut;

	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnShootAction OnAction;

	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnDirectiveChanged OnDirectiveChanged;

	/** Après chaque élimination : points gagnés et directive respectée ou non (le « + 25 » du HUD). */
	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnKillScored OnKillScored;

	/** Annonce d'un événement d'univers, EventWarningTime avant son déclenchement. */
	UPROPERTY(BlueprintAssignable, Category = "Director")
	FOnUniverseEventAnnounced OnUniverseEventAnnounced;

	// --- Compléments ---

	/** Casting : place le joueur au PlayerStart CastingPlayerStartTag, phase Casting, réplique d'accueil. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void StartCasting();

	/** Fin du casting (le joueur a fini le tutoriel) : enchaîne sur StartShoot(). */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void FinishCasting();

	/** Le joueur (joueur 0). Peut être nul. */
	UFUNCTION(BlueprintPure, Category = "Director")
	AReshootCharacter* GetPlayer() const;

	/** Fourni par AReshootGameMode au BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void SetShootScript(UShootScript* InScript);

	UFUNCTION(BlueprintPure, Category = "Director")
	UShootScript* GetShootScript() const { return Script; }

	/** Définition d'un univers d'après le script courant. Peut être nulle. */
	UFUNCTION(BlueprintPure, Category = "Director")
	UUniverseDefinition* GetUniverseDefinition(EUniverse Universe) const;

	UFUNCTION(BlueprintPure, Category = "Director")
	UDirective* GetCurrentDirective() const { return CurrentDirective; }

	UFUNCTION(BlueprintPure, Category = "Director")
	int32 GetSceneIndex() const { return SceneIndex; }

	UFUNCTION(BlueprintPure, Category = "Director")
	int32 GetTakeIndex() const { return TakeIndex; }

	/** « CUT! » : gel général, réplique, nouvelle directive (peut être nulle), puis « Action! » après CutDuration. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void CallCut(UDirective* NewDirective);

	/** « Action! » : dégel général. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void CallAction();

	/** Change d'univers : joueur (arme, déplacement), sous-niveau, delegate. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void SetUniverse(EUniverse NewUniverse);

	/** Gèle ou dégèle joueur et figurants. */
	UFUNCTION(BlueprintCallable, Category = "Director")
	void FreezeAll(bool bFreeze);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

private:
	void SetPhase(EShootPhase NewPhase);
	void SetDirective(UDirective* NewDirective);

	void BeginScene(int32 NewSceneIndex);
	void BeginTake(int32 NewTakeIndex);
	void StartNextWave();
	void EndTake();
	void RestartTake();
	void BeginFinale();
	void FinaleSwitchUniverse();
	void Wrap();

	UFUNCTION()
	void HandleWaveCleared(AWaveSpawner* Spawner);

	AWaveSpawner* FindSpawner();
	UAudienceRating* GetAudienceRating() const;
	UDirectorVoice* GetDirectorVoice() const;
	float PlayLine(FName LineId) const;

	UPROPERTY()
	TObjectPtr<UShootScript> Script;

	UPROPERTY()
	TObjectPtr<UDirective> CurrentDirective;

	UPROPERTY()
	TWeakObjectPtr<AWaveSpawner> Spawner;

	EShootPhase Phase = EShootPhase::Menu;
	EUniverse CurrentUniverse = EUniverse::None;
	int32 SceneIndex = INDEX_NONE;
	int32 TakeIndex = INDEX_NONE;
	int32 WaveIndex = INDEX_NONE;
	int32 FinaleUniverseIndex = 0;

	UPROPERTY()
	TObjectPtr<AUniverseEvent> ActiveEvent;

	bool bPlayerDiedThisTake = false;

	FTimerHandle PhaseTimer;
	FTimerHandle CutTimer;
	FTimerHandle EventTimer;
	FTimerHandle FinaleSwitchTimer;
	FTimerHandle FinaleEndTimer;
};
