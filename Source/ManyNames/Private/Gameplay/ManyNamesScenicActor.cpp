#include "Gameplay/ManyNamesScenicActor.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimationAsset.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

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
	PrimaryActorTick.bCanEverTick = false;

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
