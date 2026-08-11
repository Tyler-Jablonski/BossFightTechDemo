#include "UE58Testing/Public/MainCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "UE58Testing/Boss.h"

AMainCharacter::AMainCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(RootComponent);
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	
	DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DetectionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	DetectionSphere->SetGenerateOverlapEvents(true);
	
}

void AMainCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Register the Input Mapping Context with the Subsystem
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
	if (DetectionSphere)
	{
		DetectionSphere->SetSphereRadius(DetectionRadius);
		DetectionSphere->UpdateOverlaps();
	}
}

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind actions inside SetupPlayerInputComponent
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMainCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMainCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMainCharacter::JumpStarted);
		EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &AMainCharacter::LockOn);
	}
}

void AMainCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AMainCharacter::Look(const FInputActionValue& Value)
{
	if (bIsLockedOn)
	{
		return;
	}

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AMainCharacter::JumpStarted(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		Jump();
	}
}

void AMainCharacter::JumpEnded(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		StopJumping();
	}
}

void AMainCharacter::LockOn(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("LockOn"));
	if (bIsLockedOn)
	{
		bIsLockedOn = false;
		CurrentTarget = nullptr;
	}
	else
	{
		CurrentTarget = GetCurrentTarget();
		if (CurrentTarget != nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Locking on to target: %s"), *CurrentTarget->GetName());
			bIsLockedOn = true;
		}
	}
}

void AMainCharacter::UpdateLockOnRotation(float DeltaTime)
{
	if (!bIsLockedOn || !CurrentTarget || !Controller)
	{
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("UpdateLockOnRotation"));
	float Dist = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
	if (Dist > DetectionRadius * 1.2)
	{
		bIsLockedOn = false;
		CurrentTarget = nullptr;
		return;
	}
	FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(),
	                                                                 CurrentTarget->GetActorLocation());
	FRotator SmoothRotation = FMath::RInterpTo(Controller->GetControlRotation(), TargetRotation, DeltaTime, LockOnRotationSpeed);
	Controller->SetControlRotation(SmoothRotation);
	
}

AActor* AMainCharacter::GetCurrentTarget()
{
	TArray<AActor*> Enemies;

	if (DetectionSphere)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), DetectionRadius, 16, FColor::Green, false, 2.0f);
		DetectionSphere->GetOverlappingActors(Enemies, AActor::StaticClass());
		AActor* ClosestTarget = nullptr;
		float MinDistance = FLT_MAX;
		
		for (AActor* Actor : Enemies)
		{
			if (Actor)
			{
				float Dist = FVector::Dist(GetActorLocation(), Actor->GetActorLocation());
				if (Dist < MinDistance)
				{
					MinDistance = Dist;
					ClosestTarget = Actor;
				}
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("Closest target: %s"), ClosestTarget ? *ClosestTarget->GetName() : TEXT("None"));
		return ClosestTarget;
	}
	return nullptr;
}


void AMainCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bIsLockedOn)
	{
		UpdateLockOnRotation(DeltaTime);
	}
}
