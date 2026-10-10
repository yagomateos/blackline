#pragma once

#include "CoreMinimal.h"

/** Textos del menú (Bloque 10). Fuente: Docs/Lore.md. */
namespace BLMenuData
{
	/** Map: nivel; FirstPhase/NumPhases: sus puntos de inicio dentro de StartPhases / StartPhaseIds. */
	struct FMission { const TCHAR* Code; const TCHAR* Name; const TCHAR* Place; bool bAvailable; const TCHAR* Briefing;
		const TCHAR* Map = nullptr; int32 FirstPhase = 0; int32 NumPhases = 0;
		/** Id de LoadoutPrimaries que la misión impone (nullptr = la elige el jugador en EQUIPAMIENTO). */
		const TCHAR* ForcedPrimary = nullptr; };
	struct FWeapon { const TCHAR* Name; const TCHAR* Class; const TCHAR* Specs; const TCHAR* Text; bool bAvailable; float Stats[5]; };
	struct FIntel { const TCHAR* Title; const TCHAR* Body; };

	inline const FMission Missions[] = {
		{ TEXT("01"), TEXT("AMANECER ROTO"), TEXT("KESSRA-ESTE · PUERTO Y CALLE MAYOR · 06:10"), true,
			TEXT("Tomas Varek, nuestro informante en el puerto, no responde desde las 04:00. Su refugio está al final de la calle "
				 "principal, al otro lado del patio de contenedores. Entra por los callejones del norte, cruza el patio, pasa el control "
				 "de carretera de la Columna y recupera su disco duro. Patrullas por parejas, posiciones con sacos terreros, sin apoyo aéreo. "
				 "Evita el ruido mientras puedas."), TEXT("/Game/Maps/M01/L_M01_AmanecerRoto"), 0, 8 },
		{ TEXT("02"), TEXT("MANIFIESTO"), TEXT("REFINERÍA DE KESSRA · ALMACÉN 7 · 03:40"), true,
			TEXT("Los manifiestos del disco de Varek señalan cuatro contenedores descargados en el almacén 7 de la refinería: \"pesa demasiado\", "
				 "\"esto no es grano\". Entra solo, en lancha por el canal de refrigeración, fotografía los contenedores KSR 4471, 4472 y 0918 "
				 "y sal sin que nadie sepa que has estado allí. De noche la oscuridad es tu cobertura: evita las linternas y los focos."),
			TEXT("/Game/Maps/M02/L_M02_Manifiesto"), 8, 5 },
		{ TEXT("03"), TEXT("RÍA"), TEXT("CASCO VIEJO · MUELLES DE LA RÍA · 07:20"), true,
			TEXT("Al fondo de las fotos del almacén 7 aparece un mercante gris sin nombre ni bandera: el \"barco sin bandera\" de las notas de Varek. "
				 "El práctico del puerto viejo, que lo guía de noche, guarda un cuaderno con sus entradas y salidas. Cruza el puente viejo con la niebla, "
				 "consigue el cuaderno en las casas de la ribera y coloca una baliza en el casco antes de que zarpe. Cargas de brecha para las puertas atrancadas. "
				 "Se han visto operadores de Corvane en los muelles."),
			TEXT("/Game/Maps/M03/L_M03_Ria"), 13, 6 },
		{ TEXT("04"), TEXT("FUEGO CRUZADO"), TEXT("PUENTE DEL FERROCARRIL · 16:40"), true,
			TEXT("La Columna lanza su ofensiva para cruzar la línea negra por el puente del ferrocarril, el único paso sobre el río que sigue en pie. "
				 "En el puesto del ejército está el enlace por satélite con el que TORRE va a sacar las pruebas del país. Aguanta con la compañía del "
				 "teniente Ilić mientras sube la transmisión: ametralladora en el búnker, apoyo aéreo a petición y cargas para volar el primer tramo "
				 "si no queda otra."),
			TEXT("/Game/Maps/M04/L_M04_FuegoCruzado"), 19, 6 },
		{ TEXT("05"), TEXT("LÍNEA NEGRA"), TEXT("TERMINAL DE CONTENEDORES · CAPITANÍA · 06:30"), true,
			TEXT("Con las pruebas fuera del país, la coalición autoriza por fin a BLACKLINE. Corvane recoge en la capitanía del puerto y quema "
				 "lo que no puede llevarse. Entra en la terminal con Sable 2-2 y 2-3, llega a la sala de servidores antes de que la quemen, descarga "
				 "los registros y atrapa a \"el inglés\". Vivo."),
			TEXT("/Game/Maps/M05/L_M05_LineaNegra"), 25, 5 },
	};

	inline const TCHAR* StartPhases[] = {
		// Misión 1 (0..7)
		TEXT("INSERCIÓN"), TEXT("PATIO DEL PUERTO"), TEXT("CONTROL DE CARRETERA"), TEXT("CALLE PRINCIPAL"), TEXT("LOCAL DE VAREK"),
		TEXT("BLOQUE DE VIVIENDAS"), TEXT("AZOTEA · BLINDADO"), TEXT("MUELLE · EXTRACCIÓN"),
		// Misión 2 (8..12)
		TEXT("CANAL · INSERCIÓN"), TEXT("PARQUE DE TANQUES"), TEXT("PATIO DE CARGA"), TEXT("ALMACÉN 7"), TEXT("ZONA DE PROCESO · HUIDA"),
		// Misión 3 (13..18)
		TEXT("PUENTE VIEJO · INSERCIÓN"), TEXT("CASCO VIEJO"), TEXT("CASAS DE LA RIBERA"), TEXT("PLAZA DEL CAMPANARIO"),
		TEXT("MUELLE DE PESCADORES"), TEXT("ESPIGÓN · BALIZA"),
		// Misión 4 (19..24)
		TEXT("RETAGUARDIA · CAMIONES"), TEXT("CABEZA DE PUENTE"), TEXT("BÚNKER · AMETRALLADORA"), TEXT("OBSERVATORIO · BLINDADO"),
		TEXT("CARGAS EN EL PUENTE"), TEXT("VADO · RETIRADA"),
		// Misión 5 (25..29)
		TEXT("ENTRADA DE LA TERMINAL"), TEXT("TERMINAL DE CONTENEDORES"), TEXT("CAPITANÍA · CONTRARRELOJ"), TEXT("SALA DE SERVIDORES"),
		TEXT("AZOTEA · EL INGLÉS") };
	inline const TCHAR* StartPhaseIds[] = {
		TEXT("Fase1"), TEXT("Fase2"), TEXT("Fase3"), TEXT("Fase4"), TEXT("Objetivo"), TEXT("Bloque"), TEXT("Azotea"), TEXT("Muelle"),
		TEXT("Canal"), TEXT("Tanques"), TEXT("Carga"), TEXT("Almacen"), TEXT("Proceso"),
		TEXT("Puente"), TEXT("Casco"), TEXT("Casas"), TEXT("Campanario"), TEXT("Muelle"), TEXT("Baliza"),
		TEXT("Inicio"), TEXT("Puesto"), TEXT("Ametralladora"), TEXT("Blindado"), TEXT("Voladura"), TEXT("Retirada"),
		TEXT("Inicio"), TEXT("Terminal"), TEXT("Capitania"), TEXT("Servidores"), TEXT("Azotea") };

	// Estadísticas 0..1: daño, cadencia, alcance, control, movilidad
	inline const FWeapon Weapons[] = {
		{ TEXT("AR-7 \"HALCÓN\""), TEXT("FUSIL DE ASALTO · PRINCIPAL"), TEXT("5,56 MM · 750 DPM · CARGADOR 30+1 · MIRAS METÁLICAS DE ANILLO"),
			TEXT("Fusil de pistón corto con guardamanos M-LOK y apagallamas de jaula. Preciso y controlable en ráfagas cortas; "
				 "la recarga táctica deja una bala en la recámara."), true, { 0.6f, 0.7f, 0.7f, 0.65f, 0.6f } },
		{ TEXT("P-17"), TEXT("PISTOLA · SECUNDARIA"), TEXT("9 MM · SEMIAUTOMÁTICA · CARGADOR 17"), TEXT("Arma de apoyo para cuando el fusil se queda sin cargador."), true, { 0.35f, 0.4f, 0.3f, 0.75f, 0.95f } },
		{ TEXT("SG-12 \"MASTÍN\""), TEXT("ESCOPETA · CQB"), TEXT("CAL. 12 · CORREDERA · 7+1 CARTUCHOS · 9 PERDIGONES"), TEXT("Escopeta de corredera para interiores: devastadora a menos de 15 m, inútil a distancia. Se recarga cartucho a cartucho."), true, { 0.95f, 0.2f, 0.15f, 0.4f, 0.55f } },
		{ TEXT("SMG-9 \"VESPER\""), TEXT("SUBFUSIL · CQB"), TEXT("9 MM · 900 DPM · CARGADOR 30"), TEXT("Compacto y rápido para interiores."), false, { 0.4f, 0.9f, 0.35f, 0.6f, 0.85f } },
		{ TEXT("M-6"), TEXT("GRANADA DE FRAGMENTACIÓN"), TEXT("ESPOLETA DE 4 S · RADIO LETAL 5 M"), TEXT("Saca al enemigo de su cobertura."), false, { 0.9f, 0.1f, 0.4f, 0.5f, 0.9f } },
	};

	/** Armas que se pueden elegir en EQUIPAMIENTO: id (UBLUserSettings::LoadoutPrimary), ficha en Weapons[] y asset real. */
	struct FLoadoutWeapon { const TCHAR* Id; int32 MenuIndex; const TCHAR* Asset; };
	inline const FLoadoutWeapon LoadoutPrimaries[] = {
		{ TEXT("AR7"), 0, TEXT("/Game/Weapons/AR7/DA_AR7.DA_AR7") },
		{ TEXT("SG12"), 2, TEXT("/Game/Weapons/SG12/DA_SG12.DA_SG12") },
	};
	inline const FLoadoutWeapon LoadoutSecondaries[] = {
		{ TEXT("P17"), 1, TEXT("/Game/Weapons/P17/DA_P17.DA_P17") },
	};
	inline int32 FindPrimary(const FString& Id)
	{
		for (int32 i = 0; i < UE_ARRAY_COUNT(LoadoutPrimaries); ++i)
		{
			if (Id.Equals(LoadoutPrimaries[i].Id, ESearchCase::IgnoreCase)) { return i; }
		}
		return 0;
	}

	inline const FIntel Intel[] = {
		{ TEXT("KESSRA"), TEXT("Segunda ciudad de la República de Varania y su gran puerto de aguas profundas: 900.000 habitantes antes de la guerra, "
			"refinería, bloques obreros de los años 60 trepando por las colinas y un casco viejo junto a la ría. En invierno el amanecer llega frío "
			"y gris, con las farolas de sodio todavía encendidas.") },
		{ TEXT("LA LÍNEA NEGRA"), TEXT("Marzo de 2030: el intento de privatizar el puerto acaba en tiroteos y la policía pierde el este de la ciudad en una "
			"semana. Desde entonces una franja de calles vacías, muros de contenedores y edificios quemados sigue el antiguo trazado del ferrocarril. "
			"Los vecinos la llaman la línea negra, por el humo de los neumáticos que ardían en los cruces.") },
		{ TEXT("COLUMNA VESK"), TEXT("Milicia nacida de los comités de estibadores y de antiguos reservistas. Lleva el nombre de Ilian Vesk, sindicalista muerto "
			"en los primeros tiroteos. Desde finales de 2030 aparece con fusiles nuevos, radios cifradas y una disciplina de fuego que no aprendió en el "
			"puerto. Se reconocen por el brazalete negro. Quieren el puerto. No dicen quién les paga.") },
		{ TEXT("CORVANE SECURITY"), TEXT("Contratista militar privada. Oficialmente no está en Varania. Tiene instructores con la Columna desde otoño de 2030 y "
			"usa la guerra como banco de pruebas: munición, drones, comunicaciones y blindados que no puede probar en ningún otro sitio sin que nadie "
			"pregunte. Su responsable en Kessra solo aparece en fotos tomadas de lejos: lo llaman \"el inglés\".") },
		{ TEXT("GRUPO OPERATIVO BLACKLINE"), TEXT("Unidad de operaciones especiales de la coalición de observación desplegada en Varania para \"asesorar\". No combate "
			"oficialmente. Su misión real es reunir pruebas de la implicación de Corvane. Equipos de dos a cuatro, de noche o al amanecer. "
			"El mando habla desde el oeste de la ciudad con el indicativo TORRE.") },
		{ TEXT("SGTO. ADRIÁN ROCA — SABLE 2-1"), TEXT("34 años, once de servicio, dos despliegues anteriores. Callado, metódico, tira a no fallar. Habla poco por radio y "
			"nunca de más. No le gusta la palabra \"asesor\".") },
		{ TEXT("TOMAS VAREK"), TEXT("Exempleado de logística del puerto. Llevaba los manifiestos de carga del lado este y lleva meses pasando a BLACKLINE fotos de cajas "
			"sin marcas y números de contenedor. Dejaba notas a lápiz en los márgenes: \"esto no es grano\", \"pesa demasiado\", \"otra vez el barco sin bandera\". "
			"Dice tener algo grande guardado en un disco duro.") },
		{ TEXT("OPERADORES DE CORVANE"), TEXT("Contratistas con equipo de gama alta sin marcas: placas, cascos con visor, radios cifradas. No gritan ni se "
			"avisan a voces como la Columna: se les reconoce por el silencio, por la puntería y por las granadas que usan para sacarte de la cobertura. "
			"Sus tiradores llevan láser.") },
		{ TEXT("EL BARCO SIN BANDERA"), TEXT("Un mercante gris, sin nombre ni bandera, que fondea en la ría del casco viejo cuando hay niebla. Aparece una y otra vez "
			"en las notas de Varek: \"otra vez el barco sin bandera\". El práctico del puerto viejo lo guía de noche.") },
		{ TEXT("TTE. ILIĆ"), TEXT("Oficial del ejército de Varania. Su compañía defiende el puente del ferrocarril, el único paso sobre el río que la línea negra "
			"no ha cortado. Desconfía de los \"asesores\".") },
		{ TEXT("TORRE"), TEXT("Puesto de mando de BLACKLINE en Kessra. La voz es la del Capitán Elias Marot, oficial de inteligencia: directo, sin adornos, "
			"siempre un paso por detrás de lo que de verdad está pasando y consciente de ello. Nunca dice nombres reales por radio.") },
	};
}
