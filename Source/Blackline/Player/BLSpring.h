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
		// Euler semiimplícito: estable solo si Omega * H < 2. Con 8 pasos fijos, un frame largo (tirón, carga, captura)
		// daba pasos de 50 ms y el muelle explotaba: la cámara giraba 80° un frame y las balas salían desviadas.
		// Pasos según la frecuencia (H <= 0,5 / Omega) y como mucho 0,1 s de simulación por frame.
		const float Omega = 2.f * PI * Frequency;
		const float Dt = FMath::Min(DeltaTime, 0.1f);
		const float MaxH = FMath::Min(1.f / 120.f, 0.5f / FMath::Max(Omega, 1.f));
		const int32 Steps = FMath::Clamp(FMath::CeilToInt(Dt / MaxH), 1, 64);
		const float H = Dt / Steps;
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
