#include "Gameplay/ManyNamesEnvironmentController.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"

AManyNamesEnvironmentController::AManyNamesEnvironmentController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AManyNamesEnvironmentController::BeginPlay()
{
	Super::BeginPlay();
	ApplyBaselineState();
}

void AManyNamesEnvironmentController::SetEnvironmentProfile(const FManyNamesEnvironmentProfile& InProfile)
{
	EnvironmentProfile = InProfile;
	ApplyBaselineState();
}

bool AManyNamesEnvironmentController::ApplyWeatherState(FName StateId)
{
	if (StateId == EnvironmentProfile.BaselineState.StateId)
	{
		ApplyWeatherData(EnvironmentProfile.BaselineState);
		return true;
	}

	if (StateId == EnvironmentProfile.HeroState.StateId)
	{
		ApplyWeatherData(EnvironmentProfile.HeroState);
		return true;
	}

	return false;
}

void AManyNamesEnvironmentController::ApplyBaselineState()
{
	ApplyWeatherData(EnvironmentProfile.BaselineState);
}

void AManyNamesEnvironmentController::ApplyWeatherData(const FManyNamesWeatherState& State)
{
	if (ADirectionalLight* Sun = FindDirectionalLight())
	{
		Sun->SetActorRotation(FRotator(State.SunPitch, State.SunYaw, 0.0f));
		if (UDirectionalLightComponent* DirectionalLightComponent = Sun->GetComponent())
		{
			DirectionalLightComponent->SetIntensity(State.SunIntensity);
			DirectionalLightComponent->SetLightColor(State.KeyLightTint.ToFColor(true));
		}
	}

	if (ASkyLight* Sky = FindSkyLight())
	{
		if (USkyLightComponent* SkyLightComponent = Sky->GetLightComponent())
		{
			SkyLightComponent->SetIntensity(State.SkyIntensity);
			SkyLightComponent->RecaptureSky();
		}
	}

	if (AExponentialHeightFog* Fog = FindHeightFog())
	{
		if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
		{
			FogComponent->FogDensity = State.FogDensity;
			FogComponent->SetFogInscatteringColor(State.FogTint);
		}
	}

	CurrentTraversalSpeedMultiplier = State.bAffectsTraversal
		? FMath::Clamp(State.TraversalSpeedMultiplier, 0.5f, 1.0f)
		: 1.0f;
}

ADirectionalLight* AManyNamesEnvironmentController::FindDirectionalLight() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

ASkyLight* AManyNamesEnvironmentController::FindSkyLight() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

AExponentialHeightFog* AManyNamesEnvironmentController::FindHeightFog() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	for (TActorIterator<AExponentialHeightFog> It(GetWorld()); It; ++It)
	{
		return *It;
	}

	return nullptr;
}
