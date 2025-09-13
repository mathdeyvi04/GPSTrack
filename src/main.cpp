/**
 * @file main.cpp
 * @brief Responsável por executar a aplicação
 */
#include "GPSTrack.hpp"

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

	GPSTrack ss(
		argv[1],
		std::stoi(argv[2]),
		"/dev/ttySTM2"
	);

	ss.init();

	std::this_thread::sleep_for(std::chrono::seconds(60));

	ss.stop();

	return 0;
}