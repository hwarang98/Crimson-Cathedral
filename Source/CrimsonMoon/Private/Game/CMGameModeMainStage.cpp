// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/CMGameModeMainStage.h"
#include "Game/GameInitialization/CMGameInstance.h"
#include "GameplayTags/CMGameplayTags_Character.h"
#include "GameFramework/PlayerController.h"
#include "Game/CMGameStateMainStageV2.h"
#include "Game/CMPlayerState.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Character/CMCharacterBase.h"
#include "GameplayAbilitySystem/CMAbilitySystemComponent.h"
#include "Components/CMInventoryComponent.h"
#include "Items/Object/CMItemInstance.h"
#include "GameplayTags/CMGameplayTags_Currency.h"

ACMGameModeMainStage::ACMGameModeMainStage()
{
	bUseSeamlessTravel = true;
}

void ACMGameModeMainStage::BeginPlay()
{
	Super::BeginPlay();

	// 맵에 존재하는 모든 PlayerStart를 캐싱
	CacheAllPlayerStarts();

	if (UCMGameInstance* GI = Cast<UCMGameInstance>(GetGameInstance()))
	{
		UE_LOG(LogTemp, Log, TEXT("ACMGameModeMainStage::BeginPlay - Player Nickname: %s"), *GI->GetPlayerNickname());

		if (!GI->GetIsCreatedSession())
		{
			GI->CreateGameSession();
		}
	}
}

void ACMGameModeMainStage::CacheAllPlayerStarts()
{
	CachedPlayerStarts.Empty();
	NextPlayerStartIndex = 0;

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("CacheAllPlayerStarts: World is null"));
		return;
	}

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		APlayerStart* PS = *It;
		if (IsValid(PS))
		{
			CachedPlayerStarts.Add(PS);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CacheAllPlayerStarts: Cached %d PlayerStarts"), CachedPlayerStarts.Num());
}

void ACMGameModeMainStage::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 서버에서만 GameState의 플레이어 수를 증가
	if (HasAuthority())
	{
		if (ACMGameStateMainStageV2* GS = GetGameState<ACMGameStateMainStageV2>())
		{
			GS->IncrementConnectedPlayerCount();
		}
	}
}

void ACMGameModeMainStage::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// 서버에서만 GameState의 플레이어 수를 감소 및 사망 카운트 조정
	if (HasAuthority())
	{
		if (ACMGameStateMainStageV2* GS = GetGameState<ACMGameStateMainStageV2>())
		{
			// 나가는 플레이어의 PlayerState가 죽은 상태였다면 DeathCount도 감소
			if (APlayerState* PS = Exiting ? Exiting->GetPlayerState<APlayerState>() : nullptr)
			{
				if (ACMPlayerState* CMPS = Cast<ACMPlayerState>(PS))
				{
					if (!CMPS->GetIsPlayerDead())
					{
						GS->DecrementDeathCount();
					}
				}
			}

			GS->DecrementConnectedPlayerCount();
		}
	}
}

void ACMGameModeMainStage::SpawnPlayerForController(APlayerController* NewPlayer, const FGameplayTag& SelectedCharacterTag)
{
	if (!NewPlayer)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<APawn> PawnClassToSpawn = nullptr;

	if (SelectedCharacterTag == CMGameplayTags::Character_Class_Blade)
	{
		PawnClassToSpawn = BladePlayerPawnClass;
	}
	else if (SelectedCharacterTag == CMGameplayTags::Character_Class_Arcanist)
	{
		PawnClassToSpawn = ArcanistPlayerPawnClass;
	}
	else
	{
		PawnClassToSpawn = DefaultPlayerPawnClass;
	}

	if (!*PawnClassToSpawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPlayerForController - PawnClassToSpawn is null (Tag: %s)"), *SelectedCharacterTag.ToString());
		return;
	}

	// 캐싱된 PlayerStart에서 순차적으로 위치 선택
	FTransform SpawnTransform;
	if (CachedPlayerStarts.Num() > 0)
	{
		if (!CachedPlayerStarts.IsValidIndex(NextPlayerStartIndex))
		{
			// 인덱스가 범위를 벗어나면 0으로 래핑
			NextPlayerStartIndex = 0;
		}

		APlayerStart* ChosenStart = CachedPlayerStarts[NextPlayerStartIndex];
		if (IsValid(ChosenStart))
		{
			SpawnTransform = ChosenStart->GetActorTransform();
		}
		else
		{
			// 유효하지 않으면 기본값 사용
			SpawnTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector);
		}

		// 다음 스폰을 위해 인덱스 증가
		++NextPlayerStartIndex;
	}
	else
	{
		// PlayerStart가 하나도 없으면 원점에 스폰
		UE_LOG(LogTemp, Warning, TEXT("SpawnPlayerForController: No PlayerStarts cached. Spawning at origin."));
		SpawnTransform = FTransform(FRotator::ZeroRotator, FVector::ZeroVector);
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = nullptr;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* NewPawn = World->SpawnActor<APawn>(PawnClassToSpawn, SpawnTransform, SpawnParams);
	if (!NewPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnPlayerForController - Failed to spawn pawn"));
		return;
	}

	// GameInstance에서 이전 맵의 저장 데이터 확인 (Seamless Travel 시 로비 데이터 복원용)
	UCMGameInstance* GI = Cast<UCMGameInstance>(GetGameInstance());
	FString PlayerId = NewPlayer->GetName();

	UCMGameInstance::FPlayerTransferData PreviousSave;
	bool bHasPreviousSave = false;
	if (GI)
	{
		bHasPreviousSave = GI->LoadAndClearPlayerTransferData(PlayerId, PreviousSave);
	}

	// 기존 폰에서 재화/인벤토리 데이터 추출
	int32 SavedCurrency = 0;
	TArray<TPair<UCMDataAsset_ItemBase*, int32>> SavedInventoryItems;

	if (APawn* OldPawn = NewPlayer->GetPawn())
	{
		if (OldPawn != NewPawn)
		{
			if (ACMCharacterBase* OldCharacter = Cast<ACMCharacterBase>(OldPawn))
			{
				if (UCMAbilitySystemComponent* OldASC = OldCharacter->GetCMAbilitySystemComponent())
				{
					SavedCurrency = OldASC->GetCurrentCurrencyAmount();
				}
			}

			if (UCMInventoryComponent* OldInventory = OldPawn->FindComponentByClass<UCMInventoryComponent>())
			{
				for (UCMItemInstance* Item : OldInventory->GetInventoryItems())
				{
					if (Item && Item->ItemData)
					{
						SavedInventoryItems.Emplace(Item->ItemData, Item->Quantity);
					}
				}
			}

			// 이전 맵 저장 데이터가 없을 때만 GameInstance에 저장 (다음 맵 전환 대비)
			if (GI && !bHasPreviousSave)
			{
				UCMGameInstance::FPlayerTransferData TransferData;
				TransferData.Currency = SavedCurrency;
				TransferData.InventoryItems = SavedInventoryItems;
				GI->SavePlayerTransferData(PlayerId, TransferData);
			}

			UE_LOG(LogTemp, Log, TEXT("SpawnPlayerForController - Saved Currency: %d, Inventory Items: %d (HasPreviousSave: %s)"),
				SavedCurrency, SavedInventoryItems.Num(), bHasPreviousSave ? TEXT("true") : TEXT("false"));

			OldPawn->DetachFromControllerPendingDestroy();
			OldPawn->Destroy();
		}
	}

	NewPlayer->Possess(NewPawn);

	// 복원할 데이터 결정: 이전 맵 데이터(GameInstance) 우선, 없으면 같은 맵 OldPawn 데이터 사용
	int32 CurrencyToRestore = bHasPreviousSave ? PreviousSave.Currency : SavedCurrency;
	const TArray<TPair<UCMDataAsset_ItemBase*, int32>>& ItemsToRestore = bHasPreviousSave ? PreviousSave.InventoryItems : SavedInventoryItems;

	// 재화 복원 (Delta 방식: 새 폰의 기본 재화와의 차이만 적용)
	if (ACMCharacterBase* NewCharacter = Cast<ACMCharacterBase>(NewPawn))
	{
		if (UCMAbilitySystemComponent* NewASC = NewCharacter->GetCMAbilitySystemComponent())
		{
			int32 CurrentCurrency = NewASC->GetCurrentCurrencyAmount();
			int32 Delta = CurrencyToRestore - CurrentCurrency;

			if (Delta > 0)
			{
				NewASC->AddCurrency(Delta, CMGameplayTags::Currency_Reason_Debug);
			}
			else if (Delta < 0)
			{
				bool bSuccess;
				NewASC->SpendCurrency(-Delta, CMGameplayTags::Currency_Reason_Debug, bSuccess);
			}

			UE_LOG(LogTemp, Log, TEXT("SpawnPlayerForController - Currency Restore: %d -> %d (Delta: %d)"),
				CurrentCurrency, CurrencyToRestore, Delta);
		}
	}

	// 인벤토리 복원
	if (ItemsToRestore.Num() > 0)
	{
		if (UCMInventoryComponent* NewInventory = NewPawn->FindComponentByClass<UCMInventoryComponent>())
		{
			for (const TPair<UCMDataAsset_ItemBase*, int32>& Entry : ItemsToRestore)
			{
				NewInventory->AddItem(Entry.Key, Entry.Value);
			}
		}
	}
}

void ACMGameModeMainStage::ReturnToExitMap()
{
	// 서버에서만 동작하도록 안전장치
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("ReturnToExitMap called on non-authority."));
		return;
	}

	if (ExitMapTravelURL.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ReturnToExitMap: ExitMapTravelURL is empty. Please set it in the GameModeMainStage blueprint."));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UCMGameInstance* GI = Cast<UCMGameInstance>(GetGameInstance()))
		{
			GI->SaveAllPlayersTransferData();
		}

		FString FinalURL = ExitMapTravelURL;
		if (!FinalURL.Contains(TEXT("?")))
		{
			FinalURL += TEXT("?listen");
		}
		else if (!FinalURL.Contains(TEXT("listen")))
		{
			FinalURL += TEXT("&listen");
		}
		World->ServerTravel(FinalURL);
	}
}