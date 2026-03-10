#pragma once

#include "CoreMinimal.h"
#include "Core/ManyNamesTypes.h"
#include "GameFramework/Actor.h"
#include "Interaction/ManyNamesInteractable.h"
#include "ManyNamesInteractableActor.generated.h"

UENUM(BlueprintType)
enum class EManyNamesInteractionActionType : uint8
{
	FirstMiracle,
	QuestDialogue,
	RegionTravel
};

UCLASS(Blueprintable)
class MANYNAMES_API AManyNamesInteractableActor : public AActor, public IManyNamesInteractable
{
	GENERATED_BODY()

public:
	AManyNamesInteractableActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	virtual FText GetInteractionLabel_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* InteractingPawn) const override;
	virtual void Interact_Implementation(APawn* InteractingPawn) override;

	class UStaticMeshComponent* GetStaticMeshComponent() const { return MeshComponent; }
	class USkeletalMeshComponent* GetSkeletalMeshComponent() const { return SkeletalMeshComponent; }
	void SetInteractionLabel(const FText& InLabel) { InteractionLabel = InLabel; }
	void SetInteractionType(EManyNamesInteractionActionType InType) { InteractionType = InType; }
	void SetQuestId(FName InQuestId) { QuestId = InQuestId; }
	void SetTargetRegionId(EManyNamesRegionId InRegionId) { TargetRegionId = InRegionId; }
	void SetRequiredOutputs(const TArray<FName>& InRequiredOutputs) { RequiredOutputs = InRequiredOutputs; }
	void SetSingleUse(bool bInSingleUse) { bSingleUse = bInSingleUse; }

	UFUNCTION(BlueprintCallable, Category="ManyNames|Interaction")
	void SetNpcVisualProfile(const FManyNamesNpcVisualProfile& InProfile);

	UFUNCTION(BlueprintPure, Category="ManyNames|Interaction")
	const FManyNamesNpcVisualProfile& GetNpcVisualProfile() const { return NpcVisualProfile; }

protected:
	void ApplyNpcVisualProfile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ManyNames|Interaction")
	TObjectPtr<class UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ManyNames|Interaction")
	TObjectPtr<class USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Interaction")
	FText InteractionLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Interaction")
	EManyNamesInteractionActionType InteractionType = EManyNamesInteractionActionType::QuestDialogue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Interaction")
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Interaction")
	EManyNamesRegionId TargetRegionId = EManyNamesRegionId::Egypt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Interaction")
	TArray<FName> RequiredOutputs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Interaction")
	bool bSingleUse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Interaction")
	FManyNamesNpcVisualProfile NpcVisualProfile;

private:
	bool bConsumed = false;
};
