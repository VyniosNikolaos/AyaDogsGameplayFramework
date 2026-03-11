// Copyright Dream Awake Solutions LLC

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "DaCharacterBase.h"
#include "DaAICharacter.generated.h"

class UDaAbilitySystemComponent;
class UDaAttributeComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UUserWidget;
class UDaWorldUserWidget;

UCLASS()
class GAMEPLAYFRAMEWORK_API ADaAICharacter : public ADaCharacterBase 
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ADaAICharacter();
	
	virtual void InitAbilitySystem() override;
	
protected:

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComp;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleDefaultsOnly, Category="UI")
	TObjectPtr<UDaWorldUserWidget> PlayerSeenWidget;
	
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> PlayerSeenWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	float PlayerSeenEmoteTime;
	
	UPROPERTY(VisibleDefaultsOnly, Category="AI")
	FName TargetActorKey;

	// Health attribute set used by this actor.
	UPROPERTY()
	TObjectPtr<const class UDaCharacterAttributeSet> HealthSet;
	// Combat attribute set used by this actor.
	UPROPERTY()
	TObjectPtr<const class UDaCombatAttributeSet> CombatSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character Setup")
	int32 Level = 1;

	virtual int32 GetCharacterLevel() override;
	
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void OnPawnSeen(APawn* Pawn);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastOnPawnSeen(APawn* Pawn);

	AActor* GetTargetActor();
	void SetTargetActor(AActor* NewTarget);

	void PlayerSeenWidgetTimeExpired();
	
	virtual void PostInitializeComponents() override;

	// override so AI character can set blackboard keys, still calls super to handle health change
	virtual void OnHealthChanged(UDaAttributeComponent* HealthComponent, float OldHealth, float NewHealth, AActor* InstigatorActor) override;

	// override so AI character can set blackboard keys, still calls super to handle death
	virtual void OnDeathStarted(AActor* OwningActor, AActor* InstigatorActor) override;

};
