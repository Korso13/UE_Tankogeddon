// Fill out your copyright notice in the Description page of Project Settings.

#include "Tank.h"
#include "Canon.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Engine.h"
#include "GameStructs.h"

DECLARE_LOG_CATEGORY_EXTERN(TankLog, All, All);
DEFINE_LOG_CATEGORY(TankLog);

// Sets default values
ATank::ATank()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("TankCollision"));
	RootComponent = Collision;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tank Body"));
	BodyMesh->SetupAttachment(Collision);

	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tank Turret"));
	TurretMesh->SetupAttachment(BodyMesh);

	CanonMountingPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("Canon Mounting Point"));
	CanonMountingPoint->AttachToComponent(TurretMesh, FAttachmentTransformRules::KeepRelativeTransform);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArm->SetupAttachment(BodyMesh);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bInheritYaw = false;
	SpringArm->TargetArmLength = 1000;
	SpringArm->bDoCollisionTest = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>("Health Component");
	HealthComponent->OnDeath.AddUObject(this, &ATank::OnDeath);
	HealthComponent->OnDamage.AddUObject(this, &ATank::OnDamage);

};

// Called when the game starts or when spawned
void ATank::BeginPlay()
{
	Super::BeginPlay();

	TankController = Cast<ATankPlayerController>(GetController());
	CurrentCanonClass = FirstCannon;
	FirstCannonUsed = true;
	SecondCannonUsed = false;
	SetupCanon();
	CannonOne = Cannon;
}

// Called every frame
void ATank::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//forward movement
	CurrentForwardAxisImpulse = FMath::Lerp(CurrentForwardAxisImpulse, ForwardAxisMoveValue, ForwardMovementLerpKey); //interpolating current speed by target speed
	FVector MovePosition = GetActorLocation() + GetActorForwardVector() * MoveSpeed * CurrentForwardAxisImpulse * DeltaTime; //using interpolated forward speed
	//UE_LOG(TankLog, Warning, TEXT("Forward Impulse: %f, Forward Value: %f"), CurrentForwardAxisImpulse, ForwardAxisMoveValue); //debug!!
	SetActorLocation(MovePosition, true);
	//strafing
	CurrentStrafeAxisImpulse = FMath::Lerp(CurrentStrafeAxisImpulse, StrafeAxisMoveValue, StrafeLerpKey);
	FVector StrafePosition = GetActorLocation() + GetActorRightVector() * StrafeSpeed * CurrentStrafeAxisImpulse * DeltaTime;
	SetActorLocation(StrafePosition, true);
	//right axis rotation
	FRotator Rotation = GetActorRotation();
	CurrentRotationImpulse = FMath::Lerp(CurrentRotationImpulse, RightAxisRotationValue, RotationLerpKey);
	Rotation.Yaw = Rotation.Yaw + RotationSpeed * DeltaTime * CurrentRotationImpulse;
	//UE_LOG(LogTemp, Warning, TEXT("Impulse: %f, RotationValue: %f"), CurrentRotationImpulse, RightAxisRotationValue); //debug!!
	SetActorRotation(Rotation);
	//turret rotation
	FVector MousePosition = TankController->GetMousePos(); //getting Mouse position for turret rotation
	FRotator TargetTurretRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), MousePosition);
	FRotator CurrentTurretRotation = TurretMesh->GetComponentRotation();
	TargetTurretRotation.Pitch = CurrentTurretRotation.Pitch;
	TargetTurretRotation.Roll = CurrentTurretRotation.Roll;
	TurretMesh->SetWorldRotation(FMath::Lerp(CurrentTurretRotation, TargetTurretRotation, TurretRotationLerpKey));

	//AmmoHUD
	GEngine->AddOnScreenDebugMessage(20, 0.5f, FColor::Cyan, FString::Printf(TEXT("Ammo: %d"), Cannon->GetCurrAmmo()));

	//ScoreHUD
	GEngine->AddOnScreenDebugMessage(21, 0.5f, FColor::Purple, FString::Printf(TEXT("Your Score: %d"), TotalScore));

}

// Called to bind functionality to input
//void ATank::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	Super::SetupPlayerInputComponent(PlayerInputComponent);
//
//}

void ATank::MoveForward(const float ForwardAxisImpulse)
{
	ForwardAxisMoveValue = ForwardAxisImpulse;
}

void ATank::StrafeRight(float StrafeAxisImpulse)
{
	StrafeAxisMoveValue = StrafeAxisImpulse;
}

void ATank::RotateRight(float RightRotationImpulse)
{
	RightAxisRotationValue = RightRotationImpulse;
}

void ATank::SetupCanon()
{
	if (Cannon)
	{

		for (const auto child : Cannon->Children)
		{
			while (child->IsActorBeingDestroyed())
			{
				//
			};
		}
		Cannon->Destroy();

	}
	FActorSpawnParameters spawnParams;
	spawnParams.Instigator = this;
	spawnParams.Owner = this;
	if (CurrentCanonClass)
	{
		if (FirstCannonUsed)
		{
			if (CannonOne)
			{
				Cannon = GetWorld()->SpawnActor<ACanon>(CurrentCanonClass, spawnParams);
				Cannon->SetCurrAmmo(CannonOne->GetCurrAmmo());
			}
			else
			{
				Cannon = GetWorld()->SpawnActor<ACanon>(CurrentCanonClass, spawnParams);
				Cannon->ResetAmmo();
				CannonOne = Cannon;
				CannonOne->ResetAmmo();
			}
		}
		else
		{
			if (CannonTwo)
			{
				Cannon = GetWorld()->SpawnActor<ACanon>(CurrentCanonClass, spawnParams);
				Cannon->SetCurrAmmo(CannonTwo->GetCurrAmmo());
			}
			else
			{
				Cannon = GetWorld()->SpawnActor<ACanon>(CurrentCanonClass, spawnParams);
				Cannon->ResetAmmo();
				CannonTwo = Cannon;
				CannonTwo->ResetAmmo();
			}
		}
		Cannon->AttachToComponent(CanonMountingPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

}

void ATank::ChangeCanon(TSubclassOf<ACanon> _CanonType)
{
	if (!SecondCannon)
	{
		SecondCannon = _CanonType;
		CurrentCanonClass = SecondCannon;
		SecondCannonUsed = true;
		FirstCannonUsed = false;
		SetupCanon();
		return;
	}

	if (FirstCannonUsed)
	{
		FirstCannon = _CanonType;
		CurrentCanonClass = FirstCannon;
		SecondCannonUsed = false;
		FirstCannonUsed = true;
		CannonOne->Destroy();
	}
	else
	{
		SecondCannon = _CanonType;
		CurrentCanonClass = SecondCannon;
		SecondCannonUsed = true;
		FirstCannonUsed = false;
		CannonTwo->Destroy();
	}
	SetupCanon();
}


void ATank::PrimaryFire() const
{
	if (!Cannon)
	{
		return;
	}

	Cannon->Fire(FireType::Primary);
}

void ATank::SecondaryFire() const
{
	if (!Cannon)
	{
		return;
	}
	Cannon->Fire(FireType::Secondary);
}

void ATank::SwitchWeapon()
{
	if (FirstCannonUsed)
	{
		if (SecondCannon)
		{
			CurrentCanonClass = SecondCannon;
			FirstCannonUsed = false;
			SecondCannonUsed = true;
			CannonOne = Cannon;
			SetupCanon();
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Cyan, "Switched to second weapon", true);
		}
		else
		{
			return;
		}
	}
	else
	{
		if (!SecondCannonUsed)
		{
			return;
		}
		else if (FirstCannon)
		{
			CurrentCanonClass = FirstCannon;
			FirstCannonUsed = true;
			SecondCannonUsed = false;
			CannonTwo = Cannon;
			SetupCanon();
			GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Cyan, "Switched to first weapon", true);
		}
		else
		{
			return;
		}
	}
}

void ATank::OnDamage(FDamageInfo Damage)
{
	GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Blue, FString::Printf(TEXT("Tank took damage from %s, HP: %f"), ToCStr(Damage.Instigator->GetName()), HealthComponent->CurrentHP));
}
