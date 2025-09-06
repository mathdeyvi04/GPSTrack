/**
 * @file TrackSense.hpp
 * @brief Implementação da solução embarcada.
 */
#ifndef TRACKSENSE_HPP
#define TRACKSENSE_HPP


//-------------------------------------------------
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstring>

#include <chrono>
#include <cmath>
#include <ctime>

#include <thread>
#include <atomic>
#include <mutex>
 
#include <stdexcept>

// Específicos de Sistemas Linux
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/**
 * @class TrackSense
 * @brief Classe responsável por obter o tracking da carga.
 * @details
 * Responsabilidades:
 * - Obter dados GNSS reais ou simuladas
 * - Interpretar esses dados, gerando informações
 * - Enviar as informações via socket UDP em formato CSV
 */
class TrackSense {
public:

	/**
	 * @struct GPSData
	 * @brief Struct para armazenar dados GPS	
	 */
	struct GPSData {
		std::string time_utc; 
		double  lat;
		double  lon;
		double  vel;
		double  alt;
		int     sat;
		double hdop;
	};

private:
	// Relacionadas ao Envio UDP
	std::string ip_destino; 
	int      porta_destino;
	int             sockfd;
	sockaddr_in  addr_dest;

	// Relacionados ao fluxo de funcionamento
	std::thread                worker;
	std::atomic<bool> _is_exec{false};

	// Relacionados à comunicação com o sensor
	GPSData   last_data_given;
	std::string  porta_serial;
	int        fd_serial = -1;

	/**
	 * @brief Função estática auxiliar para separar uma string em vetores de string.
	 * @param string_de_entrada String que será fatiada.
	 * @param separador Caractere que será a flag de separação. 
	 * @details
	 * Similar ao método `split` do python, utiliza ',' caractere separador default.
	 */
	static std::vector<std::string>
	split(
		const std::string& string_de_entrada,
		char separador=','
	){

		std::vector<std::string>      elementos;
		std::stringstream ss(string_de_entrada);

		std::string elemento_individual;
		while(
			std::getline(
					  ss,
					  elemento_individual,
					  separador
					 )
		){
			if(
				!elemento_individual.empty()
			){

				elementos.push_back(elemento_individual);	
			}
		}

		return elementos;
	}

	/**
	 * @brief Função estática auxiliar para converter coordenadas NMEA (latitude/longitude) para graus decimais.
	 * @param string_numerica  String com a coordenada em formato NMEA (ex: "2257.34613").
	 * @param string_hemisf String com o hemisfério correspondente ("N", "S", "E", "W").
	 * @return Coordenada em graus decimais (negativa para hemisférios Sul e Oeste).
	 */
	static string 
	converter_lat_lon(
		const std::string& string_numerica,
		const std::string& string_hemisf
	){

		if( string_numerica.empty() ){ return 0.0; }
		
		double valor_cru = 0;
		try {

			valor_cru = std::stod(string_numerica);
		}
		catch (std::invalid_argument&) {

			std::cout << "\033[1;31mErro dentro de converter_lat_lon, valor inválido para stod: \033[0m"
					  << string_numerica
					  << std::endl;
		}

		// Parsing dos valores
		double graus   = floor(valor_cru / 100);
		double minutos = valor_cru - graus * 100;
		double dec     = (graus + minutos / 60.0) * ( (string_hemisf == "S" || string_hemisf == "W" ) ? -1 : 1 );

		return dec;
	}

	/**
	 * 	@brief Abre e configure a porta serial do GPS, possibilitando a leitura direta.
	 */
	void
	open_serial(){

		_fd_serial = ::open(
							_porta_serial.c_str(),
							O_RDONLY | O_NOCTTY | O_SYNC
						   );

		if(
			_fd_serial < 0
		){

			throw std::runtime_error("\033[1;31mErro ao abrir porta serial do GPS\033[0m");
		}

		// Verificamos a porta serial de leitura
		termios tty{};
		if(
			::tcgetattr(
 						_fd_serial,
 						&tty
				       ) != 0
		){

			throw std::runtime_error("\033[1;31mErro ao tentar entrar na porta serial, especificamente, tcgetattr\033[0m");
		}

		::cfsetospeed(&tty, B9600);
        ::cfsetispeed(&tty, B9600);

        cfmakeraw(&tty);

        // Configuração 8N1
		tty.c_cflag &= ~CSIZE;
		tty.c_cflag |= CS8;        // 8 bits
		tty.c_cflag &= ~PARENB;    // sem paridade
		tty.c_cflag &= ~CSTOPB;    // 1 stop bit
		tty.c_cflag &= ~CRTSCTS;   // sem controle de fluxo por hardware

		// Habilita leitura e ignora modem control
		tty.c_cflag |= (CLOCAL | CREAD);

		// Não encerra a comunição serial quando 'desligamos'
		tty.c_cflag &= ~HUPCL;

		// Modo raw (sem processamento de entrada/saída)
		tty.c_iflag &= ~IGNBRK;
		tty.c_iflag &= ~(IXON | IXOFF | IXANY); // sem controle de fluxo por software
		tty.c_lflag = 0;                        // sem canonical mode, echo, signals
		tty.c_oflag = 0;

		tty.c_cc[VMIN]  = 1; // lê pelo menos 1 caractere
		tty.c_cc[VTIME] = 1; // timeout em décimos de segundo (0.1s)

        if( 
        	::tcsetattr(
        		        _fd_serial,
        		        TCSANOW, 
        		        &tty
        			   ) != 0
        ){

            throw std::runtime_error("\033[1;31mErro ao tentar setar configurações na comunicação serial, especificamente, tcsetattr\033[0m");
        }
	}

	/**
	 * @brief Responsável por ler os dados emitos na porta serial.
	 */
	std::string
	_ler_dados(){

		// Devemos fazer uma leitura mais inteligente.

		std::string buffer;
		char caract = '\0';

		while(
			true
		){

			int n = read(
						_fd_serial,
						&caract,
						1
						);

			if(n > 0){

				// Então é caractere válido.
				if(caract == '\n'){ break; }
				if(caract != '\r'){ buffer += caract; printf("\n- {%c}", caract); } // Ignoramos o \r
			
			}
			else if(n == 0){ break; } // Nada a ser lido
			else{

				throw std::runtime_error("Erro na leitura");
			}
		}

		return buffer;
	}


	/**
	 * @brief Interpretará os dados caso venham no padrão RMC.	
	 * @param frase Entrada recebida na porta serial
	 * @param[out] data Último conjunto de dados
	 */
	void
	_parser_rmc(
		const std::string& frase,
		GPSData& data
	){

		std::vector<std::string> campos_de_infos = split(frase, ',');

		// Logo, está fora do padrão rmc.
		if( campos_de_infos.size() < 8 ){ return; }

		data.time_utc = campos_de_infos[1];
		data.lat = converter_lat_lon(campos_de_infos[3], campos_de_infos[4]);
		data.lon = converter_lat_lon(campos_de_infos[5], campos_de_infos[6]);
		try {
			
			data.vel = std::stod(campos_de_infos[7]) * 0.514; // Transformando de nós para m/s
		} 
		catch (std::invalid_argument&) {

			std::cout << "\033[1;31mErro ao dentro de _parser_rmc, valor inválido para stod: \033[0m"
					  << campos_de_infos[7]
					  << std::endl;
			data.vel = 0;
		}
	}

	/**
	 * @brief Interpretará os dados caso venham no padrão GGA	
	 * @param frase Entrada recebida na porta serial
	 * @param[out] data Último conjunto de dados
	 */
	void
	_parser_gga(
		const std::string& frase,
		GPSData& data 
	){

		std::vector<std::string> campos_de_infos = split(frase, ',');

		// Logo, está fora do padrão gga
		if( campos_de_infos.size() < 10 ){ return; }

		data.time_utc = campos_de_infos[1];
		data.lat = converter_lat_lon(campos_de_infos[2], campos_de_infos[3]);
		data.lon = converter_lat_lon(campos_de_infos[4], campos_de_infos[5]);
		data.sat  = std::stoi(campos_de_infos[7]);
		data.hdop = std::stod(campos_de_infos[8]);
		data.alt  = std::stod(campos_de_infos[9]);
	}

	/**
	 * @brief Responsável pelo loop principal de leitura, interpretação e envio via UDP.	
	 */
	void
	_loop(){

		while(
			_is_exec
		){

			std::string linha = _ler_dados();

			std::cout << "Estou recebendo de forma crua: " << linha << std::endl;

			if(
				!linha.empty()
			){

				if( linha.find("RMC") != std::string::npos ){

					_parser_rmc(linha, _last_data_given);
				}
				else if( linha.find("GGA") != std::string::npos ){

					_parser_gga(linha, _last_data_given);
				}
				else{

					std::cout << "\033[1;31mNão consigo codificar...: \033[0m" 
						  << linha 
						  << std::endl;
					continue;
				}	

				// Então temos os dados já codificados.
				std::cout << "\n\033[7mSensor Interpretando:\033[0m\n"
						  << formatar_csv(_last_data_given) 
						  << std::endl;
			}

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

public:

	/**
	 * @brief Construtor da Classe 
	 * @param ip_destino Endereço IP de destiono.
	 * @param porta_destino Porta UDP de destino
	 * @param porta_serial Caminho da porta serial
	 * @details
	 * Inicializa ferramentas de conexão socket UDP.
	 */
	TrackSense(
		const std::string& ip_destino,
		int             porta_destino,
		const std::string& porta_serial

	) : _ip_destino(ip_destino),
		_porta_destino(porta_destino),
		_porta_serial(porta_serial)
	{

		// As seguintes definições existentes para a comunição UDP.
		_sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if( _sockfd < 0 ){
			throw std::runtime_error("Erro ao criar socket UDP");
		}

		_addr_dest.sin_family = AF_INET;
		_addr_dest.sin_port = ::htons(_porta_destino);
		_addr_dest.sin_addr.s_addr = ::inet_addr(_ip_destino.c_str());

		_open_serial();
	}

	/**
	 * @brief Destrutor da Classe
	 * @details
	 * Interrompe thread e finaliza socket	
	 */
	~TrackSense() { stop(); if( _sockfd >= 0 ){ ::close(_sockfd); } if( _fd_serial >= 0 ){ ::close(_fd_serial); } }

	/**
	 * @brief Inicia thread de leitura de dados do GPS e envio UDP. Há garantia de instância única.
	 */
	void
	init(){

		if( _is_exec.exchange(true) ){ return; }

		std::cout << "\033[1;32mIniciando Thread de Leitura...\033[0m" << std::endl;
		_worker = std::thread(
							  [this]{ _loop(); }
							 );
	}

	/**
	 * @brief Finaliza a thread de trabalho.	
	 */
	void
	stop(){

		if( !_is_exec.exchange(false) ){ return; }

		if(
			_worker.joinable()
		){

			std::cout << "\033[1;32mSaindo da thread de leitura.\033[0m" << std::endl;
			_worker.join();
		}
	}
};

#endif // TRACKSENSE_HPP