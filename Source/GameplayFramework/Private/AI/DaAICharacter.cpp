// Copyright Dream Awake Solutions LLC


#include "AI/DaAICharacter.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "CoreGameplayTags.h"
#include "AbilitySystem/DaAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/DaCharacterAttributeSet.h"
#include "AbilitySystem/Attributes/DaCombatAttributeSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/UserWidget.h"
#include "DaAttributeComponent.h"
#include "TimerManager.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "UI/DaWorldUserWidget.h"


// Sets default values
ADaAICharacter::ADaAICharacter()
{
	CharacterTypeGameplayTag = CoreGameplayTags::TAG_Character_Type_AI;
	
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>("AIPerceptionComp");

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>("SightConfig");
	SightConfig->SightRadius = 3000.0f;
	SightConfig->LoseSightRadius = 3500.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
	
	AbilitySystemComponent = CreateDefaultSubobject<UDaAbilitySystemComponent>(TEXT("AbilityComp"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	// These attribute sets will be detected by AbilitySystemComponent::InitializeComponent. Keeping a reference so that the sets don't get garbage collected before that.
	HealthSet = CreateDefaultSubobject<UDaCharacterAttributeSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<UDaCombatAttributeSet>(TEXT("CombatSet"));
	
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	
	TargetActorKey = "TargetActor";
	
	PlayerSeenEmoteTime = 6.0f;
}

void ADaAICharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ADaAICharacter::OnTargetPerceptionUpdated);
}

void ADaAICharacter::InitAbilitySystem()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	Cast<UDaAbilitySystemComponent>(GetAbilitySystemComponent())->AbilityActorInfoSet();
	
	// read from data asset and set up abilities, attributes, and effects
	AttributeComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
	
	InitDefaultAttributes();
	
	Super::InitAbilitySystem();
}

int32 ADaAICharacter::GetCharacterLevel()
{
	return Level;
}

void ADaAICharacter::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	APawn* Pawn = Cast<APawn>(Actor);
	if (Pawn && Stimulus.WasSuccessfullySensed())
	{
		OnPawnSeen(Pawn);
	}
}

void ADaAICharacter::OnPawnSeen(APawn* Pawn)
{
	if (GetTargetActor() == Pawn)
	{
		return;
	}
	
	SetTargetActor(Pawn); //@TODO: Toggle mode to set player only vs all characters so game designers can choose

	MulticastOnPawnSeen(Pawn);
}

void ADaAICharacter::MulticastOnPawnSeen_Implementation(APawn* Pawn)
{
	// Add some cool Widget or emote to NPC to signify when they've spotted the player
	if (PlayerSeenWidget == nullptr)
	{
		PlayerSeenWidget = CreateWidget<UDaWorldUserWidget>(GetWorld(), PlayerSeenWidgetClass);
		if (PlayerSeenWidget)
		{
			PlayerSeenWidget->AttachedActor = this;
			PlayerSeenWidget->AddToViewport(10);

			FTimerHandle TimerHandle_PlayerSeenElapsed;
			GetWorldTimerManager().SetTimer(TimerHandle_PlayerSeenElapsed, this, &ADaAICharacter::PlayerSeenWidgetTimeExpired, PlayerSeenEmoteTime );
		}

		//FString Msg = FString::Printf(TEXT("%s SPOTTED"), *GetNameSafe(Pawn));
		//DrawDebugString(GetWorld(), GetActorLocation(), Msg, nullptr, FColor::White, PlayerSeenEmoteTime, true);
	}
}

void ADaAICharacter::PlayerSeenWidgetTimeExpired()
{
	PlayerSeenWidget->RemoveFromParent();
	PlayerSeenWidget->AttachedActor = nullptr;
	PlayerSeenWidget = nullptr;
}

void ADaAICharacter::OnHealthChanged(UDaAttributeComponent* OwningComp, float OldHealth, float NewHealth, AActor* InstigatorActor)
{
	Super::OnHealthChanged(OwningComp, OldHealth, NewHealth, InstigatorActor);

	// subclass this to set TargetActor Blackboard key to instigator that hit us
	// TODO: Make better decisions on wether or not to attack other non player NPCs
	if (NewHealth < OldHealth)
	{
		// Took Damage
		if (InstigatorActor != this)
		{
			SetTargetActor(InstigatorActor);
		}
	}
}

void ADaAICharacter::OnDeathStarted(AActor* OwningActor, AActor* InstigatorActor)
{
	// stop BT
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->GetBrainComponent()->StopLogic("Killed");
	}

	// Call super to ragdoll
	Super::OnDeathStarted(OwningActor, InstigatorActor);
}

void ADaAICharacter::SetTargetActor(AActor* NewTarget)
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->GetBlackboardComponent()->SetValueAsObject(TargetActorKey, NewTarget);
	}
}

AActor* ADaAICharacter::GetTargetActor()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		return Cast<AActor>(AIController->GetBlackboardComponent()->GetValueAsObject(TargetActorKey));
	}
	return nullptr;
}


