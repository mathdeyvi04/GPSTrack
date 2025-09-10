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
 * @brief Versão Simulada do Sensor GPS, gerando frases no padrão NMEA. 
 * @details
 * 
 * Criará um par de pseudo-terminais (PTY) para simular o funcionamento do sensor.
 * Intervaladamente, gera frases NMEA simuladas.
 */
class GPSSim {
private:

    // Informações ligadas ao terminal
    int fd_pai{0}, fd_filho{0};
    char caminho_do_pseudo_terminal[128];

    // Thread de Execução Paralela e Flag de Controle
    std::thread worker;
    std::atomic<bool> is_exec{false};

    // Informações de Localização 
    double lat, lon, alt;

    /**
     * @brief Converte graus decimais para formato NMEA de localização, (ddmm.mmmm).   
     * @param graus_decimais Valor em graus decimais
     * @param is_lat Flag de eixo
     * @param[out] ddmm String com valor formatado em graus e minutos
     * @param[out] hemisf Caractere indicando Hemisfério
     */ 
    static void 
    degrees_to_NMEA(
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
     * @brief Formatada um número inteiro com dois dígitos, preenchendo com zero à esquerda.
     * @param valor Número a ser formatado
     * @param quant_digitos Quantidade de Dígitos presente
     * @return string formatada    
     */
    static std::string
    format_integer(
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
     * @brief Obtém o tempo UTC atual, horário em Londres. 
     * @return Struct std::tm contendo o tempo em UTC  
     */
    static std::tm 
    get_utc_time(){

        using namespace std::chrono;
        auto tempo_atual = system_clock::to_time_t(system_clock::now());
        std::tm tempo_utc{}; 
        gmtime_r(&tempo_atual, &tempo_utc); 
        return tempo_utc;
    }

    /**
     * @brief Finaliza uma sentença NMEA a partir do corpo da frase.
     * @details 
     * 
     * Calcula o valor de paridade (checksum) do corpo da frase fornecida,
     * em seguida adiciona os delimitadores e flags no formato NMEA 
     * (prefixo '$', sufixo '*', valor de paridade em hexadecimal e "\r\n").
     *
     * @param corpo_frase Corpo da frase NMEA sem os indicadores iniciais ('$') e finais ('*' e checksum).
     * @return std::string Sentença NMEA completa, pronta para transmissão.
     */
    static std::string 
    build_nmea_string(
        const std::string& corpo_frase
    ){

        uint8_t paridade = 0;
        for(
            const char& caract : corpo_frase
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
     * @brief Classe responsável por agrupar as funções geradoras de sentenças NMEA
     * @details
     * 
     * Contém apenas os métodos estáticos que constroem a informação a ser posta na string NMEA.
     */
    class NMEAGenerator {
    public:

        /**
         * @brief Gera uma frase GGA (Global Positioning System Fix Data)
         * @details
         * 
         * Apesar de usar apenas valores de lat, long e alt, zera os demais valores.
         * 
         * @param lat_graus Latitude  em graus decimais
         * @param lon_graus Longitude em graus decimais
         * @param alt_metros Altitude em metros
         * @return String correspondendo ao corpo de frase GGA
         */
        static std::string
        generate_gga(
            int lat_graus,
            int lon_graus, 
            int alt_metros
        ){ 

            auto tempo_utc = get_utc_time();
            std::string lat_nmea, lon_nmea; 
            char hemisferio_lat, hemisferio_lon;
            
            degrees_to_NMEA(
                lat_graus, 
                true, 
                lat_nmea,
                hemisferio_lat
            );
            degrees_to_NMEA(
                lon_graus,
                false,
                lon_nmea, 
                hemisferio_lon
            );
            
            // Formato: hhmmss.ss,lat,N/S,lon,E/W,qualidade,satelites,HDOP,altitude,M,...
            std::ostringstream oss;
            oss << "GPGGA," << format_integer(tempo_utc.tm_hour, 2) 
                << format_integer(tempo_utc.tm_min, 2) 
                << format_integer(tempo_utc.tm_sec, 2) << ".00,"
                << lat_nmea   << "," << hemisferio_lat << ","
                << lon_nmea   << "," << hemisferio_lon << ",1,"
                << -1  << "," << std::fixed << std::setprecision(1) << -1 << ","
                << std::fixed << std::setprecision(1) << alt_metros << ",M,0.0,M,,";
                
            return build_nmea_string(oss.str());
        }

        /**
         * @brief Gera uma frase GGA (Recommended Minimum Navigation Information)
         * @details
         * 
         * Apesar de usar apenas valores de lat, long e alt, zera os demais valores.
         * 
         * @param lat_graus Latitude  em graus decimais
         * @param lon_graus Longitude em graus decimais
         * @param alt_metros Altitude em metros
         * @return String correspondendo ao corpo de frase GGA
         */
        static std::string
        generate_rmc(
            int lat_graus,
            int lon_graus,
            int alt_metros
        ){

            auto tempo_utc = get_utc_time();
            std::string lat_nmea, lon_nmea; 
            char hemisferio_lat, hemisferio_lon;
            
            degrees_to_NMEA(
                lat_graus, 
                true, 
                lat_nmea,
                hemisferio_lat
            );
            degrees_to_NMEA(
                lon_graus,
                false,
                lon_nmea, 
                hemisferio_lon
            );
            
            // Formato: hhmmss.ss,A,lat,N/S,lon,E/W,velocidade,curso,data,,,
            std::ostringstream oss;
            oss << "GPRMC," 
                << format_integer(tempo_utc.tm_hour, 2) 
                << format_integer(tempo_utc.tm_min,  2) 
                << format_integer(tempo_utc.tm_sec,  2) << ".00,A,"
                << lat_nmea << "," << hemisferio_lat << ","
                << lon_nmea << "," << hemisferio_lon << ","
                << std::fixed << std::setprecision(2) << -1 << ",0.00,"
                << format_integer(tempo_utc.tm_mday, 2) 
                << format_integer(tempo_utc.tm_mon + 1, 2) 
                << format_integer((tempo_utc.tm_year + 1900) % 100, 2)
                << ",,,A";
                
            return build_nmea_string(oss.str());
        }
    };

    /**
     * @brief Loop principal responsável pela geração e transmissão de dados simulados.
     *
     * @details
     * Esta função executa um laço contínuo enquanto o simulador estiver ativo (`_is_exec`).
     * Em cada iteração:
     *  - Inicializa ou atualiza a posição simulada (latitude e longitude).
     *  - Gera uma sentença NMEA do tipo GGA a partir da posição atual.
     *  - Transmite a sentença gerada através do descritor de escrita `_fd_pai`.
     *  - Aguarda o período de atualização definido em `_periodo_atualizacao`.
     *  - Atualiza a posição simulada chamando `_update_position()`.
     *
     * O loop termina automaticamente quando `_is_exec` é definido como falso.
     *
     * @note 
     * Esta função é bloqueante e deve ser executada em uma thread dedicada
     * para não interromper o fluxo principal do programa.
     */
    void 
    loop(){
        
        while (is_exec){
            

            std::string saida = NMEAGenerator::generate_gga(lat, lon, alt);
            std::cout << "\033[7mGPS6MV2 Simulado Emitindo:\033[0m \n" << saida << std::endl;
            
            // Imprimimos no terminal serial
            (void)!::write(fd_pai, saida.data(), saida.size());
            
            // Aguarda o próximo ciclo
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

public:

    /**
     * @brief Construtor do GPSSim
     * @details
     * Inicializa alguns parâmetros de posição simulada e, criando os pseudo-terminais, configura-os 
     * para o padrão do módulo real.
     *  
     * @param latitude_inicial_graus Latitude inicial em graus decimais
     * @param longitude_inicial_graus Longitude inicial em graus decimais
     * @param altitude_metros Altitude inicial em metros, setada para 10.
     */
    GPSSim(
        double latitude_inicial_graus,
        double longitude_inicial_graus,
        double altitude_metros
    ) : lat(latitude_inicial_graus), 
        lon(longitude_inicial_graus), 
        alt(altitude_metros)
    {
        
        // Cria o par de pseudo-terminais
        if(
            ::openpty( &fd_pai, &fd_filho, caminho_do_pseudo_terminal, nullptr, nullptr ) != 0
        ){
            throw std::runtime_error("Falha ao criar pseudo-terminal");
        }
        
        // Configura o terminal filho para simular o módulo real (9600 8N1)
        termios config_com{};              // Cria a estrutura vazia
        ::tcgetattr(fd_filho, &config_com); // Lê as configurações atuais e armazena na struct
        ::cfsetispeed(&config_com, B9600);   // Definimos velocidade de entrada e de saída
        ::cfsetospeed(&config_com, B9600);   // Essa constante está presente dentro do termios.h
        // Diversas operações bits a bits
        config_com.c_cflag = (config_com.c_cflag & ~CSIZE) | CS8;  
        config_com.c_cflag |= (CLOCAL | CREAD);                        
        config_com.c_cflag &= ~(PARENB | CSTOPB);                  
        config_com.c_iflag = IGNPAR; 
        config_com.c_oflag = 0; 
        config_com.c_lflag = 0;
        tcsetattr(fd_filho, TCSANOW, &config_com);  // Aplicamos as configurações
        
        // Fecha o filho - será aberto pelo usuário no caminho correto
        // Mantemos o Pai aberto para procedimentos posteriores
        ::close(fd_filho); 
    }

    /**
     * @brief Destrutor do GPSSim
     * @details
     * 
     * Chama a função `stop()` e, após verificar existência de terminal Pai, fecha-o.
     */ 
    ~GPSSim(){ stop(); if( fd_pai >= 0 ){ ::close(fd_pai); } }

    /**
     * @brief Inicia a geração de frases no padrão NMEA em uma thread separada.
     * @details
     * 
     * O padrão NMEA é o protocolo padrão usado por módulos GPS, cada mensagem 
     * começa com $ e termina com \\r\\n. Exemplo:
     * - $origem,codificacao_usada,dados1,dados2,...*paridade
     */
    void 
    init(){

        if( is_exec.exchange(true) ){ return; }

        worker = std::thread(
                            [this]{ loop(); }
                            );
    }

    /**
     * @brief Encerrará a geração de frases NMEA e aguardará a finalização da thread.    
     * @details
     * 
     * Verifica a execução da thread e encerra-a caso exista.
     * Força a finalização da thread geradora de mensagens.
     */
    void 
    stop(){

        if( !is_exec.exchange(false) ){ return; }

        if(
            worker.joinable()
        ){
            
            worker.join();            
        }
    }

    /**
     * @brief Obtém o caminho do terminal que estamos executando de forma filial.
     * @return String correspondendo ao caminho do dispositivo.  
     */
    std::string
    get_path_pseudo_term() const { return std::string(caminho_do_pseudo_terminal); }
};

#endif // GPSSim_HPP