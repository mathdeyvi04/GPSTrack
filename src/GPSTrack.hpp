/**
 * @file GPSTrack.hpp
 * @brief Implementação da solução embarcada.
 */
#ifndef GPSTRACK_HPP
#define GPSTRACK_HPP


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
 
#include <stdexcept>

// Específicos de Sistemas Linux
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/**
 * @class GPSTrack
 * @brief Classe responsável por obter o tracking da carga.
 * @details
 * Responsabilidades:
 * - Obter os dados do sensor NEO6MV2
 * - Interpretar esses dados, gerando informações
 * - Enviar as informações via socket UDP em formato CSV
 * 
 * Cada uma dessas responsabilidades está associada a um método da classe, respectivamente:
 * 
 * - read_serial()
 * - GPSData::parsing()
 * - send()
 * 
 * Os quais estarão sendo repetidamente executados pela thread worker a fim de manter a
 * continuidade de informações. 
 * 
 * Não há necessidade de mais explicações, já que o fluxo de funcionamento é simples.
 */
class GPSTrack {
public:

	/**
	 * @class GPSData
	 * @brief Classe responsável por representar os dados do GPS
	 * @details
	 * 
	 * Utilizar struct mostrou-se básico demais para atenter as necessidades
	 * de representação dos dados do GPS. Com isso, a evolução para Class tornou-se
	 * inevitável.
	 * 
	 * O sensor inicia a comunicação enviando mensagens do tipo $GPTXT.
	 * Essas mensagens são mensagens de texto informativas, geralmente vazias.
	 * Esse é um comportamento normal nos primeiros segundos após energizar o módulo.
	 *
	 * Após adquirir sinais de satélites e iniciar a navegação, o módulo passa a enviar
	 * sentenças NMEA padrão, que trazem informações úteis:
	 * 
	 * - $GPRMC (Recommended Minimum Navigation Information): 
	 *   Fornece dados essenciais de navegação, como horário UTC, status, 
	 *   latitude, longitude, velocidade, curso e data.
	 *
	 * - $GPVTG (Course over Ground and Ground Speed): 
	 *   Informa direção do movimento (rumo) e velocidade sobre o solo.
	 *
	 * - $GPGGA (Global Positioning System Fix Data): 
	 *   Dados de fixação do GPS, incluindo número de satélites usados, 
	 *   qualidade do sinal, altitude e posição.
	 *
	 * - $GPGSA (GNSS DOP and Active Satellites): 
	 *   Status dos satélites em uso e precisão (DOP - Dilution of Precision).
	 *
	 * - $GPGSV (GNSS Satellites in View): 
	 *   Informações sobre os satélites visíveis, como elevação, azimute 
	 *   e intensidade de sinal.
	 *
	 * - $GPGLL (Geographic Position - Latitude/Longitude): 
	 *   Posição geográfica em latitude e longitude, com horário associado.
	 * 
	 * Sendo assim, o sensor sai de um modo de inicialização para operacionalidade completa.
	 * 
	 * Como nosso próposito é apenas localização, nos interessa apenas o padrão GGA, o qual
	 * oferece dados profundos de localização.
	 */
	class GPSData {
	private:

		std::string pattern;
		std::vector<std::string> data; ///< Organizaremos os dados neste vetor.

	public:

		/**
		 * @brief Construtor Default
		 * @details
		 * 
		 * Reserva no vetor de dados 4 strings, respectivamente para horário UTC, latitude,
		 * longitude e altitude.	
		 */
		GPSData(){ data.reserve(4); }

		/**
		 * @brief Função estática auxiliar para converter coordenadas NMEA (latitude/longitude) para graus decimais.
		 * @param string_numerica  String com a coordenada em formato NMEA (ex: "2257.34613").
		 * @param string_hemisf String com o hemisfério correspondente ("N", "S", "E", "W").
		 * @return String coordenada em graus decimais (negativa para hemisférios Sul e Oeste).
		 */
		static std::string 
		converter_lat_lon(
			const std::string& string_numerica,
			const std::string& string_hemisf
		){

			if( string_numerica.empty() ){ return ""; }
			
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
			double coordenada = (graus + minutos / 60.0) * ( (string_hemisf == "S" || string_hemisf == "W" ) ? -1 : 1 );

			return std::to_string(coordenada);
		}

		/**
		 * @brief Setará os dados baseado no padrão de mensagem recebido.
		 * @param code_pattern Código para informar que padrão de mensagem recebeu.
		 * @param data_splitted Vetor de dados da mensagem recebida.
		 * @return Retornará true caso seja bem sucedido. False, caso contrário.
		 * @details
		 * 
		 * Tradução de códigos:
		 * 
		 * - 0 == GPGGA
		 * 
		 * A partir do padrão de mensagem recebida, organizaremos o vetor com dados relevantes.		
		 */
		bool
		parsing(
			int code_pattern,
			const std::vector<std::string>& data_splitted
		){

			data.clear(); // Garantimos que está limpo.

			if(
				code_pattern == 0
			){
				// Em gga, os dados corretos estão em:

				int idx_data_useful[] = {
										1, // Horário UTC
										2, // Latitude  em NMEA
										4, // Longitude em NMEA
										9  // Altitude
										};

				for(
					const auto& idx : idx_data_useful
				){

					if( idx == 2 || idx == 4 ){
						data.emplace_back(
										 std::move(converter_lat_lon(
										 							data_splitted[idx],
										 							data_splitted[idx + 1]
										 							))
										 );
					}
					else{
						// Então basta adicionar
						data.emplace_back(
										 // Método bizurado para não realizarmos cópias
										 std::move(data_splitted[idx])
										 );
					}
				}

				return true;
			}
			// ... podemos escalar para novos padrões de mensagem
			
			return false;
		}

		/**
		 * @brief Retorna os dados armazenados em formato CSV.
		 * @return std::string Linha CSV com os valores.
		 */
		std::string 
		to_csv() const {

		    std::ostringstream oss;
		    for (
		    	int i = 0; 
	    		    i < 4; 
	    		    i++
		    ){
		        if(i > 0){ oss << ","; }

		        oss << data[i];
		    }

		    oss << "\n";

		    return oss.str();
		}
	};

	/**
	 * @brief Função estática auxiliar para separar uma string em vetores de string.
	 * @param string_de_entrada String que será fatiada.
	 * @param separador Caractere que será a flag de separação. 
	 * @details
	 * Similar ao método `split` do python, utiliza ',' como caractere separador default.
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

private:
	// Relacionadas ao Envio UDP
	std::string ip_destino; 
	int      porta_destino;
	int             sockfd;
	sockaddr_in  addr_dest{};

	// Relacionados ao fluxo de funcionamento
	std::thread                worker;
	std::atomic<bool> is_exec{false};

	// Relacionados à comunicação com o sensor
	GPSData   last_data_given;
	std::string  porta_serial;
	int        fd_serial = -1;
	
	/**
	 * @brief Abre e configura a porta serial para comunicação o sensor.
	 * @details
	 * 
	 * Estabelece a conexão serial utilizando a porta serial especificada. 
	 * Todos os parâmetros necessários para uma comunicação estável com o dispositivo,
	 * incluindo velocidade, formato de dados e controle de fluxo são setados.
	 * 
	 * A porta é aberta em modo somente leitura (O_RDONLY) e em modo raw, no qual não há
	 * processamento adicional dos caracteres.
	 * 
	 * Aplicamos as seguintes configurações:
	 * 
	 * - 9600 bauds
	 * - 8 bits de dados
	 * - Sem paridade
	 * - 1 bit de parada
	 */
	void
	open_serial(){

		fd_serial = ::open(
							porta_serial.c_str(),
							// O_RDONLY: garante apenas leitura
							// O_NOCTTY: impede que a porta se torne o terminal controlador do processo
    						// O_SYNC:   garante que as operações de escrita sejam completadas fisicamente
							O_RDONLY | O_NOCTTY | O_SYNC
						   );

		// Confirmação de sucesso
		if( fd_serial < 0 ){ throw std::runtime_error("\033[1;31mErro ao abrir porta serial do GPS\033[0m"); }

		// Struct para armazenarmos os parâmetros da comunicação serial.
		termios tty{};
		if(
			::tcgetattr(
 						fd_serial,
 						&tty
				       ) != 0
		){ throw std::runtime_error("\033[1;31mErro ao tentar configurar a porta serial, especificamente, tcgetattr\033[0m"); }

		// Setamos velocidade
		::cfsetospeed(&tty, B9600);
        ::cfsetispeed(&tty, B9600);

        // Modo raw para não haver processamento por parte do sensor.
        ::cfmakeraw(&tty);

        // Configuração 8N1
		tty.c_cflag &= ~CSIZE;
		tty.c_cflag |= CS8;        // 8 bits
		tty.c_cflag &= ~PARENB;    // sem paridade
		tty.c_cflag &= ~CSTOPB;    // 1 stop bit
		tty.c_cflag &= ~CRTSCTS;   // sem controle de fluxo por hardware

		// Habilita leitura na porta
		tty.c_cflag |= (CLOCAL | CREAD);

		// Não encerra a comunição serial quando 'desligamos'
		tty.c_cflag &= ~HUPCL;

		tty.c_iflag &= ~IGNBRK;
		tty.c_iflag &= ~(IXON | IXOFF | IXANY); // sem controle de fluxo por software
		tty.c_lflag = 0;                        // sem canonical mode, echo, signals
		tty.c_oflag = 0;

		tty.c_cc[VMIN]  = 1; // lê pelo menos 1 caractere
		tty.c_cc[VTIME] = 1; // timeout em décimos de segundo (0.1s)

        if( 
        	::tcsetattr(
        		        fd_serial,
        		        TCSANOW, 
        		        &tty
        			   ) != 0
        ){ throw std::runtime_error("\033[1;31mErro ao tentar setar configurações na comunicação serial, especificamente, tcsetattr\033[0m"); }
	}

	/**
	 * @brief Lê dados da porta serial até encontrar uma quebra de linha.
	 * @details
	 * 
	 * - Caracteres de carriage return ('\\r') são ignorados durante a leitura.
	 * - A função termina quando encontra '\\n' ou quando não há mais dados para ler.
	 * - A leitura é feita caractere por caractere para garantir processamento correto
	 * dos dados do GPS que seguem protocolo NMEA.
	 */
	std::string
	read_serial(){

		std::string buffer;
		char caract = '\0';

		while(
			true
		){

			int n = read(
						fd_serial,
						&caract,
						1
						);

			// Confirmação de sucesso.
			if(n > 0){

				// Então é caractere válido.
				if(caract == '\n'){ break; }
				if(caract != '\r'){ buffer += caract; } // Ignoramos o \r
			
			}
			else if(n == 0){ break; } // Nada a ser lido
			else{

				throw std::runtime_error("\033[1;31mErro na leitura\033[0m");
			}
		}

		return buffer;
	}

	/**
	 * @brief Envia uma string via socket UDP para um servidor.
	 * @param mensagem String a ser enviada.
	 * @return True se a mensagem foi enviada com sucesso. False, caso contrário.
	 */
	bool
	send(
		const std::string& mensagem
	){

		ssize_t bytes = ::sendto(
								 sockfd,
								 mensagem.c_str(),
								 mensagem.size(),
								 0,
								 reinterpret_cast<struct sockaddr*>(&addr_dest),
								 sizeof(addr_dest)
			                     );

		if(bytes < 0){ std::cout << "Erro ao enviar" << std::endl; return false;}

		return true;
	}


	/**
	 * @brief Executa o loop principal de leitura, interpretação e envio de dados via UDP.
	 * @details 
	 * 
	 * Esta função realiza continuamente a leitura de dados da porta serial, interpreta as mensagens
	 * GPS no formato GPGGA, armazena os dados processados e, se desejado, os exibe em formato CSV.
	 * 
	 * O loop executa enquanto a flag de execução estiver ativa, com uma pausa de 1 segundo entre 
	 * cada iteração para evitar consumo excessivo de CPU.
	 */
	void
	loop(){

		bool parsed = false; // Apenas uma flag para sabermos se houve interpretação
		while(
			is_exec
		){

			std::string mensagem = read_serial();

			if(mensagem.empty()){ std::cout << "Nada a ser lido..." << std::endl; }
			else{

				std::cout << "Recebendo: " << mensagem << std::endl;

				if( mensagem.find("GGA") != std::string::npos ){

					last_data_given.parsing(0, split(mensagem));
					parsed = true;
				}
				// ... para escalarmos novos padrões de mensagem
				else{

				}

				if(
					parsed
				){

					std::string mensagem = last_data_given.to_csv();

					std::cout << "Interpretando: \033[7m" 
							  << mensagem
							  << "\033[0m"
							  << std::endl;

					send(
						mensagem
					);
					parsed = false;
					printf("\n");
				}
			}

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

public:

	/**
	 * @brief Construtor da Classe 
	 * @param ip_destino_ Endereço IP de destino.
	 * @param porta_destino_ Porta UDP de destino
	 * @param porta_serial_ Caminho da porta serial
	 * @details
	 * 
	 * Inicializa a comunicação UDP e abre a comunicação serial.
	 */
	GPSTrack(
		const std::string&   ip_destino_,
		int               porta_destino_,
		const std::string& porta_serial_

	) : ip_destino(ip_destino_),
		porta_destino(porta_destino_),
		porta_serial(porta_serial_)
	{

		// As seguintes definições existentes para a comunição UDP.
		sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
		if( sockfd < 0 ){
			throw std::runtime_error("Erro ao criar socket UDP");
		}

		addr_dest.sin_family = AF_INET;
		addr_dest.sin_port = ::htons(porta_destino);
		addr_dest.sin_addr.s_addr = ::inet_addr(ip_destino.c_str());

		open_serial();
	}

	/**
	 * @brief Destrutor da classe.
	 * @details 
	 * 
	 * Realiza a limpeza adequada dos recursos da classe, garantindo o término
	 * seguro das operações. 
	 * 
	 * A ordem de operações é importante:
	 * 1. Interrompe a thread de execução
	 * 2. Fecha o socket de comunicação
	 * 3. Fecha a porta serial
	 */
	~GPSTrack() { stop(); if( sockfd >= 0 ){ ::close(sockfd); } if( fd_serial >= 0 ){ ::close(fd_serial); } }
	
	/**
	 * @brief Inicializa a thread trabalhadora.
	 * @details 
	 * 
	 * Garante que apenas uma única instância da thread de execução será iniciada,
	 * utilizando um flag atômico para controle de estado. Se a thread já estiver em execução,
	 * a função retorna imediatamente sem realizar nova inicialização. Ao iniciar, exibe uma
	 * mensagem colorida no terminal e cria uma thread worker que executa o loop principal
	 * de leitura e comunicação.
	 */
	void
	init(){

		if( is_exec.exchange(true) ){ return; }

		std::cout << "\033[1;32mIniciando Thread de Leitura...\033[0m" << std::endl;
		worker = std::thread(
							  [this]{ loop(); }
							 );
	}

	/**
	 * @brief Finaliza a thread de trabalho de forma segura.
	 * @details 
	 * 
	 * Esta função realiza o desligamento controlado da thread de trabalho. Primeiro, altera o flag
	 * de execução para falso usando operação atômica. Se a thread estiver joinable (executando), imprime uma mensagem de confirmação e
	 * realiza a operação de join para aguardar a finalização segura da thread.
	 */
	void
	stop(){

		if( !is_exec.exchange(false) ){ return; }

		if(
			worker.joinable()
		){

			std::cout << "\033[1;32mSaindo da thread de leitura.\033[0m" << std::endl;
			worker.join();
		}
	}
};

#endif // GPSTRACK_HPP