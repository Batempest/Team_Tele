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

	// ==== [추가] 플레이어 ↔ 블랙홀 조종 전환 관련 ====
	bool bControllingBlackhole = false;
	APlayerController* CachedPC = nullptr;

	void ToggleControl(); // Tab 키로 호출

	// 이동 속도 & 입력 처리 함수 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float MoveSpeed = 500.f;

	void MoveForward(float Value); //앞, 뒤
	void MoveRight(float Value); //좌, 우
	void MoveUp(float Value); //상, 하

	//// === 블랙홀 끌어당김 튜닝 파라미터 (에디터에서 조정 가능, private 유지) ===

	/*멀리서도 잘 끌려오게 -> KpOuter 증가, 필요시 MinRadialAccel 증가

		중심 근처에서 급가속 / 튀는 느낌 -> KdRadial 증가 또는 KpInner 감소

		빙빙 도는 시간 길다 -> TangentialDamping 증가

		밑으로 자꾸 처박힘 -> GravityCancelOuter 증가

		중심 앞에서 둥둥 뜸 -> GravityCancelInner / Outer 감소 또는 KpInner / KpOuter 증가

		권장 범위(출발점) :
		KpInner 500~2000, KpOuter 3000~15000, KdRadial 0.5~4, MinRadialAccel 50~600, TangentialDamping 0~3, GravityCancelInner 0~0.7, GravityCancelOuter 0.7~1.2.*/

	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float KpInner = 1200.f; //블랙홀 제거 스피어(중심) 인력 세기
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float KpOuter = 5000.f; //블랙홀 외부 경계 인력 세기
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float KdRadial = 2.0f; //출렁임 방지하는 감쇠 계수 (댐핑이랑 비슷)
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float MinRadialAccel = 200.f; //내부 가속도 보장치
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float TangentialDamping = 1.5f; //접선 속도 감쇠 세기 (얘는 진짜 댐핑)
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float GravityCancelInner = 0.4f; // t=0 //중력 중심 세기
	UPROPERTY(EditAnywhere, Category = "Blackhole|Pull", meta = (AllowPrivateAccess = "true"))
	float GravityCancelOuter = 1.0f; // t=1 //중력 외부 세기

	void CheckNearbyActors();

	UFUNCTION()
	void OverlapInnerSphere(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

};
