// Fill out your copyright notice in the Description page of Project Settings.


#include "World/ShooterCoopGameMode.h"
#include "NavigationSystem.h"
#include "ShooterPlayerState.h"
#include "ShooterCharacter.h"
#include "World/ShooterGameState.h"
#include "EngineUtils.h"
#include "ShooterPlayerController.h"


AShooterCoopGameMode::AShooterCoopGameMode()
{
	/* Disable damage to coop buddies  */
	bAllowFriendlyFireDamage = false;
	bSpawnAtTeamPlayer = true;

	ScoreNightSurvived = 1000;
}


/*
	RestartPlayer - Spawn the player next to his living coop buddy instead of a PlayerStart
*/
void AShooterCoopGameMode::RestartPlayer(class AController* NewPlayer)
{
	/* Fallback to PlayerStart picking if team spawning is disabled or we're trying to spawn a bot. */
	if (!bSpawnAtTeamPlayer || (NewPlayer->PlayerState && NewPlayer->PlayerState->IsABot()))
	{
		Super::RestartPlayer(NewPlayer);
		return;
	}

	/* Look for a live player to spawn next to */
	FVector SpawnOrigin = FVector::ZeroVector;
	FRotator StartRotation = FRotator::ZeroRotator;
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		AShooterCharacter* MyCharacter = Cast<AShooterCharacter>(*It);
		if (MyCharacter && MyCharacter->IsAlive())
		{
			/* Get the origin of the first player we can find */
			SpawnOrigin = MyCharacter->GetActorLocation();
			StartRotation = MyCharacter->GetActorRotation();
			break;
		}
	}

	/* No player is alive (yet) - spawn using one of the PlayerStarts */
	if (SpawnOrigin == FVector::ZeroVector)
	{
		Super::RestartPlayer(NewPlayer);
		return;
	}

	/* Get a point on the nav mesh near the other player */
	FNavLocation StartLocation;
	UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetNavigationSystem(this);
	if (NavSystem && NavSystem->GetRandomPointInNavigableRadius(SpawnOrigin, 250.0f, StartLocation))
	{
		// Try to create a pawn to use of the default class for this player
		if (NewPlayer->GetPawn() == nullptr && GetDefaultPawnClassForController(NewPlayer) != nullptr)
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Instigator = GetInstigator();
			APawn* ResultPawn = GetWorld()->SpawnActor<APawn>(GetDefaultPawnClassForController(NewPlayer), StartLocation.Location, StartRotation, SpawnInfo);
			if (ResultPawn == nullptr)
			{
				UE_LOG(LogGameMode, Warning, TEXT("Couldn't spawn Pawn of type %s at %s"), *GetNameSafe(DefaultPawnClass), &StartLocation.Location);
			}
			NewPlayer->SetPawn(ResultPawn);
		}

		if (NewPlayer->GetPawn() == nullptr)
		{
			NewPlayer->FailedToSpawnPawn();
		}
		else
		{
			NewPlayer->Possess(NewPlayer->GetPawn());

			// If the Pawn is destroyed as part of possession we have to abort
			if (NewPlayer->GetPawn() == nullptr)
			{
				NewPlayer->FailedToSpawnPawn();
			}
			else
			{
				// Set initial control rotation to player start's rotation
				NewPlayer->ClientSetRotation(NewPlayer->GetPawn()->GetActorRotation(), true);

				FRotator NewControllerRot = StartRotation;
				NewControllerRot.Roll = 0.f;
				NewPlayer->SetControlRotation(NewControllerRot);

				SetPlayerDefaults(NewPlayer->GetPawn());
			}
		}
	}
}


void AShooterCoopGameMode::OnNightEnded()
{
	/* Respawn spectating players that died during the night */
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		/* Look for all players that are spectating */
		AShooterPlayerController* MyController = Cast<AShooterPlayerController>(*It);
		if (MyController)
		{
			if (MyController->PlayerState->IsSpectator())
			{
				RestartPlayer(MyController);
				MyController->ClientHUDStateChanged(EHUDState::Playing);
			}
			else
			{
				/* Player still alive, award him some points */
				AShooterCharacter* MyPawn = Cast<AShooterCharacter>(MyController->GetPawn());
				if (MyPawn && MyPawn->IsAlive())
				{
					AShooterPlayerState* PS = Cast<AShooterPlayerState>(MyController->PlayerState);
					if (PS)
					{
						PS->ScorePoints(ScoreNightSurvived);
					}
				}
			}
		}
	}
}


void AShooterCoopGameMode::Killed(AController* Killer, AController* VictimPlayer, APawn* VictimPawn, const UDamageType* DamageType)
{
	AShooterPlayerState* KillerPS = Killer ? Cast<AShooterPlayerState>(Killer->PlayerState) : nullptr;
	AShooterPlayerState* VictimPS = VictimPlayer ? Cast<AShooterPlayerState>(VictimPlayer->PlayerState) : nullptr;

	if (KillerPS && KillerPS != VictimPS && !KillerPS->IsABot())
	{
		KillerPS->AddKill();
		KillerPS->ScorePoints(10);
	}

	if (VictimPS && !VictimPS->IsABot())
	{
		VictimPS->AddDeath();
	}

	/* End match is all players died */
	CheckMatchEnd();
}


void AShooterCoopGameMode::CheckMatchEnd()
{
	bool bHasAlivePlayer = false;
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		AShooterCharacter* MyPawn = Cast<AShooterCharacter>(*It);
		if (MyPawn && MyPawn->IsAlive())
		{
			AShooterPlayerState* PS = Cast<AShooterPlayerState>(MyPawn->GetPlayerState());
			if (PS)
			{
				if (!PS->IsABot())
				{
					/* Found one player that is still alive, game will continue */
					bHasAlivePlayer = true;
					break;
				}
			}
		}
	}

	/* End game is all players died */
	if (!bHasAlivePlayer)
	{
		FinishMatch();
	}
}


void AShooterCoopGameMode::FinishMatch()
{
	if (IsMatchInProgress())
	{
		EndMatch();

		/* Stop spawning bots */
		GetWorldTimerManager().ClearTimer(TimerHandle_BotSpawns);

		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
		{
			AShooterPlayerController* MyController = Cast<AShooterPlayerController>(*It);
			if (MyController)
			{
				MyController->ClientHUDStateChanged(EHUDState::MatchEnd);
			}
		}
	}
}