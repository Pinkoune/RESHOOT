// RESHOOT — HUD de jeu, façon régie de tournage (GDD, section Interface). Dessiné au Canvas, sans asset :
//  - haut gauche : « Scène 2, prise 3 » + voyant REC ; haut centre : directive ; haut droite : audimat (pulse + « +25 »)
//  - bas gauche : vie ; bas droite : arme (munitions, mode médiéval, charge de l'arc) ; bas centre : sous-titres
//  - centre : réticule selon l'arme, marqueur de touche ; bord rouge quand le joueur est touché ; bandes noires au CUT!
//  - combo médiéval sous l'audimat quand l'arme médiévale est en main.
// L'interface écoute les delegates et lit les getters ; elle n'appelle jamais la logique de jeu.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ReshootHUD.generated.h"

class UFont;
class UHealthComponent;
class AReshootEnemy;

UCLASS(Blueprintable)
class RESHOOT_API AReshootHUD : public AHUD
{
	GENERATED_BODY()

public:
	AReshootHUD();

	virtual void DrawHUD() override;

	/** Police de l'interface (par défaut Roboto du moteur). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UFont> Font;

	/** Taille de texte de base, multipliée par la hauteur d'écran / 1080. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (ClampMin = "0.1"))
	float TextScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	FLinearColor AccentColor = FLinearColor(1.f, 0.78f, 0.25f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	FLinearColor RecColor = FLinearColor(0.95f, 0.1f, 0.08f, 1.f);

	/** Durée du marqueur de touche au centre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (ClampMin = "0"))
	float HitMarkerDuration = 0.18f;

	/** Durée du bord rouge quand le joueur est touché. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD", meta = (ClampMin = "0"))
	float DamageFlashDuration = 0.45f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// Dessin
	float UIScale() const;
	void DrawTextAt(const FString& Text, float X, float Y, float Scale, const FLinearColor& Color, bool bCenterX = false, bool bShadow = true);
	FVector2D MeasureText(const FString& Text, float Scale) const;
	void DrawBar(float X, float Y, float W, float H, float Pct, const FLinearColor& Fill, const FLinearColor& Back);

	void DrawTake();
	void DrawDirective();
	void DrawAudience();
	void DrawHealth();
	void DrawWeapon();
	void DrawSubtitle();
	void DrawReticle();
	void DrawDamageFlash();
	void DrawCutBars();

	// Écoute
	void TryBind();
	void BindEnemy(AActor* Actor);
	void HandleActorSpawned(AActor* Actor);

	UFUNCTION()
	void HandleRatingChanged(float NewRating, float Delta);

	UFUNCTION()
	void HandleSubtitle(FName LineId, const FText& Subtitle, float Duration);

	UFUNCTION()
	void HandlePlayerHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta);

	UFUNCTION()
	void HandleEnemyHealthChanged(UHealthComponent* HealthComponent, float NewHealth, float Delta);

	double Now() const;

	bool bBoundGlobal = false;
	FTimerHandle BindRetryTimer;
	FDelegateHandle ActorSpawnedHandle;

	UPROPERTY(Transient)
	TObjectPtr<UHealthComponent> BoundPlayerHealth;

	FText SubtitleText;
	double SubtitleEndTime = 0.0;
	double RatingPulseTime = -100.0;
	double HitMarkerTime = -100.0;
	double DamageFlashTime = -100.0;

	struct FFloatingGain
	{
		FString Text;
		double StartTime = 0.0;
	};
	TArray<FFloatingGain> FloatingGains;
};
