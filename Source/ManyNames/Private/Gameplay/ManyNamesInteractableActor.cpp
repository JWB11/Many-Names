#include "Gameplay/ManyNamesInteractableActor.h"

#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Systems/ManyNamesContentSubsystem.h"
#include "Gameplay/ManyNamesPrototypeGameMode.h"
#include "Systems/ManyNamesWorldStateSubsystem.h"

namespace
{
void SnapInteractableActorToGround(AActor* Actor, float MaxTraceDistance)
{
	if (!Actor || !Actor->GetWorld())
	{
		return;
	}

	const FVector Start = Actor->GetActorLocation() + FVector(0.0f, 0.0f, 240.0f);
	const FVector End = Start - FVector(0.0f, 0.0f, MaxTraceDistance);
	FHitResult HitResult;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ManyNamesInteractableGrounding), false, Actor);
	if (Actor->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params) && HitResult.bBlockingHit)
	{
		const FVector GroundedLocation(HitResult.ImpactPoint.X, HitResult.ImpactPoint.Y, HitResult.ImpactPoint.Z + 2.0f);
		Actor->SetActorLocation(GroundedLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}
}

AManyNamesInteractableActor::AManyNamesInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMeshComponent->SetupAttachment(MeshComponent);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));

	if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		MeshComponent->SetStaticMesh(CubeMesh);
	}
}

void AManyNamesInteractableActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyNpcVisualProfile();
}

void AManyNamesInteractableActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyNpcVisualProfile();
}

FText AManyNamesInteractableActor::GetInteractionLabel_Implementation() const
{
	return InteractionLabel;
}

bool AManyNamesInteractableActor::CanInteract_Implementation(APawn* InteractingPawn) const
{
	if (bSingleUse && bConsumed)
	{
		return false;
	}

	if (RequiredOutputs.Num() == 0)
	{
		if (InteractionType != EManyNamesInteractionActionType::QuestDialogue || QuestId.IsNone())
		{
			return true;
		}
	}

	if (!GetGameInstance())
	{
		return false;
	}

	const UManyNamesWorldStateSubsystem* WorldStateSubsystem = GetGameInstance()->GetSubsystem<UManyNamesWorldStateSubsystem>();
	if (!WorldStateSubsystem)
	{
		return false;
	}

	if (InteractionType == EManyNamesInteractionActionType::QuestDialogue && !QuestId.IsNone())
	{
		const UManyNamesContentSubsystem* ContentSubsystem = GetGameInstance()->GetSubsystem<UManyNamesContentSubsystem>();
		if (ContentSubsystem)
		{
			FManyNamesQuestRow QuestRow;
			if (ContentSubsystem->GetQuestRow(QuestId, QuestRow) &&
				!QuestRow.WorldStateOutputId.IsNone() &&
				WorldStateSubsystem->HasWorldStateOutput(QuestRow.WorldStateOutputId))
			{
				return false;
			}
		}
	}

	for (const FName& OutputId : RequiredOutputs)
	{
		if (!WorldStateSubsystem->HasWorldStateOutput(OutputId))
		{
			return false;
		}
	}

	return true;
}

void AManyNamesInteractableActor::Interact_Implementation(APawn* InteractingPawn)
{
	AManyNamesPrototypeGameMode* GameMode = GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	if (!GameMode)
	{
		return;
	}

	switch (InteractionType)
	{
	case EManyNamesInteractionActionType::FirstMiracle:
		GameMode->TriggerFirstMiracle();
		break;
	case EManyNamesInteractionActionType::QuestDialogue:
		GameMode->OpenQuestDialogue(QuestId);
		break;
	case EManyNamesInteractionActionType::RegionTravel:
		GameMode->StartRegionTravel(TargetRegionId);
		break;
	default:
		break;
	}

	if (bSingleUse)
	{
		bConsumed = true;
	}
}

void AManyNamesInteractableActor::SetNpcVisualProfile(const FManyNamesNpcVisualProfile& InProfile)
{
	NpcVisualProfile = InProfile;
	ApplyNpcVisualProfile();
}

void AManyNamesInteractableActor::ApplyNpcVisualProfile()
{
	if (!MeshComponent || !SkeletalMeshComponent)
	{
		return;
	}

	if (!NpcVisualProfile.CameraAnchorTag.IsNone())
	{
		Tags.AddUnique(NpcVisualProfile.CameraAnchorTag);
	}
	if (!NpcVisualProfile.CharacterId.IsNone())
	{
		Tags.AddUnique(FName(*FString::Printf(TEXT("Character.%s"), *NpcVisualProfile.CharacterId.ToString())));
	}
	if (!NpcVisualProfile.StanceId.IsNone())
	{
		Tags.AddUnique(FName(*FString::Printf(TEXT("Stance.%s"), *NpcVisualProfile.StanceId.ToString())));
	}

	if (!NpcVisualProfile.FallbackStaticMesh.IsNull())
	{
		if (UStaticMesh* StaticMesh = NpcVisualProfile.FallbackStaticMesh.LoadSynchronous())
		{
			MeshComponent->SetStaticMesh(StaticMesh);
		}
	}

	if (!NpcVisualProfile.SkeletalMesh.IsNull())
	{
		if (USkeletalMesh* SkeletalMesh = NpcVisualProfile.SkeletalMesh.LoadSynchronous())
		{
			SkeletalMeshComponent->SetSkeletalMeshAsset(SkeletalMesh);
			SkeletalMeshComponent->SetRelativeLocation(NpcVisualProfile.RelativeLocation);
			SkeletalMeshComponent->SetRelativeScale3D(NpcVisualProfile.RelativeScale);
			SkeletalMeshComponent->SetVisibility(true);
			SkeletalMeshComponent->SetHiddenInGame(false);
			SkeletalMeshComponent->SetAllowClothActors(NpcVisualProfile.bEnableClothSimulation);
			SkeletalMeshComponent->bDisableClothSimulation = !NpcVisualProfile.bEnableClothSimulation;
			if (NpcVisualProfile.bEnableClothSimulation)
			{
				SkeletalMeshComponent->ResumeClothingSimulation();
			}
			else
			{
				SkeletalMeshComponent->SuspendClothingSimulation();
			}
			MeshComponent->SetVisibility(false);
			MeshComponent->SetHiddenInGame(true);

			if (!NpcVisualProfile.AnimationBlueprint.IsNull())
			{
				if (UClass* AnimClass = NpcVisualProfile.AnimationBlueprint.LoadSynchronous())
				{
					SkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
					SkeletalMeshComponent->SetAnimInstanceClass(AnimClass);
				}
			}
			else if (!NpcVisualProfile.IdleAnimation.IsNull())
			{
				if (UAnimationAsset* IdleAnimation = NpcVisualProfile.IdleAnimation.LoadSynchronous())
				{
					SkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
					SkeletalMeshComponent->SetAnimation(IdleAnimation);
					const float PlayOffset = FMath::Fmod(static_cast<float>(GetUniqueID()) * 0.13f, FMath::Max(0.1f, IdleAnimation->GetPlayLength()));
					SkeletalMeshComponent->SetPosition(PlayOffset, false);
					SkeletalMeshComponent->SetPlayRate(0.94f + (static_cast<float>(GetUniqueID() % 5) * 0.02f));
					SkeletalMeshComponent->Play(true);
				}
			}

			SnapInteractableActorToGround(this, 2400.0f);
			return;
		}
	}

	SkeletalMeshComponent->SetVisibility(false);
	SkeletalMeshComponent->SetHiddenInGame(true);
	MeshComponent->SetVisibility(true);
	MeshComponent->SetHiddenInGame(false);
	SnapInteractableActorToGround(this, 2400.0f);
}
