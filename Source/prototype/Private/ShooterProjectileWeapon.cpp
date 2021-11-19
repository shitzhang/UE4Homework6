// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterProjectileWeapon.h"

AShooterProjectileWeapon::AShooterProjectileWeapon()
{
	StorageSlot = EInventorySlot::Secondary;
	WeaponType = EWeaponType::Rifle;
}

void AShooterProjectileWeapon::FireWeapon()
{
	AActor* MyOwner = GetOwner();
	if (MyOwner) 
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector MuzzleLocation = GetMuzzleLocation();
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, EyeRotation, SpawnParams);
	}
}
