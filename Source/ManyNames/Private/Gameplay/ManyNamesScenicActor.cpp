#include "Gameplay/ManyNamesScenicActor.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimationAsset.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NavigationSystem.h"

namespace
{
void SnapScenicActorToGround(AActor* Actor, float MaxTraceDistance)
{
	if (!Actor || !Actor->GetWorld())
	{
		return;
	}

	const FVector Start = Actor->GetActorLocation() + FVector(0.0f, 0.0f, 240.0f);
	const FVector End = Start - FVector(0.0f, 0.0f, MaxTraceDistance);
	FHitResult HitResult;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ManyNamesNpcGrounding), false, Actor);
	if (Actor->GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params) && HitResult.bBlockingHit)
	{
		const FVector GroundedLocation(HitResult.ImpactPoint.X, HitResult.ImpactPoint.Y, HitResult.ImpactPoint.Z + 2.0f);
		Actor->SetActorLocation(GroundedLocation, false, nullptr, ETeleportType::TeleportPhysics);
		const FRotator CurrentRotation = Actor->GetActorRotation();
		const FRotator GroundRotation = HitResult.ImpactNormal.Rotation();
		Actor->SetActorRotation(FRotator(FMath::Clamp(GroundRotation.Pitch - 90.0f, -8.0f, 8.0f), CurrentRotation.Yaw, FMath::Clamp(GroundRotation.Roll, -8.0f, 8.0f)));
	}
}
}

AManyNamesScenicActor::AManyNamesScenicActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetupAttachment(SceneRoot);
	StaticMeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
	StaticMeshComponent->SetMobility(EComponentMobility::Movable);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMeshComponent->SetupAttachment(SceneRoot);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
	SkeletalMeshComponent->SetVisibility(false);
	SkeletalMeshComponent->SetHiddenInGame(false);
}

void AManyNamesScenicActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyNpcVisualProfile();
}

void AManyNamesScenicActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyNpcVisualProfile();
	PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (NpcVisualProfile.CrowdBehaviorTier == EManyNamesCrowdBehaviorTier::HubAmbient)
	{
		FindNewRoamDestination();
	}
}

void AManyNamesScenicActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAmbientBehavior(DeltaSeconds);
}

void AManyNamesScenicActor::SetNpcVisualProfile(const FManyNamesNpcVisualProfile& InProfile)
{
	NpcVisualProfile = InProfile;
	ApplyNpcVisualProfile();
}

void AManyNamesScenicActor::ApplyNpcVisualProfile()
{
	if (!StaticMeshComponent || !SkeletalMeshComponent)
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
			StaticMeshComponent->SetStaticMesh(StaticMesh);
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
			StaticMeshComponent->SetVisibility(false);
			StaticMeshComponent->SetHiddenInGame(true);

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
					SkeletalMeshComponent->SetPlayRate(0.92f + (static_cast<float>(GetUniqueID() % 7) * 0.02f));
					SkeletalMeshComponent->Play(true);
				}
			}

			SnapScenicActorToGround(this, 2400.0f);
			return;
		}
	}

	SkeletalMeshComponent->SetVisibility(false);
	SkeletalMeshComponent->SetHiddenInGame(true);
	StaticMeshComponent->SetVisibility(true);
	StaticMeshComponent->SetHiddenInGame(false);
	SnapScenicActorToGround(this, 2400.0f);
}

void AManyNamesScenicActor::UpdateAmbientBehavior(float DeltaSeconds)
{
	if (!GetWorld() || !SkeletalMeshComponent || !SkeletalMeshComponent->IsVisible())
	{
		return;
	}

	const bool bCanFacePlayer = NpcVisualProfile.bLockFacingToPlayer || NpcVisualProfile.CrowdBehaviorTier == EManyNamesCrowdBehaviorTier::StoryFacing;
	const bool bIsAmbientMover = NpcVisualProfile.CrowdBehaviorTier == EManyNamesCrowdBehaviorTier::HubAmbient;
	bool bPlayerIsClose = false;

	if (PlayerCharacter && bCanFacePlayer)
	{
		const float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation());
		bPlayerIsClose = DistanceToPlayer <= LookAtDistance;
		if (bPlayerIsClose)
		{
			FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), PlayerCharacter->GetActorLocation());
			TargetRotation.Pitch = 0.0f;
			TargetRotation.Roll = 0.0f;
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, 3.0f));
			return;
		}
	}

	if (!bIsAmbientMover || !bHasRoamDestination)
	{
		return;
	}

	FVector CurrentLocation = GetActorLocation();
	FVector Destination = CurrentRoamDestination;
	Destination.Z = CurrentLocation.Z;
	const FVector ToDestination = Destination - CurrentLocation;
	if (ToDestination.SizeSquared2D() <= FMath::Square(60.0f))
	{
		bHasRoamDestination = false;
		FindNewRoamDestination();
		return;
	}

	const FVector MoveStep = ToDestination.GetSafeNormal2D() * AmbientMoveSpeed * DeltaSeconds;
	SetActorLocation(CurrentLocation + MoveStep, true);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), ToDestination.Rotation(), DeltaSeconds, 2.2f));

	GroundSnapAccumulator += DeltaSeconds;
	if (GroundSnapAccumulator >= 0.45f)
	{
		GroundSnapAccumulator = 0.0f;
		SnapScenicActorToGround(this, 1200.0f);
	}
}

void AManyNamesScenicActor::FindNewRoamDestination()
{
	if (!GetWorld() || NpcVisualProfile.CrowdBehaviorTier != EManyNamesCrowdBehaviorTier::HubAmbient)
	{
		return;
	}

	if (PlayerCharacter && FVector::Dist(GetActorLocation(), PlayerCharacter->GetActorLocation()) <= LookAtDistance)
	{
		bHasRoamDestination = false;
		return;
	}

	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation RandomLocation;
		if (NavSystem->GetRandomReachablePointInRadius(GetActorLocation(), AmbientRoamRadius, RandomLocation))
		{
			CurrentRoamDestination = RandomLocation.Location;
			bHasRoamDestination = true;
			return;
		}
	}

	const FVector FallbackOffset(FMath::FRandRange(-AmbientRoamRadius, AmbientRoamRadius), FMath::FRandRange(-AmbientRoamRadius, AmbientRoamRadius), 0.0f);
	CurrentRoamDestination = GetActorLocation() + FallbackOffset;
	bHasRoamDestination = true;
}
