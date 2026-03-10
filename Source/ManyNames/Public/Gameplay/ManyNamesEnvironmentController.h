#pragma once

#include "CoreMinimal.h"
#include "Core/ManyNamesTypes.h"
#include "GameFramework/Actor.h"
#include "ManyNamesEnvironmentController.generated.h"

UCLASS(Blueprintable)
class MANYNAMES_API AManyNamesEnvironmentController : public AActor
{
	GENERATED_BODY()

public:
	AManyNamesEnvironmentController();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="ManyNames|Environment")
	void SetEnvironmentProfile(const FManyNamesEnvironmentProfile& InProfile);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Environment")
	bool ApplyWeatherState(FName StateId);

	UFUNCTION(BlueprintCallable, Category="ManyNames|Environment")
	void ApplyBaselineState();

	UFUNCTION(BlueprintPure, Category="ManyNames|Environment")
	float GetTraversalSpeedMultiplier() const { return CurrentTraversalSpeedMultiplier; }

	UFUNCTION(BlueprintPure, Category="ManyNames|Environment")
	const FManyNamesEnvironmentProfile& GetEnvironmentProfile() const { return EnvironmentProfile; }

private:
	void ApplyWeatherData(const FManyNamesWeatherState& State);
	class ADirectionalLight* FindDirectionalLight() const;
	class ASkyLight* FindSkyLight() const;
	class AExponentialHeightFog* FindHeightFog() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ManyNames|Environment", meta=(AllowPrivateAccess="true"))
	FManyNamesEnvironmentProfile EnvironmentProfile;

	UPROPERTY(Transient)
	float CurrentTraversalSpeedMultiplier = 1.0f;
};
