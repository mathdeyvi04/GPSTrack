/**
 * @file debug.cpp
 * @brief Responsável por prover ferramentas de debug.
 * @details 
 * Já que a aplicação deve ser executada dentro da placa, não conseguiríamos
 * executá-la no DeskTop. Para tanto, fez-se necessário o desenvolvimento
 * de ferramentas que possibilitam a debugação de nosso código.
 */
#include <iostream>
#include "TrackSense.hpp"
#include "GPSSim.hpp"

// IME - -22.9559, -43.1659

int main(){

	// Inicializamos o módulo gps simulado
	GPSSim gps_module(
		-22.9559,
		-43.1659,
		760.0,
		1.0,
		2.0
	);
	gps_module.config_traj();
	std::cout << "Executando simulator_gps_module em: "
			  << gps_module.obter_caminho_terminal_filho()
			  << "\n";
	gps_module.init();

    //std::this_thread::sleep_for(std::chrono::seconds(3));

	TrackSense sensor(
		"127.0.0.1",
		9000,
		gps_module.obter_caminho_terminal_filho()
	);
	sensor.init();

	std::this_thread::sleep_for(std::chrono::seconds(150));
	sensor.stop();
	gps_module.stop();
	return 0;
}
