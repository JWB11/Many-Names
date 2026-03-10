#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Core/ManyNamesTypes.h"
#include "ManyNamesGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FManyNamesWorldStateChanged, const FManyNamesWorldState&, WorldState);

UCLASS()
class MANYNAMES_API UManyNamesGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category="ManyNames|Save")
	bool SaveWorldState();

	UFUNCTION(BlueprintCallable, Category="ManyNames|Save")
	bool LoadWorldState();

	UFUNCTION(BlueprintCallable, Category="ManyNames|State")
	const FManyNamesWorldState& GetWorldState() const;

	UFUNCTION(BlueprintCallable, Category="ManyNames|State")
	void SetWorldState(const FManyNamesWorldState& InWorldState);

	UPROPERTY(BlueprintAssignable, Category="ManyNames|State")
	FManyNamesWorldStateChanged OnWorldStateChanged;

private:
	FManyNamesWorldState WorldState;

	void InitializeDefaultWorldState();
};
