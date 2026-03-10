#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ManyNamesInteractable.generated.h"

UINTERFACE(BlueprintType)
class UManyNamesInteractable : public UInterface
{
	GENERATED_BODY()
};

class MANYNAMES_API IManyNamesInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FText GetInteractionLabel() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	bool CanInteract(APawn* InteractingPawn) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void Interact(APawn* InteractingPawn);
};
