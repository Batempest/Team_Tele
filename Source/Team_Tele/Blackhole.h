// Fill out your copyright notice in the Description page of Project Settings.
// Blackhole.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/PlayerController.h"
#include "Blackhole.generated.h"

UCLASS()
class TEAM_TELE_API ABlackhole : public AActor
{
	GENERATED_BODY()
	
public:
	ABlackhole();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* SphereGravityComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* SphereDestroyComp;

	// ==== [�߰�] �÷��̾� �� ��Ȧ ���� ��ȯ ���� ====
	bool bControllingBlackhole = false;
	APlayerController* CachedPC = nullptr;

	void ToggleControl(); // Tab Ű�� ȣ��

	// �̵� �ӵ� & �Է� ó�� �Լ� 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float MoveSpeed = 500.f;

	void MoveForward(float Value); //��, ��
	void MoveRight(float Value); //��, ��
	void MoveUp(float Value); //��, ��

	//// === ��Ȧ ������ Ʃ�� �Ķ���� (�����Ϳ��� ���� ����, private ����) ===

	/*�ָ����� �� �������� -> KpOuter ����, �ʿ�� MinRadialAccel ����

		�߽� ��ó���� �ް��� / Ƣ�� ���� -> KdRadial ���� �Ǵ� KpInner ����

		���� ���� �ð� ��� -> TangentialDamping ����

		������ �ڲ� ó���� -> GravityCancelOuter ����

		�߽� �տ��� �յ� �� -> GravityCancelInner / Outer ���� �Ǵ� KpInner / KpOuter ����

		���� ����(�����) :
		KpInner 500~2000, KpOuter 3000~15000, KdRadial 0.5~4, MinRadialAccel 50~600, TangentialDamping 0~3, GravityCancelInner 0~0.7, GravityCancelOuter 0.7~1.2.*/

	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float KpInner = 1200.f; //��Ȧ ���� ���Ǿ�(�߽�) �η� ����
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float KpOuter = 5000.f; //��Ȧ �ܺ� ��� �η� ����
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float KdRadial = 2.0f; //�ⷷ�� �����ϴ� ���� ��� (�����̶� ���)
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float MinRadialAccel = 200.f; //���� ���ӵ� ����ġ
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float TangentialDamping = 1.5f; //���� �ӵ� ���� ���� (��� ��¥ ����)
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float GravityCancelInner = 0.4f; // t=0 //�߷� �߽� ����
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float GravityCancelOuter = 1.0f; // t=1 //�߷� �ܺ� ����

	void CheckNearbyActors();

	UFUNCTION()
	void OverlapInnerSphere(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

};
