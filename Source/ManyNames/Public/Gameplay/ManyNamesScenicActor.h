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

	class UStaticMeshComponent* GetStaticMeshComponent() const { return StaticMeshComponent; }
	class USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }

	UFUNCTION(BlueprintCallable, Category="ManyNames|Scene")
	void SetNpcVisualProfile(const FManyNamesNpcVisualProfile& InProfile);

	UFUNCTION(BlueprintPure, Category="ManyNames|Scene")
	const FManyNamesNpcVisualProfile& GetNpcVisualProfile() const { return NpcVisualProfile; }

protected:
	void ApplyNpcVisualProfile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ManyNames|Scene")
	TObjectPtr<class USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ManyNames|Scene")
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ManyNames|Scene")
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Scene")
	FManyNamesNpcVisualProfile NpcVisualProfile;
};
