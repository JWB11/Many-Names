#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/ManyNamesTypes.h"
#include "ManyNamesFirstPersonCharacter.generated.h"

UCLASS(Blueprintable)
class MANYNAMES_API AManyNamesFirstPersonCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AManyNamesFirstPersonCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void AttemptInteract();
	void OpenJournal();
	void UseFocusShift();
	void UseInsightPulse();
	void SelectOption1();
	void SelectOption2();
	void SelectOption3();
	void SelectOption4();
	void UpdateInteractionPrompt();
	AActor* TraceForInteractable() const;
	bool CanUsePower(EManyNamesPowerId PowerId) const;
	bool HasGroundSupport(float TraceDistance) const;
	void TryRecoverInvalidSpawn();
	void ShowMessage(const FString& Message, FColor Color = FColor::White) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ManyNames|Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<class UCameraComponent> CameraComponent;

	UPROPERTY(EditDefaultsOnly, Category="ManyNames|Movement")
	float BaseWalkSpeed = 520.0f;

	UPROPERTY(EditDefaultsOnly, Category="ManyNames|Movement")
	float GroundProbeDistance = 140.0f;

	UPROPERTY(EditDefaultsOnly, Category="ManyNames|Interaction")
	float InteractionRange = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category="ManyNames|Spawn")
	float SpawnRecoveryWindowSeconds = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category="ManyNames|Spawn")
	float SpawnRecoveryTraceDistance = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category="ManyNames|Spawn")
	float SpawnFailureZ = -600.0f;

	UPROPERTY(EditDefaultsOnly, Category="ManyNames|Spawn")
	float SpawnRecoveryLift = 120.0f;

	float SpawnRecoveryElapsed = 0.0f;
	bool bSpawnRecoveryActive = true;
};
