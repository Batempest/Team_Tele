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

	// �θ�(����/�޽�) ������ ��� ���� ���� ������ 1�� ���� -> �� �θ� ����� �ٽ� �������� ��¥ ������ �̫���
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

	// ���ʵ� �����ϰ�
	SphereDestroyComp->SetAbsolute(false, false, true);
	SphereDestroyComp->SetWorldScale3D(FVector(1.f));

	// �ݰ� ���̾� ǥ��
	// ����(PIE) �߿� �� ���̰�
	SphereGravityComp->SetHiddenInGame(true);
	SphereDestroyComp->SetHiddenInGame(true);

	#if WITH_EDITOR
	// ������ ����Ʈ���� �׻� �ݰ� ���̾ ���̰�
	SphereGravityComp->bDrawOnlyIfSelected = false;
	SphereDestroyComp->bDrawOnlyIfSelected = false;

	// ���� ����(����)
	SphereGravityComp->ShapeColor = FColor::Blue;
	SphereDestroyComp->ShapeColor = FColor::Red;
	#endif

	// Bind event
	SphereDestroyComp->OnComponentBeginOverlap.AddDynamic(this, &ABlackhole::OverlapInnerSphere);
}

void ABlackhole::BeginPlay()
{
	Super::BeginPlay();

	// �߰�: �Է� Ȱ��ȭ + ���ε�
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
	// ��Ȧ �߽ɿ� ���� ������Ʈ�� '�ı�' ��� '���� + ��Ȱ��ȭ' ó��
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("== OverlapInnerSphere triggered with: %s"), *OtherActor->GetName());

	// �̹� �Һ��(���ܵ�) ���Ͷ�� �ߺ� ó�� ����
	static const FName ConsumedTag = TEXT("BH_Consumed");
	if (OtherActor->Tags.Contains(ConsumedTag))
	{
		return;
	}

	// ��� ������Ƽ�� ������Ʈ ��Ȱ��ȭ(���ü�/�浹/����)
	TArray<UPrimitiveComponent*> PrimComps;
	OtherActor->GetComponents<UPrimitiveComponent>(PrimComps);

	for (UPrimitiveComponent* PC : PrimComps)
	{
		if (!PC) continue;

		// ȭ�鿡�� ������ �ʰ�
		PC->SetVisibility(false, true);
		PC->SetHiddenInGame(true, true);

		// ����/�浹 ����
		if (PC->IsSimulatingPhysics())
		{
			PC->SetSimulatePhysics(false);
		}
		PC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// ���� ��ü�� ��Ȱ��ȭ
	OtherActor->SetActorEnableCollision(false);
	OtherActor->SetActorHiddenInGame(true);
	OtherActor->SetActorTickEnabled(false);

	// �̹� �Һ�Ǿ����� ǥ�� �� ���� üũ���� ����
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
	SetActorLocation(GetActorLocation() + Delta, true); // true: sweep(�浹 üũ)
}

void ABlackhole::MoveRight(float Value)
{
	if (!FMath::IsNearlyZero(Value))
		UE_LOG(LogTemp, Warning, TEXT("Right: %f"), Value);

	if (FMath::IsNearlyZero(Value)) return;

	const FVector Delta = GetActorRightVector() * Value * MoveSpeed * GetWorld()->GetDeltaSeconds();
	SetActorLocation(GetActorLocation() + Delta, true); // true: sweep(�浹 üũ)
}

void ABlackhole::MoveUp(float Value)
{
	if (FMath::IsNearlyZero(Value)) return;

	const FVector Delta = FVector::UpVector * Value * MoveSpeed * GetWorld()->GetDeltaSeconds();
	SetActorLocation(GetActorLocation() + Delta, true); // true = �浹 üũ
}

void ABlackhole::CheckNearbyActors()
{
	static const FName ConsumedTag(TEXT("BH_Consumed"));

	// 1) �⺻: ���Ǿ� ������ ����
	TArray<UPrimitiveComponent*> OverlappingComps;
	SphereGravityComp->GetOverlappingComponents(OverlappingComps);

	// 1-����: ��� ������ ���� �ٵ� ������� �ݰ� ����
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

		// ���� �ù� ���� ������Ƽ�� ã��
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

		// ����/�ӵ� ���
		const FVector ToC = Center - SimComp->GetComponentLocation();
		float r = ToC.Size();
		if (r <= KINDA_SMALL_NUMBER) continue;

		const FVector Dir = ToC / r;
		const FVector V = SimComp->GetPhysicsLinearVelocity();
		const float   Vr = FVector::DotProduct(V, Dir);      // +: �ٱ�, -: ����
		const FVector Vt = V - Vr * Dir;                     // ����

		// �Ÿ� ����(Inner~Outer)
		const float t = FMath::Clamp((r - InnerR) / Denom, 0.f, 1.f);

		// ���(PD) ���ӵ�
		const float Kp = FMath::Lerp(KpInner, KpOuter, t);
		float aRadial = Kp - KdRadial * Vr;             // ������ +�� �ǵ���
		aRadial = FMath::Max(aRadial, MinRadialAccel);

		// ���� ����
		SimComp->WakeAllRigidBodies();

		// ���� ���
		SimComp->AddForce(Dir * aRadial, NAME_None, /*bAccelChange=*/true);

		// ���� ����(���� ���� �� ����)
		if (!Vt.IsNearlyZero())
		{
			SimComp->AddForce(-Vt * TangentialDamping, NAME_None, /*bAccelChange=*/true);
		}

		// �Ʒ� ���� ����(�߷� + ��Ȧ �Ʒ� ����)
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

