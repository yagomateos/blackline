#pragma once

#include "CoreMinimal.h"

/** Textos del menú (Bloque 10). Fuente: Docs/Lore.md. */
namespace BLMenuData
{
	struct FMission { const TCHAR* Code; const TCHAR* Name; const TCHAR* Place; bool bAvailable; const TCHAR* Briefing; };
	struct FWeapon { const TCHAR* Name; const TCHAR* Class; const TCHAR* Specs; const TCHAR* Text; bool bAvailable; float Stats[5]; };
	struct FIntel { const TCHAR* Title; const TCHAR* Body; };

	inline const FMission Missions[] = {
		{ TEXT("01"), TEXT("AMANECER ROTO"), TEXT("KESSRA-ESTE · PUERTO Y CALLE MAYOR · 06:10"), true,
			TEXT("Tomas Varek, nuestro informante en el puerto, no responde desde las 04:00. Su refugio está al final de la calle "
				 "principal, al otro lado del patio de contenedores. Entra por los callejones del norte, cruza el patio, pasa el control "
				 "de carretera de la Columna y recupera su disco duro. Patrullas por parejas, posiciones con sacos terreros, sin apoyo aéreo. "
				 "Evita el ruido mientras puedas.") },
		{ TEXT("02"), TEXT("MANIFIESTO"), TEXT("REFINERÍA DE KESSRA · NOCHE"), false,
			TEXT("El disco de Varek apunta a un almacén de la refinería. Infiltración nocturna para fotografiar el contenido de los contenedores.") },
		{ TEXT("03"), TEXT("RÍA"), TEXT("CASCO VIEJO · AMANECER"), false,
			TEXT("Varek sigue vivo, retenido en el casco viejo junto a la ría. Rescate casa por casa. Primeros indicios de operadores de Corvane.") },
		{ TEXT("04"), TEXT("FUEGO CRUZADO"), TEXT("PUENTE DEL FERROCARRIL · DÍA"), false,
			TEXT("La Columna intenta cruzar la línea negra. Defensa del puente con el ejército de Varania mientras TORRE saca las pruebas del país.") },
		{ TEXT("05"), TEXT("LÍNEA NEGRA"), TEXT("PUERTO DE KESSRA · AMANECER"), false,
			TEXT("Asalto final al puerto para capturar a \"el inglés\" y los registros de Corvane antes de que lo destruyan todo.") },
	};

	inline const TCHAR* StartPhases[] = { TEXT("INSERCIÓN"), TEXT("PATIO DEL PUERTO"), TEXT("CONTROL DE CARRETERA"), TEXT("CALLE PRINCIPAL"), TEXT("LOCAL DE VAREK") };
	inline const TCHAR* StartPhaseIds[] = { TEXT("Fase1"), TEXT("Fase2"), TEXT("Fase3"), TEXT("Fase4"), TEXT("Objetivo") };

	// Estadísticas 0..1: daño, cadencia, alcance, control, movilidad
	inline const FWeapon Weapons[] = {
		{ TEXT("AR-7 \"HALCÓN\""), TEXT("FUSIL DE ASALTO · PRINCIPAL"), TEXT("5,56 MM · 750 DPM · CARGADOR 30+1 · MIRAS METÁLICAS DE ANILLO"),
			TEXT("Fusil de pistón corto con guardamanos M-LOK y apagallamas de jaula. Preciso y controlable en ráfagas cortas; "
				 "la recarga táctica deja una bala en la recámara."), true, { 0.6f, 0.7f, 0.7f, 0.65f, 0.6f } },
		{ TEXT("P-17"), TEXT("PISTOLA · SECUNDARIA"), TEXT("9 MM · SEMIAUTOMÁTICA · CARGADOR 17"), TEXT("Arma de apoyo para cuando el fusil se queda sin cargador."), false, { 0.35f, 0.4f, 0.3f, 0.75f, 0.95f } },
		{ TEXT("SG-12 \"MASTÍN\""), TEXT("ESCOPETA · CQB"), TEXT("CAL. 12 · CORREDERA · 8 CARTUCHOS"), TEXT("Para despejar pisos y escaleras en el asalto al bloque de viviendas."), false, { 0.95f, 0.2f, 0.15f, 0.4f, 0.55f } },
		{ TEXT("SMG-9 \"VESPER\""), TEXT("SUBFUSIL · CQB"), TEXT("9 MM · 900 DPM · CARGADOR 30"), TEXT("Compacto y rápido para interiores."), false, { 0.4f, 0.9f, 0.35f, 0.6f, 0.85f } },
		{ TEXT("M-6"), TEXT("GRANADA DE FRAGMENTACIÓN"), TEXT("ESPOLETA DE 4 S · RADIO LETAL 5 M"), TEXT("Saca al enemigo de su cobertura."), false, { 0.9f, 0.1f, 0.4f, 0.5f, 0.9f } },
	};

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
		{ TEXT("TORRE"), TEXT("Puesto de mando de BLACKLINE en Kessra. La voz es la del Capitán Elias Marot, oficial de inteligencia: directo, sin adornos, "
			"siempre un paso por detrás de lo que de verdad está pasando y consciente de ello. Nunca dice nombres reales por radio.") },
	};
}
