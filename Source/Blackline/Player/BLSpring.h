#pragma once

#include "CoreMinimal.h"

/**
 * Muelle amortiguado para movimientos procedurales (sway, aterrizajes, retroceso).
 * Frequency en Hz (más alto = más rígido), DampingRatio 1 = amortiguamiento crítico (sin rebote),
 * < 1 rebota ligeramente. Integra en sub-pasos de 1/120 s para ser estable con frame rate variable.
 */
struct FBLSpringVector
{
	FVector Value = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;

	void Update(const FVector& Target, float DeltaTime, float Frequency, float DampingRatio)
	{
		if (DeltaTime <= 0.f)
		{
			return;
		}
		const float Omega = 2.f * PI * Frequency;
		const int32 Steps = FMath::Clamp(FMath::CeilToInt(DeltaTime * 120.f), 1, 8);
		const float H = DeltaTime / Steps;
		for (int32 i = 0; i < Steps; ++i)
		{
			const FVector Accel = (Target - Value) * (Omega * Omega) - Velocity * (2.f * DampingRatio * Omega);
			Velocity += Accel * H;
			Value += Velocity * H;
		}
	}

	void AddImpulse(const FVector& Impulse) { Velocity += Impulse; }
	void Reset() { Value = Velocity = FVector::ZeroVector; }
};

/** Interpolación exponencial independiente del frame rate (Speed ~ 1/segundos de respuesta). */
FORCEINLINE float BLExpInterp(float Current, float Target, float DeltaTime, float Speed)
{
	return FMath::Lerp(Target, Current, FMath::Exp(-Speed * DeltaTime));
}

FORCEINLINE FVector BLExpInterp(const FVector& Current, const FVector& Target, float DeltaTime, float Speed)
{
	return FMath::Lerp(Target, Current, FMath::Exp(-Speed * DeltaTime));
}
