// Blackhole.cpp

#include "Blackhole.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"


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
	SphereGravityComp->SetupAttachment(MeshComp);
	SphereGravityComp->SetSphereRadius(1000.0f); //  ���⼭ �� ���� ����

	// Destroy Sphere
	SphereDestroyComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereDestroyComp"));
	SphereDestroyComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereDestroyComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereDestroyComp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	SphereDestroyComp->SetupAttachment(MeshComp);
	SphereDestroyComp->SetSphereRadius(1.0f);  //  Destroy ������ ����

	// Bind event
	SphereDestroyComp->OnComponentBeginOverlap.AddDynamic(this, &ABlackhole::OverlapInnerSphere);
}

void ABlackhole::BeginPlay()
{
	Super::BeginPlay();
}

void ABlackhole::OverlapInnerSphere(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("== OverlapInnerSphere triggered with: %s"), *OtherActor->GetName());
	if (OtherActor && OtherActor != this)
	{
		OtherActor->Destroy();
	}
}

void ABlackhole::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UE_LOG(LogTemp, Warning, TEXT("Blackhole ticking..."));

	CheckNearbyActors();
}

void ABlackhole::CheckNearbyActors()
{
	TArray<UPrimitiveComponent*> OverlappingComps;
	SphereGravityComp->GetOverlappingComponents(OverlappingComps);

	for (UPrimitiveComponent* Comp : OverlappingComps)
	{
		if (Comp && Comp->IsSimulatingPhysics())
		{
			FVector Direction = GetActorLocation() - Comp->GetComponentLocation();
			float Distance = Direction.Size();
			Direction.Normalize();

			// �߽ɿ� �������� �� ���� �η�
			// ������ ��� -> �������� ���� �� ������, clamp ������� �ʹ� ������ �� ����
			float ForceStrength = 100000.0f / FMath::Clamp(Distance * Distance, 100.0f, 3000.0f);
			/*float ForceStrength = 1e10f / FMath::Clamp(Distance * Distance, 5000.0f, 100000.0f);*/


			Comp->AddForce(Direction * ForceStrength, NAME_None, true);


			UE_LOG(LogTemp, Warning, TEXT("Distance: %.2f, Force: %.2f, Mass: %.2f, SimPhysics: %s"),
				Distance, ForceStrength, Comp->GetMass(),
				Comp->IsSimulatingPhysics() ? TEXT("true") : TEXT("false"));

		}
	}

	//  ���� �ٱ����� �����ִ� ��ü �������
	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);

	for (AActor* Actor : AllActors)
	{
		if (Actor == this) continue;

		TArray<UPrimitiveComponent*> Components;
		Actor->GetComponents<UPrimitiveComponent>(Components);

		for (UPrimitiveComponent* Comp : Components)
		{
			if (Comp && Comp->IsSimulatingPhysics())
			{
				float Dist = FVector::Dist(GetActorLocation(), Comp->GetComponentLocation());
				float MaxRadius = SphereGravityComp->GetScaledSphereRadius();

				if (Dist > MaxRadius)
				{
					FVector PullBackDir = GetActorLocation() - Comp->GetComponentLocation();
					PullBackDir.Normalize();

					float PullBackStrength = 300.0f; // �ٱ��� �ִ� ��� ������ �������� ������� ��
					Comp->AddForce(PullBackDir * PullBackStrength, NAME_None, true);

					UE_LOG(LogTemp, Warning, TEXT("PullBack to: %s"), *Comp->GetName());
				}
			}
		}
	}
}