#pragma once

#include "CoreMinimal.h"
#include "Core/ManyNamesTypes.h"
#include "GameFramework/Actor.h"
#include "ManyNamesScenicActor.generated.h"

UCLASS(Blueprintable)
class MANYNAMES_API AManyNamesScenicActor : public AActor
{
	GENERATED_BODY()

public:
	AManyNamesScenicActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	class UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }
	class USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }

	UFUNCTION(BlueprintCallable, Category="ManyNames|Scene")
	void SetNpcVisualProfile(const FManyNamesNpcVisualProfile& InProfile);

	UFUNCTION(BlueprintPure, Category="ManyNames|Scene")
	const FManyNamesNpcVisualProfile& GetNpcVisualProfile() const { return NpcVisualProfile; }

protected:
	void ApplyNpcVisualProfile();
	void UpdateAmbientBehavior(float DeltaSeconds);
	void FindNewRoamDestination();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ManyNames|Scene")
	TObjectPtr<class USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ManyNames|Scene")
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ManyNames|Scene")
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Scene")
	FManyNamesNpcVisualProfile NpcVisualProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Scene")
	float AmbientRoamRadius = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Scene")
	float LookAtDistance = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Scene")
	float AmbientMoveSpeed = 90.0f;

private:
	FTimerHandle RoamTimerHandle;
	TObjectPtr<class ACharacter> PlayerCharacter;
	FVector CurrentRoamDestination = FVector::ZeroVector;
	float GroundSnapAccumulator = 0.0f;
	bool bHasRoamDestination = false;
};
