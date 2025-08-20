/**
 * @file GPSSim.hpp
 * @brief Implementação da Classe Simuladora do GPS6MV2
 * @details 
 * Supondo que o módulo GPS6MV2 não esteja disponível, a classe implementada 
 * neste arquivo tem como objetivo simular todas as funcionalidades do mesmo.
 */
#ifndef GPSSim_HPP
#define GPSSim_HPP

//----------------------------------------------------------

// Para manipulações de Tempo e de Data
#include <chrono>
#include <ctime>

#include <cmath>
#include <vector>

#include <string>    
#include <sstream>  
#include <iomanip>  

// Para threads e sincronizações
#include <thread>
#include <atomic> 

// Para tratamento de erros
#include <stdexcept>

// As seguintes bibliotecas possuem relevância superior
// Por se tratarem de bibliotecas C, utilizaremos o padrão de `::` para explicitar
// que algumas funções advém delas.
/*
Fornece constantes e funções de controle de descritores de arquivos, 
operações de I/O de baixo nível e manipulação de flags de arquivos.
*/
#include <fcntl.h>   
/*
Fornece acesso a chamadas do OS de baixo nível, incluindo manipulação
de processos, I/O de arquivos, controle de descritores e operações do
sistema de arquivos.
*/
#include <unistd.h>  
/*
Fornece estruturas e funções para configurar a comunicação 
em sistemas Unix. Ele permite o controle detalhado sobre interfaces
de terminal (TTY)
*/
#include <termios.h> 
/*
Fornece funções para criação e manipulação de pseudo-terminais (PTYs),
um mecanismo essencial em sistemas Unix para emular terminais virtuais.
*/
#include <pty.h>     

/**
 * @brief Simulador do módulo GPS que gera frases no padrão NMEA.
 * @details
 * Cria um par de pseudo-terminais (PTY) para simular um o módulo GPS real.
 * Gera frases no padrão NMEA (RMC e GGA) em intervalos regulares, permitindo configuração
 * de posição inicial, altitude, velocidade e padrão de movimento.
 * 
 * Perceba que o objeto é simular os dados gerados, logo estes devem ser difíceis 
 */
class GPSSim {
private:

    // Informações Ligadas Aos Terminais
    int _fd_pai{0}, _fd_filho{0};
    char _nome_pt_filho[128];

    // Thread de Execução Paralela e Flag de Controle
    std::thread _thread_geradora;
    std::atomic<bool> _is_exec{false};

    // Informações da simulação
    double _lat, _lon, _alt;
    double _velocidade_nos;
    std::chrono::milliseconds _periodo_atualizacao;

    // Configurações de Trajetória
    bool   _mov_circ = false;
    double _raio{20.0}, _periodo_circ{20.0};
    double _lat_sim{0.0}, _lon_sim{0.0};
    std::chrono::steady_clock::time_point _start_time = std::chrono::steady_clock::now();


    /**
     * @brief Formatada um número inteiro com dois dígitos, preenchendo com zero à esquerda.
     * @param valor Número a ser formatado
     * @param quant_digitos Quantidade de Dígitos presente
     * @return string formatada    
     */
    static std::string
    formatar_inteiro(
        int valor,
        int quant_digitos
    ){

        std::ostringstream oss;
        oss << std::setw(quant_digitos)
            << std::setfill('0')
            << valor;
        return oss.str();
    }

    /**
     * @brief Converte graus decimais para formato NMEA de localização, (ddmm.mmmm).   
     * @param graus_decimais Valor em graus decimais
     * @param is_lat Flag de eixo
     * @param[out] ddmm String com valor formatado em graus e minutos
     * @param[out] hemisf Caractere indicando Hemisfério
     */ 
    static void 
    converter_graus_para_nmea(
        double graus_decimais,
        bool is_lat,
        std::string& ddmm,
        char& hemisf
    ){

        hemisf = (is_lat) ? (
                            ( graus_decimais >= 0 ) ? 'N' : 'S'
                            ) :
                            (
                            ( graus_decimais >= 0 ) ? 'E' : 'W'
                            );

        double valor_abs = std::fabs(graus_decimais);
        int graus  = static_cast<int>(std::floor(valor_abs));
        double min = (valor_abs - graus) * 60.0;

        // Não utilizamos apenas a função de formatar_inteiro, pois
        // utilizaremos o oss em seguida.
        std::ostringstream oss;
        if(
            is_lat
        ){

            oss << std::setw(2)
                << std::setfill('0')
                << graus
                << std::fixed
                << std::setprecision(4)
                << std::setw(7)
                << std::setfill('0')
                << min;
        }
        else{

            oss << std::setw(3)
                << std::setfill('0')
                << graus
                << std::fixed
                << std::setprecision(4)
                << std::setw(7)
                << std::setfill('0')
                << min;
        }

        ddmm = oss.str();
    }

    /**
     * @brief Após calcular a paridade do corpo da frase, adiciona as flags para o padrão NMEA.
     * @param corpo_frase Frase sem os indicadores $ e *   
     * @return String contendo a frase completa, com todas as informações e flags. 
     */
    static std::string 
    finalizar_corpo_de_frase(
        const std::string& corpo_frase
    ){

        uint8_t paridade = 0;
        for(
            char caract : corpo_frase
        ){

            paridade ^= (uint8_t)caract;
        }

        std::ostringstream oss;
        oss << '$'
            << corpo_frase
            << '*'
            << std::uppercase
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << (int)paridade 
            << "\r\n";

        return oss.str();
    }

    /**
     * @brief Obtém o tempo UTC atual, horário em Londres. 
     * @return Struct std::tm contendo o tempo em UTC  
     */
    static std::tm 
    obter_tempo_utc(){

        using namespace std::chrono;
        auto tempo_atual = system_clock::to_time_t(system_clock::now());
        std::tm tempo_utc{}; 
        gmtime_r(&tempo_atual, &tempo_utc); 
        return tempo_utc;
    }

    /**
     * @brief Gera uma frase RMC (Recommended Minimum Specific GNSS Data)
     * @param lat_graus Latitude  em graus decimais
     * @param lon_graus Longitude em graus decimais
     * @param velocidade_nos Velocidade em nós
     * @return String correspondendo ao corpo de Frase RMC
     */
    std::string 
    _gerar_corpo_de_frase_rmc(
        double lat_graus,
        double lon_graus,
        double velocidade_nos
    ){

        auto tempo_utc = obter_tempo_utc();
        std::string lat_nmea, lon_nmea; 
        char hemisferio_lat, hemisferio_lon;
        
        converter_graus_para_nmea(
            lat_graus, 
            true, 
            lat_nmea,
            hemisferio_lat
        );
        converter_graus_para_nmea(
            lon_graus,
            false,
            lon_nmea, 
            hemisferio_lon
        );
        
        // Formato: hhmmss.ss,A,lat,N/S,lon,E/W,velocidade,curso,data,,,
        std::ostringstream oss;
        oss << "GPRMC," 
            << formatar_inteiro(tempo_utc.tm_hour, 2) 
            << formatar_inteiro(tempo_utc.tm_min,  2) 
            << formatar_inteiro(tempo_utc.tm_sec,  2) << ".00,A,"
            << lat_nmea << "," << hemisferio_lat << ","
            << lon_nmea << "," << hemisferio_lon << ","
            << std::fixed << std::setprecision(2) << velocidade_nos << ",0.00,"
            << formatar_inteiro(tempo_utc.tm_mday, 2) 
            << formatar_inteiro(tempo_utc.tm_mon + 1, 2) 
            << formatar_inteiro((tempo_utc.tm_year + 1900) % 100, 2)
            << ",,,A";
            
        return finalizar_corpo_de_frase(oss.str());
    }

    /**
     * @brief Gera uma frase GGA (Global Positioning System Fix Data)
     * @param lat_graus Latitude  em graus decimais
     * @param lon_graus Longitude em graus decimais
     * @param alt_metros Altitude em metros
     * @param sat Número de satélites visíveis
     * @param hdop Horizontal Dilution of Precision
     * @return String correspondendo ao corpo de frase GGA
     */
    std::string 
    _gerar_corpo_de_frase_gga(
        double lat_graus, 
        double lon_graus, 
        double alt_metros, 
        int    sat = 8, 
        double hdop = 0.9
    ){
        auto tempo_utc = obter_tempo_utc();
        std::string lat_nmea, lon_nmea; 
        char hemisferio_lat, hemisferio_lon;
        
        converter_graus_para_nmea(
            lat_graus, 
            true, 
            lat_nmea,
            hemisferio_lat
        );
        converter_graus_para_nmea(
            lon_graus,
            false,
            lon_nmea, 
            hemisferio_lon
        );
        
        // Formato: hhmmss.ss,lat,N/S,lon,E/W,qualidade,satelites,HDOP,altitude,M,...
        std::ostringstream oss;
        oss << "GPGGA," << formatar_inteiro(tempo_utc.tm_hour, 2) 
            << formatar_inteiro(tempo_utc.tm_min, 2) 
            << formatar_inteiro(tempo_utc.tm_sec, 2) << ".00,"
            << lat_nmea   << "," << hemisferio_lat << ","
            << lon_nmea   << "," << hemisferio_lon << ",1,"
            << sat  << "," << std::fixed << std::setprecision(1) << hdop << ","
            << std::fixed << std::setprecision(1) << alt_metros << ",M,0.0,M,,";
            
        return finalizar_corpo_de_frase(oss.str());
    }

    /**
     * @brief Caso seja movimento circular, atualizará a posição.
     */
    void 
    _update_position(){
        if (!_mov_circ){ return; }
        
        using namespace std::chrono;
        double tempo_decorrido = duration<double>(steady_clock::now() - _start_time).count();
        double angulo = (2.0 * M_PI) * std::fmod(tempo_decorrido / _periodo_circ, 1.0);
        
        // Aproximação: 1° de latitude = 111320 metros
        // Ajuste da longitude pelo cosseno da latitude
        double delta_lat = (_raio * std::sin(angulo)) / 111320.0;
        double delta_lon = (_raio * std::cos(angulo)) / (111320.0 * std::cos(_lat * M_PI / 180.0));
        
        _lat_sim = _lat + delta_lat;
        _lon_sim = _lon + delta_lon;
    }


    /**
     * @brief Loop principal de geração de dados
     */
    void 
    _loop(){
        // Inicializa posição simulada
        _lat_sim = _lat; 
        _lon_sim = _lon;
        
        while (_is_exec){
            
            // Gera frases NMEA
            auto rmc = _gerar_corpo_de_frase_rmc(_lat_sim, _lon_sim, _velocidade_nos);
            auto gga = _gerar_corpo_de_frase_gga(_lat_sim, _lon_sim, _alt, 10, 0.8);  

            const std::string saida = rmc + gga;
            std::cout << "\033[7mGPS6MV2 Simulado Emitindo:\033[0m \n" << saida << std::endl;
            (void)!::write(_fd_pai, saida.data(), saida.size());
            
            // Aguarda o próximo ciclo
            std::this_thread::sleep_for(_periodo_atualizacao);

            _update_position();
        }
    }
	
public:

	/**
     * @brief Construtor do GPSSim
	 * @details
	 * Inicializa alguns parâmetros de posição simulada e, criando os pseudo-terminais, configura-os 
	 * para o padrão do módulo real.

	 * Utilizou-se o termo `explict` para impedir que futuras conversões implicitas sejam 
	 * impedidas. Além disso, foi pensado em utilizar o padrão de `Singleton` para manter apenas uma instância
	 * da classe. Entretanto, após outras reuniões, percebeu-se a falta de necessidade.
	 *  
     * @param latitude_inicial_graus Latitude inicial em graus decimais
     * @param longitude_inicial_graus Longitude inicial em graus decimais
     * @param altitude_metros Altitude inicial em metros, setada para 10.
     * @param frequencia_atualizacao_hz Frequência de atualização das frases NMEA em Hertz, setada para 1
     * @param velocidade_nos Velocidade sobre o fundo em nós (para frase RMC), setada para 0
     */
    explicit 
    GPSSim(
    	double latitude_inicial_graus,
    	double longitude_inicial_graus,
    	double altitude_metros = 10.0,
    	double frequencia_atualizacao_hz = 1.0, 
    	double velocidade_nos = 10.0
    ) : _lat(latitude_inicial_graus), 
        _lon(longitude_inicial_graus), 
        _alt(altitude_metros),
        _periodo_atualizacao((frequencia_atualizacao_hz) > 0 ? 
                             std::chrono::milliseconds((int)std::llround(1000.0/frequencia_atualizacao_hz)) :
                             std::chrono::milliseconds(1000)),
        _velocidade_nos(velocidade_nos) 
    {
        
        // Cria o par de pseudo-terminais
        if(
        	::openpty( &_fd_pai, &_fd_filho, _nome_pt_filho, nullptr, nullptr ) != 0
        ){
            throw std::runtime_error("Falha ao criar pseudo-terminal");
        }
        
        // Configura o terminal filho para simular o módulo real (9600 8N1)
        termios config_com{};              // Cria a estrutura vazia
        ::tcgetattr(_fd_filho, &config_com); // Lê as configurações atuais e armazena na struct
        ::cfsetispeed(&config_com, B115200);   // Definimos velocidade de entrada e de saída
        ::cfsetospeed(&config_com, B115200);   // Essa constante está presente dentro do termios.h
        // Diversas operações bits a bits
        config_com.c_cflag = (config_com.c_cflag & ~CSIZE) | CS8;  
        config_com.c_cflag |= (CLOCAL | CREAD);                        
        config_com.c_cflag &= ~(PARENB | CSTOPB);                  
        config_com.c_iflag = IGNPAR; 
        config_com.c_oflag = 0; 
        config_com.c_lflag = 0;
        tcsetattr(_fd_filho, TCSANOW, &config_com);  // Aplicamos as configurações
        
        // Fecha o filho - será aberto pelo usuário no caminho correto
        // Mantemos o Pai aberto para procedimentos posteriores
        ::close(_fd_filho); 
    }

    /**
     * @brief Destrutor do GPSSim
     * @details
     * Chama a função `stop()` e, após verificar existência de terminal Pai, fecha-o.
     */ 
    ~GPSSim(){ stop(); if( _fd_pai >= 0 ){ ::close(_fd_pai); } }

    /**
     * @brief Obtém o caminho do terminal que estamos executando de forma filial.
     * @return string correspondendo ao caminho do dispositivo.  
     */
    std::string
    obter_caminho_terminal_filho() const { return std::string(_nome_pt_filho); }

    /**
     * @brief Inicia a geração de frases no padrão NMEA em uma thread separada.
     * @details
     * Garante a criação de apenas uma thread utilizando uma variável atômica.
     * O padrão NMEA é o protocolo padrão usado por módulos GPS, cada mensagem 
     * começa com $ e termina com \\r\\n. Exemplo:
     * - $origem,codificacao_usada,dados1,dados2,...*paridade
     */
    void 
    init(){
        if(
            // Variáveis atômicas possuem esta funcionalidade
            _is_exec.exchange(true)
        ){

            return;
        }

        _thread_geradora = std::thread(
                                        // Função Lambda que será executada pela thread
                                        // Observe que os argumentos utilizados serão obtidos pelo
                                        // this inserido nos colchetes
                                        [this]{ _loop(); }
                                      );
    }

    /**
     * @brief Encerrará a geração de frases NMEA e aguardará a finalização da thread.    
     * @details
     * Verifica a execução da thread e encerra-a caso exista.
     * Força a finalização da thread geradora de mensagens.
     */
    void 
    stop(){

        if(
            !_is_exec.exchange(false)
        ){

            return;
        }

        if(
            _thread_geradora.joinable()
        ){
            
            _thread_geradora.join();            
        }
    }

    /**
     * @brief Configura a trajetória circular para a simulação.
     */
    void 
    config_traj(
        double raio_metros = 20.0,
        double periodo_segundos = 120.0
    ){

        if( !_mov_circ ){ return; }

        _raio         = raio_metros;
        _periodo_circ = periodo_segundos;
        _mov_circ     = true;
    }
};

#endif // GPSSim_HPP