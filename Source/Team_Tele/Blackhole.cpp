// Fill out your copyright notice in the Description page of Project Settings.
// Blackhole.cpp

#include "Blackhole.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h" 
#include "Engine/EngineTypes.h"

// Sets default values
ABlackhole::ABlackhole()
{
	PrimaryActorTick.bCanEverTick = true;

	// Static Mesh
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = MeshComp;

	// Gravity Sphere
	SphereGravityComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereGravityComp"));
	SphereGravityComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereGravityComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereGravityComp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	SphereGravityComp->SetGenerateOverlapEvents(true);
	SphereGravityComp->SetupAttachment(MeshComp);
	SphereGravityComp->SetSphereRadius(1000.0f);

	// 부모(액터/메시) 스케일 상속 끊고 월드 스케일 1로 고정 -> 와 부모 상속이 핵심 문제였음 진짜 개얼탱 미쳣나
	SphereGravityComp->SetAbsolute(false, false, true);
	SphereGravityComp->SetWorldScale3D(FVector(1.f));

	// Destroy Sphere
	SphereDestroyComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereDestroyComp"));
	SphereDestroyComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereDestroyComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereDestroyComp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	SphereDestroyComp->SetGenerateOverlapEvents(true);
	SphereDestroyComp->SetupAttachment(MeshComp);
	SphereDestroyComp->SetSphereRadius(1.0f);

	// 이쪽도 동일하게
	SphereDestroyComp->SetAbsolute(false, false, true);
	SphereDestroyComp->SetWorldScale3D(FVector(1.f));

	// 반경 와이어 표시
	// 게임(PIE) 중엔 안 보이게
	SphereGravityComp->SetHiddenInGame(true);
	SphereDestroyComp->SetHiddenInGame(true);

	#if WITH_EDITOR
	// 에디터 뷰포트에선 항상 반경 와이어가 보이게
	SphereGravityComp->bDrawOnlyIfSelected = false;
	SphereDestroyComp->bDrawOnlyIfSelected = false;

	// 색상 구분(선택)
	SphereGravityComp->ShapeColor = FColor::Blue;
	SphereDestroyComp->ShapeColor = FColor::Red;
	#endif

	// Bind event
	SphereDestroyComp->OnComponentBeginOverlap.AddDynamic(this, &ABlackhole::OverlapInnerSphere);
}

void ABlackhole::BeginPlay()
{
	Super::BeginPlay();

	// 추가: 입력 활성화 + 바인딩
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		EnableInput(PC);
		if (InputComponent)
		{
			InputComponent->BindAxis(TEXT("MoveForward"), this, &ABlackhole::MoveForward);
			InputComponent->BindAxis(TEXT("MoveRight"), this, &ABlackhole::MoveRight);
			InputComponent->BindAxis(TEXT("MoveUp"), this, &ABlackhole::MoveUp);
		}
	}
}

void ABlackhole::OverlapInnerSphere(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 블랙홀 중심에 닿은 오브젝트는 '파괴' 대신 '숨김 + 비활성화' 처리
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("== OverlapInnerSphere triggered with: %s"), *OtherActor->GetName());

	// 이미 소비된(숨겨둔) 액터라면 중복 처리 방지
	static const FName ConsumedTag = TEXT("BH_Consumed");
	if (OtherActor->Tags.Contains(ConsumedTag))
	{
		return;
	}

	// 모든 프리미티브 컴포넌트 비활성화(가시성/충돌/물리)
	TArray<UPrimitiveComponent*> PrimComps;
	OtherActor->GetComponents<UPrimitiveComponent>(PrimComps);

	for (UPrimitiveComponent* PC : PrimComps)
	{
		if (!PC) continue;

		// 화면에서 보이지 않게
		PC->SetVisibility(false, true);
		PC->SetHiddenInGame(true, true);

		// 물리/충돌 정지
		if (PC->IsSimulatingPhysics())
		{
			PC->SetSimulatePhysics(false);
		}
		PC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 액터 자체도 비활성화
	OtherActor->SetActorEnableCollision(false);
	OtherActor->SetActorHiddenInGame(true);
	OtherActor->SetActorTickEnabled(false);

	// 이미 소비되었음을 표시 → 이후 체크에서 제외
	OtherActor->Tags.AddUnique(ConsumedTag);
}

void ABlackhole::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	/*UE_LOG(LogTemp, Warning, TEXT("Blackhole ticking..."));*/

	CheckNearbyActors();
}

void ABlackhole::MoveForward(float Value)
{
	if (!FMath::IsNearlyZero(Value))
		UE_LOG(LogTemp, Warning, TEXT("Forward: %f"), Value);

	if (FMath::IsNearlyZero(Value)) return;

	const FVector Delta = GetActorForwardVector() * Value * MoveSpeed * GetWorld()->GetDeltaSeconds();
	SetActorLocation(GetActorLocation() + Delta, true); // true: sweep(충돌 체크)
}

void ABlackhole::MoveRight(float Value)
{
	if (!FMath::IsNearlyZero(Value))
		UE_LOG(LogTemp, Warning, TEXT("Right: %f"), Value);

	if (FMath::IsNearlyZero(Value)) return;

	const FVector Delta = GetActorRightVector() * Value * MoveSpeed * GetWorld()->GetDeltaSeconds();
	SetActorLocation(GetActorLocation() + Delta, true); // true: sweep(충돌 체크)
}

void ABlackhole::MoveUp(float Value)
{
	if (FMath::IsNearlyZero(Value)) return;

	const FVector Delta = FVector::UpVector * Value * MoveSpeed * GetWorld()->GetDeltaSeconds();
	SetActorLocation(GetActorLocation() + Delta, true); // true = 충돌 체크
}

void ABlackhole::CheckNearbyActors()
{
	static const FName ConsumedTag(TEXT("BH_Consumed"));

	// 1) 기본: 스피어 오버랩 수집
	TArray<UPrimitiveComponent*> OverlappingComps;
	SphereGravityComp->GetOverlappingComponents(OverlappingComps);

	// 1-보강: 비어 있으면 물리 바디 대상으로 반경 쿼리
	if (OverlappingComps.Num() == 0)
	{
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
		ObjTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));

		TArray<AActor*> Ignore;
		Ignore.Add(this);

		UKismetSystemLibrary::SphereOverlapComponents(
			GetWorld(),
			GetActorLocation(),
			SphereGravityComp->GetScaledSphereRadius(),
			ObjTypes,
			UPrimitiveComponent::StaticClass(),
			Ignore,
			OverlappingComps
		);
	}

	const FVector Center = GetActorLocation();
	const float OuterR = SphereGravityComp->GetScaledSphereRadius();
	const float InnerR = SphereDestroyComp->GetScaledSphereRadius();
	const float Denom = FMath::Max(OuterR - InnerR, 1.f);

	for (UPrimitiveComponent* Comp : OverlappingComps)
	{
		if (!Comp) continue;

		AActor* OwnerActor = Comp->GetOwner();
		if (!OwnerActor || OwnerActor == this) continue;
		if (OwnerActor->Tags.Contains(ConsumedTag)) continue;

		// 실제 시뮬 중인 프리미티브 찾기
		UPrimitiveComponent* SimComp = Comp->IsSimulatingPhysics() ? Comp : nullptr;
		if (!SimComp)
		{
			TArray<UPrimitiveComponent*> OwnerComps;
			OwnerActor->GetComponents<UPrimitiveComponent>(OwnerComps);
			for (UPrimitiveComponent* P : OwnerComps)
			{
				if (P && P->IsSimulatingPhysics()) { SimComp = P; break; }
			}
			if (!SimComp) continue;
		}

		// 기하/속도 계산
		const FVector ToC = Center - SimComp->GetComponentLocation();
		float r = ToC.Size();
		if (r <= KINDA_SMALL_NUMBER) continue;

		const FVector Dir = ToC / r;
		const FVector V = SimComp->GetPhysicsLinearVelocity();
		const float   Vr = FVector::DotProduct(V, Dir);      // +: 바깥, -: 안쪽
		const FVector Vt = V - Vr * Dir;                     // 접선

		// 거리 보간(Inner~Outer)
		const float t = FMath::Clamp((r - InnerR) / Denom, 0.f, 1.f);

		// 방사(PD) 가속도
		const float Kp = FMath::Lerp(KpInner, KpOuter, t);
		float aRadial = Kp - KdRadial * Vr;             // 안쪽이 +가 되도록
		aRadial = FMath::Max(aRadial, MinRadialAccel);

		// 슬립 방지
		SimComp->WakeAllRigidBodies();

		// 안쪽 당김
		SimComp->AddForce(Dir * aRadial, NAME_None, /*bAccelChange=*/true);

		// 접선 감쇠(빙빙 도는 것 억제)
		if (!Vt.IsNearlyZero())
		{
			SimComp->AddForce(-Vt * TangentialDamping, NAME_None, /*bAccelChange=*/true);
		}

		// 아래 성분 보정(중력 + 블랙홀 아래 방향)
		const float G = SimComp->IsGravityEnabled() ? -GetWorld()->GetGravityZ() : 0.f; // ~980
		const float bhDown = (Dir.Z < 0.f) ? (-Dir.Z * aRadial) : 0.f;
		const float upScale = FMath::Lerp(GravityCancelInner, GravityCancelOuter, t);
		const float upCancel = (G + bhDown) * upScale;

		if (upCancel > 0.f)
		{
			SimComp->AddForce(FVector(0.f, 0.f, upCancel), NAME_None, /*bAccelChange=*/true);
		}
	}
}

