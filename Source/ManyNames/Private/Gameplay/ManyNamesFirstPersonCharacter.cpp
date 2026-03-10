#include "Gameplay/ManyNamesFirstPersonCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/ManyNamesEnvironmentController.h"
#include "Gameplay/ManyNamesPrototypeGameMode.h"
#include "Interaction/ManyNamesInteractable.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Systems/ManyNamesGameInstance.h"

AManyNamesFirstPersonCharacter::AManyNamesFirstPersonCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	CameraComponent->SetupAttachment(GetCapsuleComponent());
	CameraComponent->SetRelativeLocation(FVector(-10.0f, 0.0f, 64.0f));
	CameraComponent->bUsePawnControlRotation = true;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = false;
	GetCharacterMovement()->PerchRadiusThreshold = 12.0f;
	GetCharacterMovement()->MaxStepHeight = 50.0f;
	GetCharacterMovement()->SetWalkableFloorAngle(52.0f);
	GetCharacterMovement()->bMaintainHorizontalGroundVelocity = true;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AManyNamesFirstPersonCharacter::BeginPlay()
{
	Super::BeginPlay();
	ShowMessage(TEXT("Many Names prototype loaded. Use E to interact, J for journal, and 1-4 for dialogue or travel."), FColor::Green);
}

void AManyNamesFirstPersonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bSpawnRecoveryActive)
	{
		SpawnRecoveryElapsed += DeltaSeconds;
		TryRecoverInvalidSpawn();
		if (SpawnRecoveryElapsed >= SpawnRecoveryWindowSeconds)
		{
			bSpawnRecoveryActive = false;
		}
	}

	float TraversalMultiplier = 1.0f;
	if (const AManyNamesEnvironmentController* EnvironmentController = GetWorld() ? Cast<AManyNamesEnvironmentController>(UGameplayStatics::GetActorOfClass(GetWorld(), AManyNamesEnvironmentController::StaticClass())) : nullptr)
	{
		TraversalMultiplier = EnvironmentController->GetTraversalSpeedMultiplier();
	}
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * TraversalMultiplier;
	if (HasGroundSupport(GroundProbeDistance))
	{
		GetCharacterMovement()->bUseSeparateBrakingFriction = false;
	}

	UpdateInteractionPrompt();
}

void AManyNamesFirstPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AManyNamesFirstPersonCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AManyNamesFirstPersonCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AManyNamesFirstPersonCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AManyNamesFirstPersonCharacter::LookUp);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AManyNamesFirstPersonCharacter::AttemptInteract);
	PlayerInputComponent->BindAction(TEXT("Journal"), IE_Pressed, this, &AManyNamesFirstPersonCharacter::OpenJournal);
	PlayerInputComponent->BindAction(TEXT("FocusPower"), IE_Pressed, this, &AManyNamesFirstPersonCharacter::UseFocusShift);
	PlayerInputComponent->BindAction(TEXT("InsightPulse"), IE_Pressed, this, &AManyNamesFirstPersonCharacter::UseInsightPulse);
	PlayerInputComponent->BindAction(TEXT("Option1"), IE_Pressed, this, &AManyNamesFirstPersonCharacter::SelectOption1);
	PlayerInputComponent->BindAction(TEXT("Option2"), IE_Pressed, this, &AManyNamesFirstPersonCharacter::SelectOption2);
	PlayerInputComponent->BindAction(TEXT("Option3"), IE_Pressed, this, &AManyNamesFirstPersonCharacter::SelectOption3);
	PlayerInputComponent->BindAction(TEXT("Option4"), IE_Pressed, this, &AManyNamesFirstPersonCharacter::SelectOption4);
}

void AManyNamesFirstPersonCharacter::MoveForward(float Value)
{
	if (const AManyNamesPrototypeGameMode* GameMode = GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (GameMode->IsDialogueMovementLocked())
		{
			return;
		}
	}
	AddMovementInput(GetActorForwardVector(), Value);
}

void AManyNamesFirstPersonCharacter::MoveRight(float Value)
{
	if (const AManyNamesPrototypeGameMode* GameMode = GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		if (GameMode->IsDialogueMovementLocked())
		{
			return;
		}
	}
	AddMovementInput(GetActorRightVector(), Value);
}

void AManyNamesFirstPersonCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void AManyNamesFirstPersonCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void AManyNamesFirstPersonCharacter::AttemptInteract()
{
	if (AActor* HitActor = TraceForInteractable())
	{
		if (IManyNamesInteractable::Execute_CanInteract(HitActor, this))
		{
			IManyNamesInteractable::Execute_Interact(HitActor, this);
			return;
		}
	}

	ShowMessage(TEXT("No valid interaction in range."), FColor::Red);
}

void AManyNamesFirstPersonCharacter::OpenJournal()
{
	if (const AManyNamesPrototypeGameMode* GameMode = GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		ShowMessage(GameMode->GetJournalSummary(), FColor::Cyan);
	}
}

void AManyNamesFirstPersonCharacter::UseFocusShift()
{
	if (CanUsePower(EManyNamesPowerId::FocusShift))
	{
		ShowMessage(TEXT("Focus Shift activated. Time narrows around the target path."), FColor::Yellow);
	}
	else
	{
		ShowMessage(TEXT("Focus Shift is not unlocked yet."), FColor::Red);
	}
}

void AManyNamesFirstPersonCharacter::UseInsightPulse()
{
	if (CanUsePower(EManyNamesPowerId::InsightPulse))
	{
		ShowMessage(TEXT("Insight Pulse reveals the immediate narrative route."), FColor::Yellow);
	}
	else
	{
		ShowMessage(TEXT("Insight Pulse is not unlocked yet."), FColor::Red);
	}
}

void AManyNamesFirstPersonCharacter::SelectOption1()
{
	if (AManyNamesPrototypeGameMode* GameMode = GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		GameMode->HandleMenuSelection(0);
	}
}

void AManyNamesFirstPersonCharacter::SelectOption2()
{
	if (AManyNamesPrototypeGameMode* GameMode = GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		GameMode->HandleMenuSelection(1);
	}
}

void AManyNamesFirstPersonCharacter::SelectOption3()
{
	if (AManyNamesPrototypeGameMode* GameMode = GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		GameMode->HandleMenuSelection(2);
	}
}

void AManyNamesFirstPersonCharacter::SelectOption4()
{
	if (AManyNamesPrototypeGameMode* GameMode = GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		GameMode->HandleMenuSelection(3);
	}
}

void AManyNamesFirstPersonCharacter::UpdateInteractionPrompt()
{
	if (!GEngine)
	{
		return;
	}

	if (AActor* HitActor = TraceForInteractable())
	{
		const FText Label = IManyNamesInteractable::Execute_GetInteractionLabel(HitActor);
		GEngine->AddOnScreenDebugMessage(1001, 0.05f, FColor::White, FString::Printf(TEXT("[E] %s"), *Label.ToString()));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(1001, 0.05f, FColor::Transparent, TEXT(""));
	}

	if (const AManyNamesPrototypeGameMode* GameMode = GetWorld() ? Cast<AManyNamesPrototypeGameMode>(GetWorld()->GetAuthGameMode()) : nullptr)
	{
		const FText MenuPrompt = GameMode->GetMenuPromptText();
		GEngine->AddOnScreenDebugMessage(1002, 0.05f, FColor::Silver, MenuPrompt.ToString());
	}
}

AActor* AManyNamesFirstPersonCharacter::TraceForInteractable() const
{
	if (!CameraComponent || !GetWorld())
	{
		return nullptr;
	}

	const FVector Start = CameraComponent->GetComponentLocation();
	const FVector End = Start + (CameraComponent->GetForwardVector() * InteractionRange);
	FHitResult HitResult;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ManyNamesInteractTrace), false, this);
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		if (HitResult.GetActor() && HitResult.GetActor()->GetClass()->ImplementsInterface(UManyNamesInteractable::StaticClass()))
		{
			return HitResult.GetActor();
		}
	}

	return nullptr;
}

bool AManyNamesFirstPersonCharacter::CanUsePower(EManyNamesPowerId PowerId) const
{
	if (const UManyNamesGameInstance* GameInstance = GetGameInstance<UManyNamesGameInstance>())
	{
		return GameInstance->GetWorldState().UnlockedPowers.Contains(PowerId);
	}

	return false;
}

bool AManyNamesFirstPersonCharacter::HasGroundSupport(float TraceDistance) const
{
	if (!GetWorld())
	{
		return false;
	}

	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
	const FVector End = Start - FVector(0.0f, 0.0f, TraceDistance);
	FHitResult HitResult;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ManyNamesSpawnGroundTrace), false, this);
	return GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params) && HitResult.bBlockingHit;
}

void AManyNamesFirstPersonCharacter::TryRecoverInvalidSpawn()
{
	if (HasGroundSupport(SpawnRecoveryTraceDistance) && GetActorLocation().Z > SpawnFailureZ)
	{
		return;
	}

	AActor* PlayerStartActor = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass());
	if (!PlayerStartActor)
	{
		return;
	}

	const FVector RecoveryLocation = PlayerStartActor->GetActorLocation() + FVector(0.0f, 0.0f, SpawnRecoveryLift);
	SetActorLocation(RecoveryLocation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(PlayerStartActor->GetActorRotation());
	GetCharacterMovement()->StopMovementImmediately();
	ShowMessage(TEXT("Recovered to a valid spawn point."), FColor::Yellow);
	bSpawnRecoveryActive = false;
}

void AManyNamesFirstPersonCharacter::ShowMessage(const FString& Message, FColor Color) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, Color, Message);
	}
}
