// Fill out your copyright notice in the Description page of Project Settings.

#include "Canon.h"

#include "DrawDebugHelpers.h"
#include "DamageTaker.h"
#include "Engine/Engine.h"

// Sets default values
ACanon::ACanon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	//setting up meshes
	sceenecmp = CreateDefaultSubobject<USceneComponent>("Scene CMP");
	RootComponent = sceenecmp;
	CanonMesh = CreateDefaultSubobject<UStaticMeshComponent>("Cannon mesh");
	CanonMesh->SetupAttachment(RootComponent);
	ProjectileSpawnPoint = CreateDefaultSubobject<UArrowComponent>("Projectile spawn point");
	ProjectileSpawnPoint->SetupAttachment(CanonMesh);

	if (Type == CanonType::ProjectileCanon)
	{
		for (size_t i = 0; i < 20; i++)
		{
			Pool.AddDefaulted();
			Pool[i].IsFree = true;
		}
	}
}

// Called when the game starts or when spawned
void ACanon::BeginPlay()
{
	Super::BeginPlay();
	
	Reload();
}

void ACanon::Fire(FireType att_type)
{
	if (!IsReadyToFire())
	{
		return;
	}
	if (Ammo <= 0)
	{
		return;
	}
	//firing
	ReadyToFire = false;
	//primary or secondary attack type?
	switch (att_type)
	{
	case(FireType::Primary):
		{
		//checking cannon type
		if (Type == CanonType::ProjectileCanon && AmmoType)
		{
			GEngine->AddOnScreenDebugMessage(10, 1, FColor::Black, "Firing projectile");
			FActorSpawnParameters SpawnParams;
			SpawnParams.Instigator = GetInstigator();
			SpawnParams.Owner = this;

			for (size_t i = 0; i < 20; i++)
			{
				if(!Pool[i].IsFree)
				{
					if(!Pool[i].Projectile.IsValid())
					{
						Pool[i].IsFree = true;
					}
				}

				if (Pool[i].IsFree)
				{
					Pool[i].Projectile = GetWorld()->SpawnActor<AProjectile>(AmmoType, ProjectileSpawnPoint->GetComponentTransform(), SpawnParams);
					Pool[i].IsFree = false;
					break;
				}
			}

		}
		else if(Type == CanonType::LaserCanon)
		{
			GEngine->AddOnScreenDebugMessage(10, 1, FColor::Black, "Firing laser");
			FHitResult HitResult;
			FCollisionQueryParams CollParams;
			CollParams.AddIgnoredActor(this);
			CollParams.AddIgnoredActor(GetInstigator());
			CollParams.bTraceComplex = true;
			CollParams.bReturnPhysicalMaterial = false;
			CollParams.TraceTag = FName(TEXT("Projectile"));
			FVector start = ProjectileSpawnPoint->GetComponentLocation();
			FVector end = ProjectileSpawnPoint->GetForwardVector() * LaserRange + start;

			//drawing tracer
			GetWorld()->LineTraceSingleByChannel(HitResult, start, end, ECollisionChannel::ECC_Visibility, CollParams);
			DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.1f, 0, 4);

			//Dealing laser damage
			if(HitResult.Actor.IsValid())
			{
				IDamageTaker* Damageable = Cast<IDamageTaker>(HitResult.Actor);
				if(Damageable)
				{
					FDamageInfo DamageData;
					DamageData.Instigator = GetInstigator();
					DamageData.DamageDealer = this;
					DamageData.DamageValue = LaserDamage;
					Damageable->TakeDamage(DamageData);
				}
				else
				{
					HitResult.Actor->Destroy();
				}
			}
		}
		Ammo -= AmmoPrimaryConsumption;
		break;
		}

	case(FireType::Secondary):
		{

			GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &ACanon::Refire, RefireSpeed, true);
			//ending RefireTimer
			FTimerDelegate ClearTimerDelegate;
			ClearTimerDelegate.BindLambda([&]() {GetWorld()->GetTimerManager().ClearTimer(RefireTimer); });
			GetWorld()->GetTimerManager().SetTimer(RefireCD, ClearTimerDelegate, 0.5f * 3 + 0.01f, false);

			Ammo -= AmmoSecondaryConsumption;
			break;
		}

	default:
		break;
	}
	//seting up reload timer:
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &ACanon::Reload, FireRate, false);
	
}

void ACanon::Refire()
{
	GEngine->AddOnScreenDebugMessage(10, 1, FColor::Black, "Firing alternate fire");
	if(CanonType::LaserCanon == Type)
	{
		GEngine->AddOnScreenDebugMessage(11, 1, FColor::Red, "Laser");
		FHitResult HitResult;
		FCollisionQueryParams CollParams;
		CollParams.AddIgnoredActor(this);
		CollParams.AddIgnoredActor(GetInstigator());
		CollParams.bTraceComplex = true;
		CollParams.bReturnPhysicalMaterial = false;
		CollParams.TraceTag = FName(TEXT("Projectile"));
		FVector start = ProjectileSpawnPoint->GetComponentLocation();
		FVector end = ProjectileSpawnPoint->GetForwardVector() * LaserRange + start;

		//drawing tracer
		GetWorld()->LineTraceSingleByChannel(HitResult, start, end, ECollisionChannel::ECC_Visibility, CollParams);
		DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 0.1f, 0, 4);

		//Dealing laser damage
		if (HitResult.Actor.IsValid())
		{
			IDamageTaker* Damageable = Cast<IDamageTaker>(HitResult.Actor);
			if (Damageable)
			{
				FDamageInfo DamageData;
				DamageData.Instigator = GetInstigator();
				DamageData.DamageDealer = this;
				DamageData.DamageValue = LaserDamage;
				Damageable->TakeDamage(DamageData);
			}
			else
			{
				HitResult.Actor->Destroy();
			}
		}
	}
	else if (Type == CanonType::ProjectileCanon && AmmoType)
	{
		GEngine->AddOnScreenDebugMessage(11, 1, FColor::Red, "Projectile");
		FActorSpawnParameters SpawnParams;
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.Owner = this;
		//GetWorld()->SpawnActor<AProjectile>(AmmoType, ProjectileSpawnPoint->GetComponentTransform(), SpawnParams);
		
		for (size_t i = 0; i < 20; i++)
		{
			if (!Pool[i].IsFree)
			{
				if (!Pool[i].Projectile.IsValid())
				{
					Pool[i].IsFree = true;
				}
			}

			if (Pool[i].IsFree)
			{
				Pool[i].Projectile = GetWorld()->SpawnActor<AProjectile>(AmmoType, ProjectileSpawnPoint->GetComponentTransform(), SpawnParams);
				Pool[i].IsFree = false;
				break;
			}
		}


	}
}

void ACanon::AddAmmo(int32 Count)
{
	Ammo = Ammo + Count;
}

int32 ACanon::GetCurrAmmo() const
{
	return Ammo;
}

void ACanon::SetCurrAmmo(int32 Count)
{
	Ammo = Count;
}


