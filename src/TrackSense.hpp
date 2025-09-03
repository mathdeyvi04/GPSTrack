/**
 * @file TrackSense.hpp
 * @brief Implementação da Classe Sensora do Módulo GPS6MV2
 * @details 
 * Responsável por ler, interpretar e enviar dados via UDP.
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

// Sistemas Linux
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/**
 * @class TrackSense
 * @brief Representa um sensor GPS (GY-GPS6MV2 ou Simulado).
 * @details
 * Responsável por:
 * - Obter informações GNSS reais ou simuladas
 * - Interpretar e armazenar esses dados
 * - Enviar os dados via socket UDP em formato CSV
 * 
 * Para atender essas necessidades, foi implementada diversas funções 
 * independentes em thread e com formas inteligentes de organização.
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
	std::string _ip_destino; 
	int      _porta_destino;
	int             _sockfd;
	sockaddr_in  _addr_dest;

	// Relacionados ao trabalho de leitura
	std::thread               _worker;
	std::atomic<bool> _is_exec{false};

	// Relacionados aos dados lidos
	GPSData _last_data_given{};
	std::string  _porta_serial;
	int        _fd_serial = -1;

	/**
	 * @brief Função estática auxiliar para separar string em vetores de string. Similar ao método `split` do python.
	 */
	static std::vector<std::string>
	split(
		const std::string& string_de_entrada,
		char separador
	){

		std::vector<std::string> elementos;
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
	 * @brief Função estática auxiliar para converter latitude e longitude para graus decimais.
	 * @param valor String representante da medida de latitude ou longitude.
	 * @param hemisf String representante do hemisfério.
	 */
	static double 
	converter_lat_long(
		const std::string& valor,
		const std::string& hemisf
	){

		if( valor.empty() ){ return 0.0; }
		
		double valor_cru = std::stod(valor);
		double graus = floor(valor_cru / 100);
		double minutos = valor_cru - graus * 100;
		double dec = (graus + minutos / 60.0) * ( (hemisf == "S" || hemisf == "W" ) ? -1 : 1 );

		return dec;
	}

	/**
	 * @brief Função estática auxiliar para formar string .csv	
	 */
	static std::string
	formatar_csv(
		const GPSData& data
	){

		std::ostringstream oss;
		oss << data.time_utc << ","
			<< std::fixed     << std::setprecision(6)
			<< data.lat       << ","
			<< data.lon       << ","
			<< std::setprecision(2) << data.vel << ","
			<< data.alt << ","
			<< data.sat << ","
			<< data.hdop;

		return oss.str();
	}

	/**
	 * 	@brief Abre e configure a porta serial do GPS, possibilitando a leitura direta.
	 */
	void
	_open_serial(){

		_fd_serial = ::open(
							_porta_serial.c_str(),
							O_RDONLY | O_NOCTTY | O_SYNC
						   );

		if(
			_fd_serial < 0
		){

			throw std::runtime_error("Erro ao abrir porta serial do GPS");
		}

		// Verificamos a porta serial de leitura
		termios tty{};
		if(
			::tcgetattr(
 						_fd_serial,
 						&tty
				       ) != 0
		){

			throw std::runtime_error("Erro ao tentar entrar na porta serial, tcgetattr");
		}

		::cfsetospeed(&tty, B115200);
        ::cfsetispeed(&tty, B115200);

        // Diversas operações bit a bit
        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8 bits
        tty.c_iflag &= ~IGNBRK;
        tty.c_lflag = 0;
        tty.c_oflag = 0;
        tty.c_cc[VMIN] = 1;
        tty.c_cc[VTIME] = 1;
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~(PARENB | PARODD);
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if( 
        	::tcsetattr(
        		        _fd_serial,
        		        TCSANOW, 
        		        &tty
        			   ) != 0
        ){

            throw std::runtime_error("Erro tcsetattr");
        }
	}

	/**
	 * @brief Responsável por ler os dados emitos na porta serial.
	 * @details
	 * Utiliza um buffer de 256 caracteres.
	 */
	std::string
	_ler_dados(){

		char buffer[256];
		int ult_indice = read(
			                  _fd_serial, 
			                  buffer, 
			                  sizeof(buffer) - 1
			                 );
        
        if( ult_indice > 0 ){

            buffer[ult_indice] = '\0';
            return std::string(buffer);
        }

        std::cout << "\nNada a ser lido..." << std::endl;
        return {};
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
		data.lat = converter_lat_long(campos_de_infos[3], campos_de_infos[4]);
		data.lon = converter_lat_long(campos_de_infos[5], campos_de_infos[6]);
		data.vel = std::stod(campos_de_infos[7]) * 0.514; // Transformando de nós para m/s
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
		data.lat = converter_lat_long(campos_de_infos[2], campos_de_infos[3]);
		data.lon = converter_lat_long(campos_de_infos[4], campos_de_infos[5]);
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
			if(
				!linha.empty()
			){

				if( linha.find("RMC") != std::string::npos ){

					_parser_rmc(linha, _last_data_given);
				}
				else if( linha.find("GGA") != std::string::npos ){

					_parser_gga(linha, _last_data_given);
				}
			}

			// Então temos os dados.
			std::cout << "\n\033[7mSensor Interpretando:\033[0m\n"
					  << formatar_csv(_last_data_given) << std::endl;
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
		const std::string& porta_serial = ""

	) : _ip_destino(ip_destino),
		_porta_destino(porta_destino),
		_porta_serial(porta_serial)
	{

		_sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if( _sockfd < 0 ){ throw std::runtime_error("Erro ao criar socket UDP"); }

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

			std::cout << "\nSaindo da thread de leitura." << std::endl;
			_worker.join();
		}
	}
};

#endif // TRACKSENSE_HPP