#include "Gameplay/ManyNamesEnvironmentController.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

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
	ApplyRenderFeatureToggles();
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
	ApplyRenderFeatureToggles();

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

void AManyNamesEnvironmentController::ApplyRenderFeatureToggles()
{
	ActiveRenderPath = ResolveRenderPath();
	const bool bWindowsHighEnd = ActiveRenderPath == EManyNamesRenderPath::WindowsHighEnd;

	SetConsoleVariableInt(TEXT("r.HeterogeneousVolumes"), EnvironmentProfile.bAllowHeterogeneousVolumes ? 1 : 0);
	SetConsoleVariableInt(TEXT("r.Translucency.HeterogeneousVolumes"), EnvironmentProfile.bAllowHeterogeneousVolumes ? 1 : 0);
	SetConsoleVariableInt(TEXT("r.HeterogeneousVolumes.Shadows"), EnvironmentProfile.bAllowHeterogeneousVolumes ? 1 : 0);
	SetConsoleVariableInt(TEXT("r.MegaLights.EnableForProject"), bWindowsHighEnd && EnvironmentProfile.bAllowMegaLights ? 1 : 0);
	SetConsoleVariableInt(TEXT("r.MegaLights.HardwareRayTracing"), bWindowsHighEnd && EnvironmentProfile.bAllowMegaLights ? 1 : 0);
	SetConsoleVariableInt(TEXT("r.MegaLights.Volume"), bWindowsHighEnd && EnvironmentProfile.bAllowMegaLights ? 1 : 0);
	SetConsoleVariableInt(TEXT("r.VolumetricFog"), 1);
}

EManyNamesRenderPath AManyNamesEnvironmentController::ResolveRenderPath() const
{
	switch (EnvironmentProfile.PreferredRenderPath)
	{
	case EManyNamesRenderPath::WindowsHighEnd:
#if PLATFORM_WINDOWS
		return EManyNamesRenderPath::WindowsHighEnd;
#else
		return EManyNamesRenderPath::MacFallback;
#endif
	case EManyNamesRenderPath::MacFallback:
		return EManyNamesRenderPath::MacFallback;
	case EManyNamesRenderPath::Auto:
	default:
#if PLATFORM_WINDOWS
		return EManyNamesRenderPath::WindowsHighEnd;
#else
		return EManyNamesRenderPath::MacFallback;
#endif
	}
}

void AManyNamesEnvironmentController::SetConsoleVariableInt(const TCHAR* Name, int32 Value) const
{
	if (IConsoleVariable* ConsoleVariable = IConsoleManager::Get().FindConsoleVariable(Name))
	{
		ConsoleVariable->Set(Value, ECVF_SetByCode);
	}
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
