/**
 * @file debug.cpp
 * @brief Responsável por prover ferramentas de debug.
 * @details 
 * Já que a aplicação deve ser executada dentro da placa, não conseguiríamos
 * executá-la no DeskTop. Para tanto, fez-se necessário o desenvolvimento
 * de ferramentas que possibilitam a debugação de nosso código.
 */
#include <iostream>
#include "GPSTracker.hpp"
#include "GPSSim.hpp"

// IME - -22.9559, -43.1659

int main(
    int argc,
	char* argv[]
){
    if(argc == 1){

		std::cout << "Falta informar o IP e a PORTA de destino." << std::endl;
		return -1;
	}
	else if(argc == 2){

		std::cout << "Falta informar a PORTA de destino." << std::endl;
		return -1;
	}
	else if(argc > 3){

		std::cout << "Há argumentos inválidos, informe apenas IP e PORTA de destino." << std::endl;
		return -1;
	}

	// Inicializamos o módulo gps simulado
	GPSSim gps_module(
		-22.9559,
		-43.1659,
		32.0,
		true
	);
	std::cout << "Executando simulator_gps_module em: "
			  << gps_module.get_path_pseudo_term()
			  << "\n";
	gps_module.init();

    // Apenas espera para não encher o buffer
	std::this_thread::sleep_for(std::chrono::seconds(1));

	GPSTracker sensor(
		argv[1],
		std::stoi(argv[2]),
		gps_module.get_path_pseudo_term()
	);
	sensor.init();

	std::this_thread::sleep_for(std::chrono::seconds(60));

	sensor.stop();
	gps_module.stop();
	return 0;
}
